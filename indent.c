/*
   Copyright 1999-2018,2019, Thomas E. Dickey

   Copyright (c) 1994,1996,1997, Joseph Arceneaux.  All rights reserved.

   Copyright (c) 1992, Free Software Foundation, Inc.  All rights reserved.

   Copyright (c) 1985 Sun Microsystems, Inc. Copyright (c) 1980 The Regents
   of the University of California. Copyright (c) 1976 Board of Trustees of
   the University of Illinois. All rights reserved.

   Redistribution and use in source and binary forms are permitted
   provided that
   the above copyright notice and this paragraph are duplicated in all such
   forms and that any documentation, advertising materials, and other
   materials related to such distribution and use acknowledge that the
   software was developed by the University of California, Berkeley, the
   University of Illinois, Urbana, and Sun Microsystems, Inc.  The name of
   either University or Sun Microsystems may not be used to endorse or
   promote products derived from this software without specific prior written
   permission. THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES
   OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */

#include "sys.h"
#include "indent.h"
#include "backup.h"

void
usage (void)
{
  fprintf (stderr, "usage: %s file [-o outfile ] [ options ]\n", progname);
  fprintf (stderr, "       %s file1 file2 ... fileN [ options ]\n", progname);
  exit (invocation_error);
}


/* True if we're handling C++ code. */
int c_plus_plus = 0;

/* Section if we're handling lex/yacc code. */
int lex_section = 0;
int next_lexcode = 0;

/* Stuff that needs to be shared with the rest of indent.
   Documented in indent.h.  */
char *labbuf;
char *s_lab;
char *e_lab;
char *l_lab;
char *codebuf;
char *s_code;
char *e_code;
char *l_code;
char *combuf;
char *s_com;
char *e_com;
char *l_com;
struct buf save_com;
char *bp_save;
char *be_save;
int code_lines;
int in_line_no;
int out_line_no;
int out_column_no;
struct fstate keywordf;
struct fstate stringf;
struct fstate boxcomf;
struct fstate blkcomf;
struct fstate scomf;
struct fstate bodyf;
int break_comma;

/* Round up P to be a multiple of SIZE. */
#ifndef ROUND_UP
#define ROUND_UP(p,size) (((unsigned long) (p) + (size) - 1) & (unsigned long)( ~((size) - 1)))
#endif

static void
need_chars (struct buf *bp, size_t needed)
{
  size_t current_size = (size_t) (bp->end - bp->ptr);

  if ((current_size + needed) >= bp->size)
    {
      bp->size = ROUND_UP (current_size + needed, 1024);
      bp->ptr = xrealloc (bp->ptr, bp->size);
      if (bp->ptr == NULL)
	fatal ("Ran out of memory");
      bp->end = bp->ptr + current_size;
    }
}

/* True if there is an embedded comment on this code line */
int embedded_comment_on_line;

int else_or_endif;

/* structure indentation levels */
int *di_stack;

/* Currently allocated size of di_stack.  */
size_t di_stack_alloc;

/* when this is positive, we have seen a ? without
   the matching : in a <c>?<s>:<s> construct */
int squest;

#define CHECK_CODE_SIZE \
	if (e_code >= l_code) { \
	    size_t nsize = (size_t) (l_code - s_code + 400); \
	    codebuf = xrealloc (codebuf, nsize); \
	    e_code = codebuf + (e_code - s_code) + 1; \
	    l_code = codebuf + nsize - 5; \
	    s_code = codebuf + 1; \
	}

#define CHECK_LAB_SIZE \
	if (e_lab >= l_lab) { \
	    size_t nsize = (size_t) (l_lab - s_lab + 400); \
	    labbuf = xrealloc (labbuf, nsize); \
	    e_lab = labbuf + (e_lab - s_lab) + 1; \
	    l_lab = labbuf + nsize - 5; \
	    s_lab = labbuf + 1; \
	}

/* Compute the length of the line we will be outputting. */

static int
output_line_length (void)
{
  int lab_length = 0;
  int code_length = 0;
  int com_length = 0;
  int length;

  if (s_lab != e_lab)
    lab_length = count_columns (1, s_lab, EOL);
  if (s_code != e_code)
    code_length = count_columns (1, s_code, EOL);
  if (s_com != e_com)
    com_length = count_columns (1, s_com, EOL);

  length = 0;
  if (lab_length != 0)
    length += compute_label_target () + lab_length;

  if (code_length != 0)
    {
      if (embedded_comment_on_line)
	length += parser_state_tos->com_col + com_length;

      length += compute_code_target () + code_length;
    }

  return length;
}

/* Warn when we are about to split a line, showing if possible where it will
 * be broken.
 */
static void
warn_broken_line (int which)
{
  int col = token_col ();
  int len = token_len ? token_len : 1;
  const char *s = token;

  if (!isgraph (UChar (*s)))
    {
      switch (*s)
	{
	case ' ':
	  s = "space";
	  break;
	case '\r':
	  s = "carriage return";
	  break;
	case '\n':
	  s = "newline";
	  break;
	case '\t':
	  s = "tab";
	  break;
	case '\f':
	  s = "form feed";
	  break;
	default:
	  s = "nonprinting character";
	  break;
	}
      len = (int) strlen (s);
    }
  message (col, "Line split #%d at %.*s", which, len, s);
}

static void
not_a_decl (void)
{
  int tos = parser_state_tos->tos;

  if (tos > 1
      && (parser_state_tos->p_stack[tos - 1] == elsehead)
      && (parser_state_tos->il[tos] >= ind_size))
    {
      if (debug > 1)
	{
	  printf ("not_a_decl in_parm %d in_decl %d\n",
		  parser_state_tos->in_parameter_declaration,
		  parser_state_tos->in_decl);
	}
      parser_state_tos->last_token = ident;
      parser_state_tos->in_decl = 0;
      parser_state_tos->il[tos] -= ind_size;
      parser_state_tos->i_l_follow = parser_state_tos->il[tos];
    }
}
#define CHECK_LAST_DECL \
    if (parser_state_tos->last_token == decl) \
      not_a_decl()

/* Force a newline at this point in the output stream. */

#define PARSE(token) if (parse (token) != total_success) \
                       file_exit_value = indent_error

static enum exit_values
indent (struct file_buffer *this_file)
{
  int i;
  enum codes hd_type;
  char *t_ptr;
  enum codes last_code;
  enum codes type_code = code_eof;
  enum exit_values file_exit_value = total_success;

  /* current indentation for declarations */
  int dec_ind = 0;

  /* true when we've just see a "case"; determines what to do
     with the following colon */
  int scase = 0;

  /* true when in the expression part of if(...), while(...), etc. */
  int sp_sw = 0;

  /* True if we have just encountered the end of an if (...), etc. (i.e. the
     ')' of the if (...) was the last token).  The variable is set to 2 in
     the middle of the main token reading loop and is decremented at the
     beginning of the loop, so it will reach zero when the second token after
     the ')' is read.  */
  int last_token_ends_sp = 0;

  /* true iff last keyword was an else */
  int last_else = 0;

  /* Used when buffering up comments to remember that
     a newline was passed over */
  int flushed_nl = 0;
  int force_nl = 0;

  /*
   * temporary parse-state
   */
  char *s_key;
  char *e_key;

  in_prog = in_prog_pos = this_file->data;
  in_prog_size = this_file->size;

  hd_type = code_eof;
  dec_ind = 0;
  last_token_ends_sp = false;
  last_else = false;
  sp_sw = force_nl = false;
  scase = false;
  squest = false;

  buf_break = 0;
  break_line = 0;

  if (decl_com_ind <= 0)	/* if not specified by user, set this */
    decl_com_ind =
      ljust_decl ? (com_ind <= 10 ? 2 : com_ind - 8) : com_ind;
  if (continuation_indent == 0)
    continuation_indent = ind_size;
  fill_buffer ();		/* Fill the input buffer */

  {
    char *p = buf_ptr;
    int col = 1;

    while (!at_buffer_end (p))
      {
	if (*p == ' ')
	  col++;
	else if (*p == TAB)
	  col = tabsize - (col % tabsize) + 1;
	else if (*p == EOL)
	  col = 1;
	else
	  break;

	p++;
      }
  }

  /* START OF MAIN LOOP */
  while (1)
    {				/* this is the main loop.  it will go until
				   we reach eof */
      int is_procname_definition;

      last_code = type_code;
      type_code = lexi ();	/* lexi reads one token.  "token" points to
				   the actual characters. lexi returns a code
				   indicating the type of token */

      /* If the last time around we output an identifier or
         a paren, then consider breaking the line here if it's
         too long.

         A similar check is performed at the end of the loop, after
         we've put the token on the line. */
      if (max_col > 0 && buf_break != NULL
	  && ((parser_state_tos->last_token == ident
	       && type_code != comma
	       && type_code != semicolon
	       && type_code != newline
	       && type_code != form_feed
	       && type_code != rparen
	       && type_code != struct_delim)
	      ||
	      (parser_state_tos->last_token == rparen
	       && type_code != comma && type_code != rparen))
	  && output_line_length () > max_col)
	{
	  break_line = 1;
	  force_nl = true;
	}

      if (last_token_ends_sp > 0)
	last_token_ends_sp--;
      is_procname_definition = ((parser_state_tos->procname[0] != '\0'
				 && parser_state_tos->in_parameter_declaration)
				|| parser_state_tos->classname[0] != '\0');

      /* The following code moves everything following an if (), while (),
         else, etc. up to the start of the following statement to a buffer. This
         allows proper handling of both kinds of brace placement. */
      flushed_nl = false;
      while (parser_state_tos->search_brace)
	{
	  /* After scanning an if(), while (), etc., it might be necessary to
	     keep track of the text between the if() and the start of the
	     statement which follows.  Use save_com to do so.  */
	  if (debug > 1)
	    {
	      printf ("token %s\n", parsecode2s (type_code));
	    }
	  switch (type_code)
	    {
	    case newline:
	      flushed_nl = true;
	      /* FALLTHRU */
	    case form_feed:
	      break;		/* form feeds and newlines found here will be
				   ignored */

	    case lbrace:
	      /* Ignore buffering if no comment stored. */
	      if (save_com.end == save_com.ptr)
		{
		  parser_state_tos->search_brace = false;
		  goto check_type;
		}

	      /* We need to put the left curly-brace back into save_com somewhere.  */
	      if (btype_2)
		{
		  /* Put the brace at the beginning of the saved buffer */
		  save_com.ptr[0] = L_CURL;
		  save_com.len = 1;
		  save_com.column = current_column ();
		}
	      else
		{
		  /* Put the brace at the end of the saved buffer, after
		     a newline character.  The newline char will cause
		     a `dump_line' call, thus ensuring that the brace
		     will go into the right column. */
		  *save_com.end++ = EOL;
		  *save_com.end++ = L_CURL;
		  save_com.len += 2;
		}

	      /* Go to common code to get out of this loop.  */
	      goto sw_buffer;

	      /* Save this comment in the `save_com' buffer, for
	         possible re-insertion in the output stream later. */
	    case comment:
	      parser_state_tos->in_comment = true;
	      if ((last_code != rparen || parser_state_tos->paren_depth != 0)
		  && (!flushed_nl || save_com.end != save_com.ptr))
		{
		  need_chars (&save_com, (size_t) 10);
		  if (save_com.end == save_com.ptr)
		    {		/* if this is the first comment, we must set
				   up the buffer */
		      save_com.ptr[0] = save_com.ptr[1] = ' ';
		      save_com.end = save_com.ptr + 2;
		      save_com.len = 2;
		      save_com.column = current_column ();
		    }
		  else
		    {
		      *save_com.end++ = EOL;	/* add newline between
						   comments */
		      *save_com.end++ = ' ';
		      save_com.len += 2;
		    }
		  *save_com.end++ = '/';	/* copy in start of comment */
		  *save_com.end++ = '*';

		  for (;;)
		    {		/* loop until we get to the end of the
				   comment */
		      /* make sure there is room for this character and
		         (while we're at it) the '/' we might add at the end
		         of the loop. */
		      need_chars (&save_com, (size_t) 2);
		      *save_com.end = *buf_ptr++;
		      save_com.len++;
		      if (at_buffer_end (buf_ptr))
			{
			  fill_buffer ();
			  if (had_eof)
			    {
			      message (0, "EOF encountered in comment");
			      return indent_punt;
			    }
			}

		      if (*save_com.end++ == '*' && *buf_ptr == '/')
			break;	/* we are at end of comment */

		    }
		  *save_com.end++ = '/';	/* add ending slash */
		  save_com.len++;
		  if (at_buffer_end (++buf_ptr))	/* get past / in buffer */
		    fill_buffer ();
		  parser_state_tos->in_comment = false;
		  break;
		}
	      parser_state_tos->in_comment = false;
	      /* FALLTHRU */

	      /* Just some statement. */
	    default:
	      /* Some statement.  Unless it's special, arrange
	         to break the line. */
	      if ((type_code == sp_paren
		   && *token == 'i'	/* "if" statement */
		   && last_else
		   && else_if)
		  ||
		  (type_code == sp_nparen
		   && *token == 'e'	/* "else" statement */
		   && e_code != s_code
		   && e_code[-1] == R_CURL))	/* The "else" follows right curly-brace */
		force_nl = false;
	      else if (flushed_nl)
		force_nl = true;

	      if (save_com.end == save_com.ptr)
		{
		  /* ignore buffering if comment wasnt saved up */
		  parser_state_tos->search_brace = false;
		  goto check_type;
		}

	      if (force_nl)
		{
		  force_nl = false;
		  need_chars (&save_com, (size_t) 2);
		  *save_com.end++ = EOL;
		  save_com.len++;
		  if (!flushed_nl)
		    warn_broken_line (1);
		  flushed_nl = false;
		}

	      /* Now copy this token we just found into the
	         saved buffer. */
	      *save_com.end++ = ' ';
	      save_com.len++;
	      buf_ptr = token;

	      parser_state_tos->procname = "\0";
	      parser_state_tos->procname_end = "\0";
	      parser_state_tos->classname = "\0";
	      parser_state_tos->classname_end = "\0";

	      /* Switch input buffers so that calls to lexi() will
	         read from our save buffer. */
	    sw_buffer:
	      parser_state_tos->search_brace = false;
	      bp_save = buf_ptr;
	      be_save = buf_end;
	      buf_ptr = save_com.ptr;
	      need_chars (&save_com, (size_t) 1);
	      buf_end = save_com.end;
	      save_com.end = save_com.ptr;	/* make save_com empty */
	      break;
	    }			/* end of switch */

	  if (type_code != code_eof)
	    {
	      int just_saw_nl = false;

	      if (type_code == newline)
		just_saw_nl = true;
	      type_code = lexi ();
	      if (type_code == newline && just_saw_nl == true)
		{
		  dump_line ();
		  flushed_nl = true;
		}
	      is_procname_definition
		= (parser_state_tos->procname[0] != '\0'
		   && parser_state_tos->in_parameter_declaration);
	    }

	  if (type_code == ident
	      && flushed_nl
	      && !procnames_start_line
	      && parser_state_tos->in_decl
	      && parser_state_tos->procname[0] != '\0')
	    flushed_nl = 0;
	}			/* end of while (search_brace) */

      last_else = 0;

    check_type:
      if (type_code == code_eof)
	{			/* we got eof */
	  if (s_lab != e_lab || s_code != e_code
	      || s_com != e_com)	/* must dump end of line */
	    dump_line ();
	  if (parser_state_tos->tos > 1)	/* check for balanced braces */
	    {
	      message (0, "Unexpected end of file");
	      file_exit_value = indent_error;
	    }

	  if (verbose)
	    {
	      printf ("There were %d output lines and %d comments\n",
		      (int) out_lines, (int) com_lines);
	      if (com_lines > 0 && code_lines > 0)
		printf ("(Lines with comments)/(Lines with code): %6.3f\n",
			(1.0 * com_lines) / code_lines);
	    }
	  fflush (output);

	  return file_exit_value;
	}

      if ((type_code != comment) &&
	  (type_code != cplus_comment) &&
	  (type_code != newline) &&
	  (type_code != preesc) &&
	  (type_code != form_feed))
	{
	  /*
	   * Do not split "({" or "})" chunks since those may be used to
	   * indicate gcc compound statements (ugh).  A more conventional
	   * use is in xterm, which uses the latter in macros to simplify
	   * ifdef's.
	   */
	  if ((force_nl
	       && !(last_code == lparen && type_code == lbrace)
	       && !(type_code == rparen && last_code == rbrace)
	       && (type_code != semicolon)
	       && (type_code != lbrace || !btype_2)))
	    {
	      if (!flushed_nl)
		warn_broken_line (2);

	      flushed_nl = false;
	      if (break_line)
		dump_line ();
	      else
		{
		  dump_line ();
		  parser_state_tos->want_blank = false;
		}
	      force_nl = false;
	    }

	  parser_state_tos->in_stmt = true;	/* turn on flag which causes
						   an extra level of
						   indentation. this is
						   turned off by a semicolon or
						   right curly-brace */
	  if (s_com != e_com)
	    {			/* There is an embedded comment in the current
				   line. Move it from the com buffer to the
				   code buffer.  */
	      /* Do not add a space before the comment if it is the first
	         thing on the line.  */
	      if (e_code != s_code)
		{
		  buf_break = e_code;
		  *e_code++ = ' ';
		  embedded_comment_on_line = 2;
		}
	      else
		embedded_comment_on_line = 1;

	      for (t_ptr = s_com; *t_ptr && (t_ptr < e_com); ++t_ptr)
		{
		  CHECK_CODE_SIZE;
		  *e_code++ = *t_ptr;
		}

	      buf_break = e_code;
	      *e_code++ = ' ';
	      *e_code = '\0';	/* null terminate code sect */
	      parser_state_tos->want_blank = false;
	      e_com = s_com;
	    }
	}
      else if (type_code != comment
	       && type_code != cplus_comment
	       && !(parser_state_tos->last_token == comma &&
		    !leave_comma))
	{
	  /* preserve force_nl thru a comment but
	     cancel forced newline after newline, form feed, etc.
	     however, don't cancel if last thing seen was comma-newline
	     and -bc flag is on. */
	  force_nl = false;
	}



      /* Main switch on type of token scanned */

      CHECK_CODE_SIZE;
      switch (type_code)
	{			/* now, decide what to do with the token */

	case form_feed:	/* found a form feed in line */
	  parser_state_tos->use_ff = true;	/* a form feed is treated
						   much like a newline */
	  dump_line ();
	  parser_state_tos->want_blank = false;
	  break;

	case newline:
	  if (s_lab != e_lab && *s_lab == '#')
	    {
	      dump_line ();
	      if (s_code == e_code)
		parser_state_tos->want_blank = false;
	    }
	  else if (((parser_state_tos->last_token != comma
		     || !leave_comma || !break_comma
		     || parser_state_tos->p_l_follow > 0
		     || parser_state_tos->block_init
		     || s_com != e_com)
		    && ((parser_state_tos->last_token != rbrace || !btype_2
			 || !parser_state_tos->in_decl))))
	    {
	      /* Attempt to detect the newline before a procedure name,
	         and if e.g., K&R style, leave the procedure on the
	         same line as the type. */
	      if (!procnames_start_line
		  && parser_state_tos->last_token != lbrace
		  && parser_state_tos->last_token != semicolon
		  && parser_state_tos->last_rw == rw_decl
		  && parser_state_tos->last_rw_depth == 0
		  && !parser_state_tos->block_init
		  && parser_state_tos->in_decl)
		{
		  /* Put a space between the type and the procedure name,
		     unless it was a pointer type and the user doesn't
		     want such spaces after '*'. */
		  if (!(!space_after_pointer_type
			&& e_code > s_code
			&& e_code[-1] == '*'))
		    parser_state_tos->want_blank = true;
		}
	      else
		{
		  dump_line ();
		  parser_state_tos->want_blank = false;
		}
	    }
	  /* If we were on the line with a #else or a #endif, we aren't
	     anymore.  */
	  else_or_endif = false;
	  break;

	case lparen:
	  /* Braces in initializer lists should be put on new lines. This is
	     necessary so that -gnu does not cause things like char
	     *this_is_a_string_array[] = { "foo", "this_string_does_not_fit",
	     "nor_does_this_rather_long_string" } which is what happens
	     because we are trying to line the strings up with the
	     parentheses, and those that are too long are moved to the right
	     an ugly amount.

	     However, if the current line is empty, the left brace is
	     already on a new line, so don't molest it.  */
	  if (token[0] == L_CURL
	      && (s_code != e_code || s_com != e_com || s_lab != e_lab))
	    {
	      dump_line ();
	      /* Do not put a space before the left curly-brace.  */
	      parser_state_tos->want_blank = false;
	    }

	  /* Count parens so we know how deep we are.  */
	  if (++parser_state_tos->p_l_follow
	      >= (int) parser_state_tos->paren_indents_size)
	    {
	      parser_state_tos->paren_indents_size *= 2;
	      parser_state_tos->paren_indents = (int *)
		xrealloc ((char *) parser_state_tos->paren_indents,
			  parser_state_tos->paren_indents_size
			  * sizeof (int));
	    }
	  parser_state_tos->paren_depth++;
	  if (parser_state_tos->want_blank && *token != '['
	      && (parser_state_tos->last_token != ident || proc_calls_space
		  || (parser_state_tos->its_a_keyword
		      && (!parser_state_tos->sizeof_keyword
			  || blank_after_sizeof))))
	    {
	      buf_break = e_code;
	      *e_code++ = ' ';
	      *e_code = '\0';	/* null terminate code sect */
	    }

	  if (parser_state_tos->in_decl && !parser_state_tos->block_init)
	    {
	      if (*token != '[')
		{
		  while ((e_code - s_code) < dec_ind)
		    {
		      CHECK_CODE_SIZE;
		      buf_break = e_code;
		      *e_code++ = ' ';
		    }
		  *e_code++ = token[0];
		}
	      else
		*e_code++ = token[0];
	    }
	  else
	    *e_code++ = token[0];

	  parser_state_tos->paren_indents[parser_state_tos->p_l_follow - 1]
	    = (int) (e_code - s_code);
	  if (sp_sw && parser_state_tos->p_l_follow == 1
	      && extra_expression_indent
	      && parser_state_tos->paren_indents[0] < 2 * ind_size)
	    parser_state_tos->paren_indents[0] = 2 * ind_size;
	  parser_state_tos->want_blank = false;

	  if (parser_state_tos->in_or_st
	      && *token == '('
	      && parser_state_tos->tos <= 2)
	    {
	      /* this is a kludge to make sure that declarations will be
	         aligned right if proc decl has an explicit type on it, i.e.
	         "int a(x) {..." ("}" may be present) */
	      parse_lparen_in_decl ();

	      /* Turn off flag for structure decl or initialization.  */
	      parser_state_tos->in_or_st = false;
	    }
	  if (parser_state_tos->sizeof_keyword)
	    parser_state_tos->sizeof_mask |= 1 << parser_state_tos->p_l_follow;

	  /* The '(' that starts a cast can never be preceded by an
	     identifier or decl.  */
	  if (parser_state_tos->last_token == decl
	      || (parser_state_tos->last_token == ident
		  && parser_state_tos->last_rw != rw_return))
	    parser_state_tos->noncast_mask |=
	      1 << parser_state_tos->p_l_follow;
	  else
	    parser_state_tos->noncast_mask &=
	      ~(1 << parser_state_tos->p_l_follow);

	  break;

	case rparen:
	  parser_state_tos->paren_depth--;
	  if (parser_state_tos->cast_mask
	      & (1 << parser_state_tos->p_l_follow)
	      & ~parser_state_tos->sizeof_mask)
	    {
	      parser_state_tos->last_u_d = true;
	      parser_state_tos->cast_mask &=
		(1 << parser_state_tos->p_l_follow) - 1;
	      if (!parser_state_tos->cast_mask && cast_space)
		parser_state_tos->want_blank = true;
	      else
		parser_state_tos->want_blank = false;
	    }
	  else if (parser_state_tos->in_decl
		   && !parser_state_tos->block_init
		   && parser_state_tos->paren_depth == 0)
	    parser_state_tos->want_blank = true;

	  parser_state_tos->sizeof_mask
	    &= (1 << parser_state_tos->p_l_follow) - 1;
	  if (--parser_state_tos->p_l_follow < 0)
	    {
	      parser_state_tos->p_l_follow = 0;
	      if (!parser_state_tos->inner_stmt)
		message (token_col (), "Extra %c", (int) *token);
	    }

	  /* if the paren starts the line, then indent it */
	  if (e_code == s_code)
	    {
	      int level = parser_state_tos->p_l_follow;
	      parser_state_tos->paren_level = level;
	      if (level > 0)
		paren_target = -parser_state_tos->paren_indents[level - 1];
	      else
		paren_target = 0;
	    }
	  *e_code++ = token[0];

	  if (parser_state_tos->inner_stmt != 0
	      && parser_state_tos->inner_stmt > parser_state_tos->paren_depth)
	    {
	      parser_state_tos->inner_stmt = 0;
	      sp_sw = false;
	    }

	  /* check for end of if (...), or some such */
	  if (sp_sw && (parser_state_tos->p_l_follow == 0))
	    {

	      /* Indicate that we have just left the parenthesized expression
	         of a while, if, or for, unless we are getting out of the
	         parenthesized expression of the while of a do-while loop.
	         (do-while is different because a semicolon immediately
	         following this will not indicate a null loop body).  */
	      if (parser_state_tos->p_stack[parser_state_tos->tos]
		  != dohead)
		last_token_ends_sp = 2;
	      sp_sw = false;
	      force_nl = true;	/* must force newline after if */
	      parser_state_tos->last_u_d = true;	/* inform lexi that a
							   following operator is
							   unary */
	      parser_state_tos->in_stmt = false;	/* dont use statement
							   continuation
							   indentation */

	      PARSE (hd_type);	/* let parser worry about if, or whatever */
	    }
	  parser_state_tos->search_brace = btype_2;	/* this should insure
							   that constructs such
							   as main(){...} and
							   int[]{...} have their
							   braces put in the
							   right place */
	  break;

	case unary_op:		/* this could be any unary operation */

	  if (parser_state_tos->want_blank)

	    {
	      buf_break = e_code;
	      *e_code++ = ' ';
	      *e_code = '\0';	/* null terminate code sect */
	    }

	  {
	    char *res = token;
	    char *res_end = token_end;

	    /* if this is a unary op in a declaration, we should
	       indent this token */
	    if (parser_state_tos->paren_depth == 0
		&& parser_state_tos->in_decl
		&& !parser_state_tos->block_init)
	      {
		while ((e_code - s_code) < (dec_ind - (token_end - token)))
		  {
		    CHECK_CODE_SIZE;
		    buf_break = e_code;
		    *e_code++ = ' ';
		  }
	      }

	    for (t_ptr = res; t_ptr < res_end; ++t_ptr)
	      {
		CHECK_CODE_SIZE;
		*e_code++ = *t_ptr;
	      }

	    *e_code = '\0';	/* null terminate code sect */
	  }

	  parser_state_tos->want_blank = false;
	  break;

	case binary_op:	/* any binary operation */
	  if (parser_state_tos->want_blank
	      || (e_code > s_code && !isblank (*e_code)))
	    {
	      buf_break = e_code;
	      *e_code++ = ' ';
	      *e_code = '\0';	/* null terminate code sect */
	    }

	  if (indent_eqls > 0)
	    {
	      int need;
	      int have = (int) (e_code - s_code);

	      if (indent_eqls_1st <= 0)
		indent_eqls_1st = first_token_col ();
	      need = indent_eqls - indent_eqls_1st;

	      while (have < need)
		{
		  CHECK_CODE_SIZE;
		  *e_code++ = ' ';
		  ++have;
		}
	    }

	  {
	    char *res = token;
	    char *res_end = token_end;

	    for (t_ptr = res; t_ptr < res_end; ++t_ptr)
	      {
		CHECK_CODE_SIZE;
		*e_code++ = *t_ptr;	/* move the operator */
	      }
	  }
	  parser_state_tos->want_blank = true;
	  break;

	case postop:		/* got a trailing ++ or -- */
	  *e_code++ = token[0];
	  *e_code++ = token[1];
	  parser_state_tos->want_blank = true;
	  break;

	case question:		/* got a ? */
	  squest++;		/* this will be used when a later colon
				   appears so we can distinguish the
				   <c>?<n>:<n> construct */
	  if (parser_state_tos->want_blank)
	    {
	      buf_break = e_code;
	      *e_code++ = ' ';
	    }
	  *e_code++ = '?';
	  parser_state_tos->want_blank = true;
	  *e_code = '\0';	/* null terminate code sect */
	  break;

	case casestmt:		/* got word 'case' or 'default' */
	  scase = true;		/* so we can process the later colon
				   properly */
	  goto copy_id;

	case colon:		/* got a ':' */
	  if (squest > 0)
	    /* it is part of the <c> ? <n> : <n> construct */
	    {
	      --squest;
	      if (parser_state_tos->want_blank)
		{
		  buf_break = e_code;
		  *e_code++ = ' ';
		}
	      *e_code++ = ':';
	      *e_code = '\0';	/* null terminate code sect */
	      parser_state_tos->want_blank = true;
	      break;
	    }
	  if (parser_state_tos->in_decl)
	    {
	      *e_code++ = ':';
	      parser_state_tos->want_blank = false;
	      break;
	    }
	  parser_state_tos->in_stmt = false;	/* seeing a label does not
						   imply we are in a statement */
	  for (t_ptr = s_code; *t_ptr; ++t_ptr)
	    *e_lab++ = *t_ptr;	/* turn everything so far into a label */
	  e_code = s_code;
	  *e_lab++ = ':';
	  buf_break = e_code;
	  *e_lab++ = ' ';
	  *e_lab = '\0';
	  /* parser_state_tos->pcase will be used by dump_line to decide
	     how to indent the label. force_nl will force a case n: to be
	     on a line by itself */
	  force_nl = parser_state_tos->pcase = scase;
	  scase = false;
	  parser_state_tos->want_blank = false;
	  break;

	  /* Deal with C++ Class::Method */
	case doublecolon:
	  *e_code++ = ':';
	  *e_code++ = ':';
	  parser_state_tos->want_blank = false;
	  parser_state_tos->last_u_d = true;
	  break;

	case semicolon:
	  /* we are not in an initialization or structure declaration */
	  parser_state_tos->in_or_st = false;
	  scase = false;
	  squest = 0;
	  /* The following code doesn't seem to do much good. Just because
	     we've found something like extern int foo();    or int (*foo)();
	     doesn't mean we are out of a declaration.  Now if it was serving
	     some purpose we'll have to address that.... if
	     (parser_state_tos->last_token == rparen)
	     parser_state_tos->in_parameter_declaration = 0; */
	  parser_state_tos->cast_mask = 0;
	  parser_state_tos->sizeof_mask = 0;
	  parser_state_tos->block_init = 0;
	  parser_state_tos->block_init_level = 0;
	  parser_state_tos->just_saw_decl--;

	  if (parser_state_tos->in_decl
	      && s_code == e_code
	      && !parser_state_tos->block_init)
	    while ((e_code - s_code) < (dec_ind - 1))
	      {
		CHECK_CODE_SIZE;
		buf_break = e_code;
		*e_code++ = ' ';
	      }
	  *e_code = '\0';	/* null terminate code sect */

	  /* if we were in a first level structure declaration,
	     we aren't any more */
	  parser_state_tos->in_decl = (parser_state_tos->dec_nest > 0);

	  /* If we have a semicolon following an if, while, or for, and the
	     user wants us to, we should insert a space (to show that there
	     is a null statement there).  */
	  if (last_token_ends_sp && space_sp_semicolon)
	    {
	      buf_break = e_code;
	      *e_code++ = ' ';
	    }
	  *e_code++ = ';';
	  *e_code = '\0';	/* null terminate code sect */
	  parser_state_tos->want_blank = true;
	  /* we are no longer in the middle of a statement */
	  parser_state_tos->in_stmt = (parser_state_tos->p_l_follow > 0);

	  if (!sp_sw)
	    {			/* if not if for (;;) */
	      PARSE (semicolon);
	      force_nl = true;	/* force newline after a end of statement */
	    }
	  break;

	case lbrace:		/* got a left curly-brace */
	  if (parser_state_tos->paren_depth > 0)
	    {
	      /* FIXME: make this handle nested inner statements.  That would
	       * probably require saving the paren_depth values on a stack.
	       * For now, just detect/warn about this.
	       */
	      if (parser_state_tos->inner_stmt > 0
		  && parser_state_tos->inner_stmt > parser_state_tos->paren_depth)
		message (token_col (), "Nested inner statement");
	      parser_state_tos->inner_stmt = parser_state_tos->paren_depth;

	      /*
	       * Reset, so we'll format statement fragments within the curly
	       * braces.  But limit it to the following:
	       *          foo({statement})
	       *          foo(param,{statement})
	       * Otherwise we'll get into trouble with things like
	       *          foo(if(){something})
	       * by splitting the line at odd points.
	       */
	      if (last_code == lparen
		  || last_code == comma)
		parser_state_tos->p_l_follow = 0;
	    }

	  parser_state_tos->in_stmt = false;	/* dont indent the {} */
	  if (!parser_state_tos->block_init)
	    force_nl = true;	/* force other stuff on same line as left curly-brace onto
				   new line */
	  else if (parser_state_tos->block_init_level <= 0)
	    parser_state_tos->block_init_level = 1;
	  else
	    parser_state_tos->block_init_level++;

	  if (s_code != e_code && !parser_state_tos->block_init)
	    {
	      if (!btype_2)
		{
		  dump_line ();
		  parser_state_tos->want_blank = false;
		}
	      else
		{
		  if (parser_state_tos->in_parameter_declaration
		      && !parser_state_tos->in_or_st)
		    {
		      parser_state_tos->i_l_follow = 0;
		      dump_line ();
		      parser_state_tos->want_blank = false;
		    }
		  else
		    parser_state_tos->want_blank = true;
		}
	    }
	  if (parser_state_tos->in_parameter_declaration)
	    prefix_blankline_requested = 0;

	  if (s_code == e_code)
	    parser_state_tos->ind_stmt = false;		/* dont put extra indentation
							   on line with left curly-brace */
	  if (parser_state_tos->in_decl && parser_state_tos->in_or_st)
	    {
	      /* This is a structure declaration.  */
	      if (parser_state_tos->dec_nest >= (int) di_stack_alloc)
		{
		  di_stack_alloc *= 2;
		  di_stack = (int *)
		    xrealloc ((char *) di_stack,
			      di_stack_alloc * sizeof (*di_stack));
		}
	      di_stack[parser_state_tos->dec_nest++] = dec_ind;
	      /* ?              dec_ind = 0; */
	    }
	  else
	    {
	      parser_state_tos->in_decl = false;
	      parser_state_tos->decl_on_line = false;	/* we cant be in the
							   middle of a
							   declaration, so dont
							   do special
							   indentation of
							   comments */

	      parser_state_tos->in_parameter_declaration = 0;
	    }
	  dec_ind = 0;

	  /* We are no longer looking for an initializer or structure. Needed
	     so that the '=' in "enum bar {a = 1 ...}" does not get interpreted as
	     the start of an initializer.  */
	  parser_state_tos->in_or_st = false;

	  PARSE (lbrace);
	  if (last_code != lparen)
	    if (parser_state_tos->want_blank)	/* put a blank before left curly-brace if
						   left curly-brace is not at start of
						   line */
	      {
		buf_break = e_code;
		*e_code++ = ' ';
	      }

	  parser_state_tos->want_blank = false;
	  *e_code++ = L_CURL;
	  *e_code = '\0';	/* null terminate code sect */
	  parser_state_tos->just_saw_decl = 0;
	  break;

	case rbrace:		/* got a right curly-brace */
	  /* semicolons can be omitted in declarations */
	  if (parser_state_tos->p_stack[parser_state_tos->tos] == decl
	      && !parser_state_tos->block_init)
	    PARSE (semicolon);

	  parser_state_tos->just_saw_decl = 0;
	  parser_state_tos->block_init_level--;
	  if (s_code != e_code && !parser_state_tos->block_init)
	    {			/* right curly-brace must be first on line */
	      warn_broken_line (3);
	      dump_line ();
	    }
	  *e_code++ = R_CURL;
	  parser_state_tos->want_blank = true;
	  parser_state_tos->in_stmt = parser_state_tos->ind_stmt = false;
	  if (parser_state_tos->dec_nest > 0)
	    {			/* we are in multi-level structure
				   declaration */
	      dec_ind = di_stack[--parser_state_tos->dec_nest];
	      if (parser_state_tos->dec_nest == 0
		  && !parser_state_tos->in_parameter_declaration)
		parser_state_tos->just_saw_decl = 2;
	      parser_state_tos->in_decl = true;
	    }
	  prefix_blankline_requested = 0;
	  PARSE (rbrace);
	  parser_state_tos->search_brace
	    = (cuddle_else
	       && parser_state_tos->p_stack[parser_state_tos->tos] == ifhead)
	    || (cuddle_do_while
		&& (parser_state_tos->p_stack[parser_state_tos->tos] == dohead));

	  if ((parser_state_tos->p_stack[parser_state_tos->tos] == stmtl
	       && ((parser_state_tos->last_rw != rw_struct_like
		    && parser_state_tos->last_rw != rw_decl)
		   || !btype_2))
	      || (parser_state_tos->p_stack[parser_state_tos->tos] == ifhead)
	      || (parser_state_tos->p_stack[parser_state_tos->tos] == dohead
		  && !cuddle_do_while && !btype_2))
	    force_nl = true;
	  else if (parser_state_tos->tos <= 1
		   && blanklines_after_procs
		   && parser_state_tos->dec_nest <= 0)
	    postfix_blankline_requested = 1;
	  break;

	case swstmt:		/* got keyword "switch" */
	  sp_sw = true;
	  hd_type = swstmt;	/* keep this for when we have seen the
				   expression */
	  parser_state_tos->in_decl = false;
	  goto copy_id;		/* go move the token into buffer */

	case sp_paren:		/* token is if, while, for */
	  sp_sw = true;		/* the interesting stuff is done after the
				   expression is scanned */
	  hd_type = (*token == 'i' ? ifstmt :
		     (*token == 'w' ? whilestmt : forstmt));

	  /* remember the type of header for later use by parser */
	  goto copy_id;		/* copy the token into line */

	case sp_nparen:	/* got else, do */
	  parser_state_tos->in_stmt = false;
	  if (*token == 'e')
	    {
	      if (e_code != s_code && (!cuddle_else || e_code[-1] != R_CURL))
		{
		  warn_broken_line (4);
		  dump_line ();	/* make sure this starts a line */
		  parser_state_tos->want_blank = false;
		}
	      force_nl = true;	/* also, following stuff must go onto new
				   line */
	      last_else = 1;
	      PARSE (elselit);
	    }
	  else
	    {
	      if (e_code != s_code)
		{		/* make sure this starts a line */
		  warn_broken_line (5);
		  dump_line ();
		  parser_state_tos->want_blank = false;
		}
	      force_nl = true;	/* also, following stuff must go onto new
				   line */
	      last_else = 0;
	      PARSE (dolit);
	    }
	  goto copy_id;		/* move the token into line */

	  /* Handle C++ operator overloading like:

	     Class foo::operator = ()"

	     This is just like a decl, but we need to remember this
	     token type. */
	case overloaded:
	  if (parser_state_tos->want_blank)
	    {
	      buf_break = e_code;
	      *e_code++ = ' ';
	    }
	  parser_state_tos->want_blank = true;
	  for (t_ptr = token; t_ptr < token_end; ++t_ptr)
	    {
	      CHECK_CODE_SIZE;
	      *e_code++ = *t_ptr;
	    }
	  *e_code = '\0';	/* null terminate code sect */
	  break;

	case decl:		/* we have a declaration type (int, register,
				   etc.) */

	  /*
	   * This special case is related to the comma after a decl.
	   */
	  if (parser_state_tos->last_token == lparen
	      && (parser_state_tos->p_stack[parser_state_tos->tos] == elsehead)
	      && hd_type == ifstmt)
	    {
	      type_code = ident;
	      goto copy_id;
	    }

	  /* handle C++ const function declarations like
	     const MediaDomainList PVR::get_itsMediaDomainList() const
	     {
	     return itsMediaDomainList;
	     }
	     by ignoring "const" just after a parameter list */
	  if (parser_state_tos->last_token == rparen
	      && parser_state_tos->in_parameter_declaration
	      && (token_end - token) == 5
	      && !strncmp (token, "const", (size_t) 5))
	    {
	      buf_break = e_code;
	      *e_code++ = ' ';
	      for (t_ptr = token; t_ptr < token_end; ++t_ptr)
		{
		  CHECK_CODE_SIZE;
		  *e_code++ = *t_ptr;
		}
	      *e_code = '\0';	/* null terminate code sect */
	      break;
	    }

	  if (!parser_state_tos->sizeof_mask)
	    PARSE (decl);

	  if (parser_state_tos->last_token == rparen
	      && parser_state_tos->tos <= 1)
	    {
	      parser_state_tos->in_parameter_declaration = 1;
	      if (s_code != e_code)
		{
		  dump_line ();
		  parser_state_tos->want_blank = false;
		}
	    }
	  if (parser_state_tos->in_parameter_declaration
	      && indent_parameters
	      && parser_state_tos->dec_nest == 0
	      && parser_state_tos->p_l_follow == 0)
	    {
	      parser_state_tos->ind_level
		= parser_state_tos->i_l_follow = indent_parameters;
	      parser_state_tos->ind_stmt = 0;
	    }

	  /* in_or_st set for struct or initialization decl. Don't set it if
	     we're in ansi prototype */
	  if (!parser_state_tos->paren_depth)
	    parser_state_tos->in_or_st = true;

	  parser_state_tos->in_decl = parser_state_tos->decl_on_line = true;
	  if (parser_state_tos->dec_nest <= 0)
	    parser_state_tos->just_saw_decl = 2;
	  if (prefix_blankline_requested
	      && (parser_state_tos->block_init != 0
		  || parser_state_tos->block_init_level != -1
		  || parser_state_tos->last_token != rbrace
		  || e_code != s_code
		  || e_lab != s_lab
		  || e_com != s_com))
	    prefix_blankline_requested = 0;
	  i = (int) (token_end - token + 1);	/* get length of token plus 1 */

	  /* dec_ind = e_code - s_code + (parser_state_tos->decl_indent>i ?
	     parser_state_tos->decl_indent : i); */
	  dec_ind = decl_indent > 0 ? decl_indent : i;
	  goto copy_id;

	  /* Handle C++ operator overloading.  See case overloaded above. */
	case cpp_operator:

	case ident:		/* got an identifier or constant */
	  /* If we are in a declaration, we must indent identifier. But not
	     inside the parentheses of an ANSI function declaration.  */
	  if (parser_state_tos->in_decl
	      && parser_state_tos->p_l_follow == 0
	      && parser_state_tos->last_token != rbrace)
	    {
	      if (parser_state_tos->want_blank)
		{
		  buf_break = e_code;
		  *e_code++ = ' ';
		  *e_code = '\0';	/* null terminate code sect */
		}
	      parser_state_tos->want_blank = false;

	      if (is_procname_definition == 0 || !procnames_start_line)
		{
		  if (!parser_state_tos->block_init)
		    {
		      while ((e_code - s_code) < dec_ind)
			{
			  CHECK_CODE_SIZE;
			  buf_break = e_code;
			  *e_code++ = ' ';
			}
		      *e_code = '\0';	/* null terminate code sect */
		    }
		}
	      else
		{
		  if (dec_ind && s_code != e_code
		      && parser_state_tos->last_token != doublecolon)
		    dump_line ();
		  dec_ind = 0;
		  parser_state_tos->want_blank = false;
		}
	    }
	  else if (sp_sw && parser_state_tos->p_l_follow == 0)
	    {
	      sp_sw = false;
	      force_nl = true;
	      parser_state_tos->last_u_d = true;
	      parser_state_tos->in_stmt = false;
	      PARSE (hd_type);
	    }
	copy_id:
	  if (parser_state_tos->want_blank)
	    {
	      buf_break = e_code;
	      *e_code++ = ' ';
	    }

	  for (t_ptr = token; t_ptr < token_end; ++t_ptr)
	    {
	      CHECK_CODE_SIZE;
	      *e_code++ = *t_ptr;
	    }
	  *e_code = '\0';	/* null terminate code sect */

	  parser_state_tos->want_blank = true;

	  /* If the token is va_dcl, it appears without a semicolon, so we
	     need to pretend that one was there.  */
	  if ((token_end - token) == 6
	      && strncmp (token, "va_dcl", (size_t) 6) == 0)
	    {
	      parser_state_tos->in_or_st = false;
	      parser_state_tos->just_saw_decl--;
	      parser_state_tos->in_decl = 0;
	      PARSE (semicolon);
	      force_nl = true;
	    }
	  break;

	case struct_delim:
	  for (t_ptr = token; t_ptr < token_end; ++t_ptr)
	    {
	      CHECK_CODE_SIZE;
	      *e_code++ = *t_ptr;
	    }

	  parser_state_tos->want_blank = false;		/* dont put a blank after a
							   period */
	  break;

	case comma:
	  /* check for type passed as parameter */
	  CHECK_LAST_DECL;
	  /* only put blank after comma if comma does not start the line */
	  parser_state_tos->want_blank = (s_code != e_code);
	  if (parser_state_tos->paren_depth == 0
	      && parser_state_tos->in_decl
	      && is_procname_definition == 0
	      && !parser_state_tos->block_init)
	    while ((e_code - s_code) < (dec_ind - 1))
	      {
		CHECK_CODE_SIZE;
		buf_break = e_code;
		*e_code++ = ' ';
	      }

	  *e_code++ = ',';
	  if (parser_state_tos->p_l_follow == 0)
	    {
	      if (parser_state_tos->block_init_level <= 0)
		parser_state_tos->block_init = 0;
	      /* If we are in a declaration, and either the user wants all
	         comma'd declarations broken, or the line is getting too
	         long, break the line.  */
	      if (break_comma && !leave_comma)
		force_nl = true;
	    }
	  break;

	case preesc:		/* got the character '#' */
	  if ((s_com != e_com) ||
	      (s_lab != e_lab) ||
	      (s_code != e_code))
	    dump_line ();
	  {
	    int in_comment = 0;
	    int in_cplus_comment = 0;
	    int com_start = 0;
	    char quote = 0;
	    int com_end = 0;

	    /* ANSI allows spaces between '#' and preprocessor directives.
	       If the user specified "-lps" and there are such spaces,
	       they will be part of `token', otherwise `token' is just
	       '#'. */
	    for (t_ptr = token; t_ptr < token_end; ++t_ptr)
	      {
		CHECK_LAB_SIZE;
		*e_lab++ = *t_ptr;
	      }

	    while (!had_eof && (!at_line_end (buf_ptr) || in_comment))
	      {
		CHECK_LAB_SIZE;
		*e_lab = *buf_ptr++;
		if (at_buffer_end (buf_ptr))
		  fill_buffer ();

		switch (*e_lab++)
		  {
		  case BACKSLASH:
		    if (!in_comment && !in_cplus_comment)
		      {
			*e_lab++ = *buf_ptr++;
			if (at_buffer_end (buf_ptr))
			  fill_buffer ();
		      }
		    break;

		  case '/':
		    if ((*buf_ptr == '*' || *buf_ptr == '/')
			&& !in_comment && !in_cplus_comment && !quote)
		      {
			save_com.column = current_column () - 1;
			if (*buf_ptr == '/')
			  in_cplus_comment = 1;
			else
			  in_comment = 1;
			*e_lab++ = *buf_ptr++;
			com_start = (int) (e_lab - s_lab - 2);
		      }
		    break;

		  case '"':
		  case '\'':
		    if (!quote)
		      quote = e_lab[-1];
		    else if (e_lab[-1] == quote)
		      quote = 0;
		    break;

		  case '*':
		    if (*buf_ptr == '/' && in_comment)
		      {
			in_comment = 0;
			*e_lab++ = *buf_ptr++;
			com_end = (int) (e_lab - s_lab);
		      }
		    break;
		  }
	      }

	    while (e_lab > s_lab && isblank (e_lab[-1]))
	      e_lab--;

	    if (in_cplus_comment)	/* Should we also check in_comment? -jla */
	      {
		*e_lab++ = *buf_ptr++;
		com_end = (int) (e_lab - s_lab);
	      }

	    if (e_lab - s_lab == com_end && bp_save == 0)
	      {			/* comment on preprocessor line */
		size_t added;

		if (save_com.end != save_com.ptr)
		  {
		    need_chars (&save_com, (size_t) 2);
		    *save_com.end++ = EOL;	/* add newline between
						   comments */
		    *save_com.end++ = ' ';
		    save_com.len += 2;
		  }
		added = (size_t) (com_end - com_start);
		need_chars (&save_com, added + 1);
		strncpy (save_com.end, s_lab + com_start, added);
		save_com.end[added] = '\0';
		save_com.end += added;
		save_com.len += added;

		e_lab = s_lab + com_start;
		while (e_lab > s_lab
		       && isblank (e_lab[-1]))
		  e_lab--;

		/* Switch input buffers so that calls to lexi() will
		   read from our save buffer. */
		bp_save = buf_ptr;
		be_save = buf_end;
		buf_ptr = save_com.ptr;
		need_chars (&save_com, (size_t) 1);
		buf_end = save_com.end;
		save_com.end = save_com.ptr;	/* make save_com empty */
	      }
	    *e_lab = '\0';	/* null terminate line */
	    parser_state_tos->pcase = false;
	  }

	  /*
	   * s_lab points to the "#" mark, but there may be spaces between it
	   * and the keyword, if any.
	   */
	  for (s_key = s_lab + 1;
	       s_key < e_lab && isspace (UChar (*s_key));
	       ++s_key)
	    {
	      ;
	    }
	  if (s_key < e_lab)
	    {
	      for (e_key = s_key + 1;
		   e_key < e_lab && isName (UChar (*e_key));
		   ++e_key)
		{
		  ;
		}
	    }
	  else
	    {
	      s_key = e_lab;
	      e_key = e_lab;
	    }

#define SAME_KEY(name,len) \
	  ((e_key - s_key) == len && strncmp (s_key, name, (size_t) len) == 0)

	  if (SAME_KEY ("if", 2)
	      || SAME_KEY ("ifdef", 5)
	      || SAME_KEY ("ifndef", 6))
	    {
	      if (blanklines_around_conditional_compilation)
		{
		  prefix_blankline_requested++;
		  while (*in_prog_pos++ == EOL)
		    {
		      ;
		    }
		  in_prog_pos--;
		}
	      {
		/* Push a copy of the parser_state onto the stack. All
		   manipulations will use the copy at the top of stack, and
		   then we can return to the previous state by popping the
		   stack.  */
		struct parser_state *new;

		new = (struct parser_state *)
		  xmalloc (sizeof (struct parser_state));
		(void) memcpy (new, parser_state_tos, sizeof (struct parser_state));

		/* We need to copy the dynamically allocated arrays in the
		   struct parser_state too.  */
		new->p_stack = (enum codes *)
		  xmalloc (parser_state_tos->p_stack_size
			   * sizeof (enum codes));
		(void) memcpy (new->p_stack, parser_state_tos->p_stack,
			       (parser_state_tos->p_stack_size
				* sizeof (enum codes)));

		new->il = (int *)
		  xmalloc (parser_state_tos->p_stack_size * sizeof (int));
		(void) memcpy (new->il, parser_state_tos->il,
			       parser_state_tos->p_stack_size * sizeof (int));

		new->cstk = (int *)
		  xmalloc (parser_state_tos->p_stack_size
			   * sizeof (int));
		(void) memcpy (new->cstk, parser_state_tos->cstk,
			       parser_state_tos->p_stack_size * sizeof (int));

		new->paren_indents = (int *) xmalloc
		  (parser_state_tos->paren_indents_size * sizeof (int));
		(void) memcpy (new->paren_indents,
			       parser_state_tos->paren_indents,
			       (parser_state_tos->paren_indents_size
				* sizeof (int)));

		new->next = parser_state_tos;
		parser_state_tos->preprocessor_indent = true;
		parser_state_tos = new;
	      }
	    }
	  else if (SAME_KEY ("else", 4) || SAME_KEY ("elif", 4))
	    {
	      /* When we get #else, we want to restore the parser state to
	         what it was before the matching #if, so that things get
	         lined up with the code before the #if.  However, we do not
	         want to pop the stack; we just want to copy the second to
	         top elt of the stack because when we encounter the #endif,
	         it will pop the stack.  */
	      else_or_endif = true;
	      if (parser_state_tos->next)
		{
		  /* First save the addresses of the arrays for the top of
		     stack.  */
		  enum codes *tos_p_stack = parser_state_tos->p_stack;
		  int *tos_il = parser_state_tos->il;
		  int *tos_cstk = parser_state_tos->cstk;
		  int *tos_paren_indents = parser_state_tos->paren_indents;
		  struct parser_state *second = parser_state_tos->next;

		  (void) memcpy (parser_state_tos, second,
				 sizeof (struct parser_state));
		  parser_state_tos->next = second;

		  /* Now copy the arrays from the second to top of stack to
		     the top of stack.  */
		  /* Since the p_stack, etc. arrays only grow, never shrink,
		     we know that they will be big enough to fit the array
		     from the second to top of stack.  */
		  parser_state_tos->p_stack = tos_p_stack;
		  (void) memcpy (parser_state_tos->p_stack,
				 parser_state_tos->next->p_stack,
				 parser_state_tos->p_stack_size
				 * sizeof (enum codes));

		  parser_state_tos->il = tos_il;
		  (void) memcpy (parser_state_tos->il,
				 parser_state_tos->next->il,
				 (parser_state_tos->p_stack_size
				  * sizeof (int)));

		  parser_state_tos->cstk = tos_cstk;
		  (void) memcpy (parser_state_tos->cstk,
				 parser_state_tos->next->cstk,
				 (parser_state_tos->p_stack_size
				  * sizeof (int)));

		  parser_state_tos->paren_indents = tos_paren_indents;
		  (void) memcpy (parser_state_tos->paren_indents,
				 parser_state_tos->next->paren_indents,
				 (parser_state_tos->paren_indents_size
				  * sizeof (int)));

		  parser_state_tos->preprocessor_indent = false;
		}
	      else
		{
		  message (0, "Unmatched #%.*s", e_key - s_key, s_key);
		  file_exit_value = indent_error;
		}
	    }
	  else if (SAME_KEY ("endif", 5))
	    {
	      else_or_endif = true;
	      /* We want to remove the second to top elt on the stack, which
	         was put there by #if and was used to restore the stack at
	         the #else (if there was one). We want to leave the top of
	         stack unmolested so that the state which we have been using
	         is unchanged.  */
	      if (parser_state_tos->next)
		{
		  struct parser_state *second = parser_state_tos->next;

		  parser_state_tos->next = second->next;
		  free (second->p_stack);
		  free (second->il);
		  free (second->cstk);
		  free (second->paren_indents);
		  free (second);
		}
	      else
		{
		  message (0, "Unmatched #endif");
		  file_exit_value = indent_error;
		}

	      if (blanklines_around_conditional_compilation)
		{
		  postfix_blankline_requested++;
		  n_real_blanklines = 0;
		}
	      else
		prefix_blankline_requested = 0;
	    }

	  /* Normally, subsequent processing of the newline character
	     causes the line to be printed.  The following clause handles
	     a special case (comma-separated declarations separated
	     by the preprocessor lines) where this doesn't happen. */
	  if (parser_state_tos->last_token == comma
	      && parser_state_tos->p_l_follow <= 0
	      && leave_comma && !parser_state_tos->block_init
	      && break_comma && s_com == e_com)
	    {
	      dump_line ();
	      parser_state_tos->want_blank = false;
	    }
	  break;

	  /* A C or C++ comment. */
	case comment:
	case cplus_comment:
	  parser_state_tos->in_comment = true;
	  if (flushed_nl)
	    {
	      dump_line ();
	      parser_state_tos->want_blank = false;
	      force_nl = false;
	    }
	  print_comment ();
	  parser_state_tos->in_comment = false;
	  break;

	default:
	  abort ();
	}			/* end of big switch statement */

      *e_code = '\0';		/* make sure code section is null terminated */
      if (type_code != comment
	  && type_code != cplus_comment
	  && type_code != newline
	  && type_code != preesc
	  && type_code != form_feed)
	parser_state_tos->last_token = type_code;

      /*  Now that we've put the token on the line (in most cases),
         consider breaking the line because it's too long.

         Don't consider the cases of `unary_op', newlines,
         declaration types (int, etc.), if, while, for,
         identifiers (handled at the beginning of the loop),
         periods, or preprocessor commands. */
      if (max_col > 0 && buf_break != 0)
	{
	  if (((type_code == binary_op)
	       || (type_code == postop)
	       || (type_code == question)
	       || (type_code == colon && (scase || squest <= 0))
	       || (type_code == semicolon)
	       || (type_code == sp_nparen)
	       || (type_code == ident && *token == '\"')
	       || (type_code == comma))
	      && output_line_length () > max_col)
	    {
	      break_line = 1;
	      force_nl = true;
	    }
	}

    }				/* end of main while (1) loop */
}

const char *progname = 0;	/* actual name of this program */

/* Points to current input file name */
const char *in_name = 0;

/* Points to the current input buffer */
struct file_buffer *current_input = 0;

/* Points to the name of the output file */
static const char *out_name = 0;

/* How many input files were specified */
static size_t input_files;

/* Names of all input files */
static const char **in_file_names;

/* Initial number of input filenames to allocate. */
static size_t max_input_files = 128;

/* Some operating systems don't allow more than one dot in a filename.  */
#if defined (ONE_DOT_PER_FILENAME)
#define INDENT_PROFILE "indent.pro"
#else
#define INDENT_PROFILE ".indent.pro"
#endif

static void
reset_lex (void)
{
  lex_section = 0;
  next_lexcode = 0;
}

int
main (int argc, char **argv)
{
  int i;
  char *profile_pathname = 0;
  int using_stdin = false;
  enum exit_values exit_status;
  const char *profile_filename = INDENT_PROFILE;

  if ((progname = strrchr (argv[0], '/')) != 0)
    ++progname;
  else
    progname = argv[0];

  init_lexi ();
  init_parser ();
  initialize_backups ();
  exit_status = total_success;

  output = 0;
  input_files = 0;
  in_file_names = (const char **) xmalloc (max_input_files * sizeof (char *));

  set_defaults ();
  for (i = 1; i < argc; ++i)
    {
      char *param = argv[i];

      if (strcmp (param, "-c++") == 0
	  || strcmp (param, "--c-plus-plus") == 0)
	c_plus_plus = 1;

      if (strcmp (param, "-npro") == 0
	  || strcmp (param, "--ignore-profile") == 0
	  || strcmp (param, "+ignore-profile") == 0)
	break;
      if (strcmp (param, "--profile") == 0)
	{
	  profile_filename = argv[++i];
	  if (profile_filename == 0)
	    usage ();
	}
    }
  if (i >= argc)
    profile_pathname = set_profile (profile_filename);

  for (i = 1; i < argc; ++i)
    {
      char *param = argv[i];

      if (param[0] != '-' && param[0] != '+')	/* Filename */
	{
	  if (expect_output_file == true)	/* Last arg was "-o" */
	    {
	      if (out_name != 0)
		{
		  fprintf (stderr,
			   "%s: only one output file (2nd was %s)\n",
			   progname,
			   param);
		  exit (invocation_error);
		}

	      if (input_files > 1)
		{
		  fprintf (stderr,
			   "%s: only one input file when output file is specified\n",
			   progname);
		  exit (invocation_error);
		}

	      out_name = param;
	      expect_output_file = false;
	      continue;
	    }
	  else
	    {
	      if (using_stdin)
		{
		  fprintf (stderr,
			   "%s: can't have filenames when specifying standard input\n",
			   progname);
		  exit (invocation_error);
		}

	      input_files++;
	      if (input_files > 1)
		{
		  if (out_name != 0)
		    {
		      fprintf (stderr,
			       "%s: only one input file when output file is specified\n",
			       progname);
		      exit (invocation_error);
		    }

		  if (use_stdout != 0)
		    {
		      fprintf (stderr,
			       "%s: only one input file when stdout is used\n",
			       progname);
		      exit (invocation_error);
		    }

		  if (input_files > max_input_files)
		    {
		      max_input_files = 2 * max_input_files;
		      in_file_names
			= (const char **) xrealloc ((char *) in_file_names,
						    (max_input_files
						     * sizeof (char *)));
		    }
		}

	      in_file_names[input_files - 1] = param;
	    }
	}
      else
	{
	  /* '-' as filename means stdin. */
	  if (param[0] == '-' && param[1] == '\0')
	    {
	      if (input_files > 0)
		{
		  fprintf (stderr,
			   "%s: can't have filenames when specifying standard input\n",
			   progname);
		  exit (invocation_error);
		}

	      using_stdin = true;
	    }
	  else if (strcmp (param, "--profile") == 0)
	    {
	      ++i;
	    }
	  else
	    i += set_option (param, (i < argc ? argv[i + 1] : 0), 1);
	}
    }

  if (verbose && profile_pathname)
    fprintf (stderr, "Read profile %s\n", profile_pathname);
  if (debug)
    print_options ();

  if (input_files > 1)
    {
      /* When multiple input files are specified, make a backup copy
         and then output the indented code into the same filename. */

      for (i = 0; input_files; i++, input_files--)
	{
	  enum exit_values status;

	  in_name = out_name = in_file_names[i];
	  current_input = read_file (in_file_names[i]);
	  output = fopen (out_name, "w");
	  if (output == 0)
	    {
	      fprintf (stderr, "%s: can't create %s\n", progname, out_name);
	      exit (indent_fatal);
	    }

	  make_backup (current_input);
	  reset_lex ();
	  reset_parser ();
	  status = indent (current_input);
	  if (status > exit_status)
	    exit_status = status;
	  if (fclose (output) != 0)
	    fatal ("Can't close output file %s", out_name);
	}
    }
  else
    {
      /* One input stream -- specified file, or stdin */

      if (input_files == 0 || using_stdin)
	{
	  input_files = 1;
	  in_file_names[0] = "Standard input";
	  in_name = in_file_names[0];
	  current_input = read_stdin ();
	}
      else
	/* 1 input file */
	{
	  in_name = in_file_names[0];
	  current_input = read_file (in_file_names[0]);
	  if (!out_name && !use_stdout)
	    {
	      out_name = in_file_names[0];
	      make_backup (current_input);
	    }
	}

      /* Use stdout if it was specified ("-st"), or neither input
         nor output file was specified. */
      if (use_stdout || !out_name)
	output = stdout;
      else
	{
	  output = fopen (out_name, "w");
	  if (output == 0)
	    {
	      fprintf (stderr, "%s: can't create %s\n", progname, out_name);
	      exit (invocation_error);
	    }
	}

      reset_lex ();
      reset_parser ();
      exit_status = indent (current_input);
    }

  exit ((int) exit_status);

#ifdef _WIN32
  return 0;
#endif
}
