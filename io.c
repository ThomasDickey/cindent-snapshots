/*
   Copyright 1999-2020,2022, Thomas E. Dickey

   Copyright (c) 1994, Joseph Arceneaux.  All rights reserved.

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
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. */


#include "sys.h"
#include "indent.h"

#ifdef VMS
#   include <file.h>
#   include <types.h>
#   include <stat.h>
#else /* not VMS */

#include <sys/types.h>
#include <sys/stat.h>
/* POSIX says that <fcntl.h> should exist.  Some systems might need to use
   <sys/fcntl.h> or <sys/file.h> instead.  */
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#endif

#endif /* not VMS */

/* number of levels a label is placed to left of code */
#define LABEL_OFFSET 2

#define beginning_comment(p) ((p)[0] == '/' && ((p)[1] == '*' || ((p)[1]) == '/'))

/* Stuff that needs to be shared with the rest of indent. Documented in
   indent.h.  */
char *in_prog;
char *in_prog_pos;
char *cur_line;
unsigned long in_prog_size;
FILE *output;
char *buf_ptr;
char *buf_end;
char *buf_break;
int break_line;
int had_eof;
int out_lines;
int com_lines;
int indent_eqls;
int indent_eqls_1st;

int suppress_blanklines = 0;
static int suppress_formatting = 0;
static int not_first_line;

int paren_target;

#ifdef VMS
/* Folks say VMS requires its own read routine.  Then again, some folks
   say it doesn't.  Different folks have also sent me conflicting versions
   of this function.  Who's right?

   Anyway, this version was sent by MEHRDAD@glum.dev.cf.ac.uk and modified
   slightly by me. */

int
vms_read (int file_desc, char *buffer, int nbytes)
{
  char *bufp;
  int nread, nleft;

  bufp = buffer;
  nread = 0;
  nleft = nbytes;

  nread = read (file_desc, bufp, nleft);
  while (nread > 0)
    {
      bufp += nread;
      nleft -= nread;
      if (nleft < 0)
	fatal ("Internal buffering error");
      nread = read (file_desc, bufp, nleft);
    }

  return nbytes - nleft;
}
#endif /* VMS */

static void
pass_char (int ch)
{
  if (ch == '\n')
    {
      ++out_line_no;
      out_column_no = 0;
    }
  else if (ch == TAB)
    {
      int target_column = out_column_no + tabsize - (out_column_no % tabsize);
      if (use_tabs || suppress_formatting)
	{
	  out_column_no = target_column;
	}
      else
	{
	  ch = ' ';
	  while (out_column_no < target_column - 1)
	    {
	      putc (ch, output);
	      ++out_column_no;
	    }
	  ++out_column_no;
	}
    }
  else
    {
      ++out_column_no;
    }
  putc (ch, output);
}

static void
pass_text (const char *s)
{
  while (*s != 0)
    pass_char (*s++);
}

#define at_current_eof(p) \
	((unsigned) ((p) - current_input->data) == current_input->size)

static char *
pass_line (char *p)
{
  while (*p != '\0' && *p != EOL)
    pass_char (*p++);
  if (*p == EOL)
    {
      pass_char ('\n');
      ++in_line_no;
      ++p;
      in_prog_pos = p;
      cur_line = p;
    }
  else if (*p == '\0' && at_current_eof (p))
    {
      buf_ptr = buf_end = in_prog_pos = p;
      had_eof = true;
      p = 0;
    }
  return p;
}

static void
pass_n_text (const char *s, int n)
{
  while (n-- > 0)
    pass_char (*s++);
}

static void
reset_output (void)
{
  reset_parser ();
  not_first_line = 0;
  n_real_blanklines = 0;
  suppress_blanklines = 1;
}

int
count_columns (
		int column,
		char *bp,
		int stop_char)
{
  while (*bp != stop_char && *bp != NULL_CHAR)
    {
      switch (*bp++)
	{
	case EOL:
	case FORM_FEED:	/* form feed */
	  column = 1;
	  break;
	case TAB:
	  column += tabsize - (column - 1) % tabsize;
	  break;
	case 010:		/* backspace */
	  --column;
	  break;
	default:
	  ++column;
	  break;
	}
    }

  return column;
}

static int
this_column (char *from, char *to, int column)
{
  while (from < to)
    {
      switch (*from)
	{
	case EOL:
	case FORM_FEED:	/* form feed */
	  column = 1;
	  break;

	case TAB:
	  column += tabsize - (column - 1) % tabsize;
	  break;

	case '\b':		/* backspace */
	  column--;
	  break;

	default:
	  column++;
	  break;
	}

      from++;
    }
  return column;
}

/* Return the column we are at in the input line. */

int
current_column (void)
{
  char *p;
  int column;

  if (buf_ptr >= save_com.ptr && buf_ptr <= save_com.ptr + save_com.len)
    {
      p = save_com.ptr;
      column = save_com.column;
    }
  else
    {
      p = cur_line;
      column = 1;
    }

  return this_column (p, buf_ptr, column);
}

/* Fill the output line with whitespace up to TARGET_COLUMN, given that
   the line is currently in column CURRENT_COLUMN.  Returns the ending
   column. */

static int
pad_output (
	     int my_current_col,
	     int target_column)
{
  if (my_current_col < target_column)
    {
      if (tabsize > 1)
	{
	  int offset;

	  offset = tabsize - (my_current_col - 1) % tabsize;
	  while (my_current_col + offset <= target_column)
	    {
	      pass_char (TAB);
	      my_current_col += offset;
	      offset = tabsize;
	    }
	}

      while (my_current_col < target_column)
	{
	  pass_char (' ');
	  my_current_col++;
	}
    }
  return my_current_col;
}

static int
preprocessor_level (struct parser_state *p)
{
  int level = 0;
  while (p != 0)
    {
      if (p->preprocessor_indent)
	++level;
      p = p->next;
    }
  return level;
}

static void
drain_blanklines (void)
{
  if (debug > 2)
    {
      printf ("drain_blanklines:");
      printf (" bl_line:%d, ", parser_state_tos->bl_line);
      printf (" request:%d, ", prefix_blankline_requested);
      printf (" atfirst:%d, ", not_first_line);
      printf (" swallow:%d, ", swallow_optional_blanklines);
      printf (" actual:%d, ", n_real_blanklines);
      printf ("\n");
    }
  suppress_blanklines = 0;
  parser_state_tos->bl_line = false;
  if (prefix_blankline_requested
      && not_first_line
      && n_real_blanklines == 0)
    n_real_blanklines = 1;
  else if (swallow_optional_blanklines && n_real_blanklines > 1)
    n_real_blanklines = 1;

  while (--n_real_blanklines >= 0)
    pass_char (EOL);
  n_real_blanklines = 0;
}

void
dump_line (void)
{				/* dump_line is the routine that actually
				   effects the printing of the new source. It
				   prints the label section, followed by the
				   code section with the appropriate nesting
				   level, followed by any comments */
  int not_truncated = 1;

  if (parser_state_tos->procname[0])
    {
      parser_state_tos->ind_level = 0;
      parser_state_tos->procname = "\0";
    }

  /* A blank line */
  if (s_code == e_code && s_lab == e_lab && s_com == e_com)
    {
      /* If we have a formfeed on a blank line, we should just output it,
         rather than treat it as a normal blank line.  */
      if (parser_state_tos->use_ff)
	{
	  pass_char (FORM_FEED);
	  parser_state_tos->use_ff = false;
	}
      else if (!suppress_blanklines)
	{
	  parser_state_tos->bl_line = true;
	  n_real_blanklines++;
	}
    }
  else
    {
      int cur_col;

      drain_blanklines ();

      if (parser_state_tos->ind_level == 0)
	parser_state_tos->ind_stmt = 0;		/* This is a class A kludge. Don't do
						   additional statement indentation
						   if we are at bracket level 0 */

      if (e_lab != s_lab || e_code != s_code)
	++code_lines;		/* keep count of lines with code */

      if (e_lab != s_lab)
	{			/* print lab, if any */
	  int label_target = compute_label_target ();
	  char *skip_pound = s_lab;
	  char *s_key;

	  while (e_lab > s_lab && isblank (e_lab[-1]))
	    e_lab--;

	  cur_col = 1;
	  if (*skip_pound == '#')
	    {
	      pass_char (*skip_pound++);
	      ++cur_col;

	      for (s_key = skip_pound; s_key < e_lab && isblank (*s_key); ++s_key);

	      if (s_key >= e_lab)
		{
		  s_key = 0;
		}
	      else if (preprocessor_indentation)
		{
		  int adj = (!strncmp (s_key, "if", (size_t) 2) ||
			     !strncmp (s_key, "el", (size_t) 2)) ? 1 : 0;
		  int pad_preproc = ((preprocessor_level (parser_state_tos)
				      - adj)
				     * preprocessor_indentation);
		  cur_col = pad_output (cur_col, cur_col + pad_preproc);
		}
	    }
	  else
	    {
	      s_key = 0;
	    }
	  cur_col = pad_output (cur_col, label_target);
	  if (s_key != 0
	      && (strncmp (s_key, "else", (size_t) 4) == 0
		  || strncmp (s_key, "endif", (size_t) 5) == 0))
	    {
	      /* Treat #else and #endif as a special case because any text
	         after #else or #endif should be converted to a comment.  */
	      char *s = skip_pound;

	      if (e_lab[-1] == EOL)
		e_lab--;
	      do
		{
		  /* skip 's' past the end of "endif" */
		  pass_char (*s++);
		}
	      while ((s < s_key) || (s < e_lab && 'a' <= *s && *s <= 'z'));
	      cur_col = this_column (skip_pound, s, cur_col);
	      /* skip whitespace after "endif" */
	      while (isblank (*s) && s < e_lab)
		s++;
	      if (s < e_lab)
		{
		  int len = (int) (e_lab - s);

		  if (cur_col < else_endif_col)
		    {
		      (void) pad_output (cur_col, else_endif_col);
		    }
		  else
		    {
		      pass_char (' ');
		      ++cur_col;
		    }
		  if (beginning_comment (s))
		    {
		      pass_n_text (s, len);
		    }
		  else
		    {
		      pass_text ("/* ");
		      pass_n_text (s, len);
		      pass_text (" */");
		      len += 2;
		    }
		  cur_col += len;
		}
	    }
	  else
	    {
	      pass_n_text (skip_pound, (int) (e_lab - skip_pound));
	      cur_col = count_columns (cur_col, skip_pound, NULL_CHAR);
	    }
	}
      else
	cur_col = 1;		/* there is no label section */

      parser_state_tos->pcase = false;

      /* Remove trailing spaces */
      while (isblank (*(e_code - 1)) && e_code > s_code)
	*(--e_code) = NULL_CHAR;
      if (s_code != e_code)
	{			/* print code section, if any */
	  char *p;
	  int i;
	  int target_col = 0;

	  /* If a comment begins this line, then indent it to the right
	     column for comments, otherwise the line starts with code,
	     so indent it for code. */
	  if (embedded_comment_on_line == 1)
	    target_col = parser_state_tos->com_col;
	  else
	    target_col = compute_code_target ();

	  /* If a line ends in an lparen character, the following line should
	     not line up with the parenthesis, but should be indented by the
	     usual amount.  */
	  if (parser_state_tos->last_token == lparen)
	    {
	      parser_state_tos->paren_indents[parser_state_tos->p_l_follow - 1]
		+= ind_size - 1;
	    }

	  for (i = 0; i < parser_state_tos->p_l_follow; i++)
	    if (parser_state_tos->paren_indents[i] >= 0)
	      parser_state_tos->paren_indents[i]
		= -(parser_state_tos->paren_indents[i] + target_col);

	  cur_col = pad_output (cur_col, target_col);

	  if (break_line
	      && s_com == e_com
	      && buf_break > s_code && buf_break < e_code - 1)
	    {
	      size_t len;

	      for (p = s_code; p < buf_break; p++)
		pass_char (*p);

	      *buf_break = '\0';
	      cur_col = count_columns (cur_col, s_code, NULL_CHAR);

	      not_truncated = 0;
	      len = (size_t) (e_code - buf_break - 1);
	      memmove (s_code, buf_break + 1, len);
	      e_code = s_code + len;
	      buf_break = e_code;
	      *e_code = '\0';
	      break_line = 0;
	    }
	  else
	    {
	      for (p = s_code; p < e_code; p++)
		pass_char (*p);
	      cur_col = count_columns (cur_col, s_code, NULL_CHAR);
	    }
	}

      if (s_com != e_com)
	{
	  {
	    /* Here for comment printing.  This code is new as of
	       version 1.8 */
	    int target = parser_state_tos->com_col;
	    char *com_st = s_com;

	    if ((cur_col > target) && (*s_lab != '#'))
	      {
		pass_char (EOL);
		cur_col = 1;
		++out_lines;
	      }
	    else if ((cur_col == target)
		     && (cur_col > 1)
		     && (s_code != e_code || s_lab != e_lab))
	      {
		pass_char (' ');
		++cur_col;
	      }
	    else
	      {
		(void) pad_output (cur_col, target);
	      }
	    pass_n_text (com_st, (int) (e_com - com_st));
	    com_lines++;
	  }
	}
      else if (embedded_comment_on_line)
	com_lines++;
      embedded_comment_on_line = 0;

      if (parser_state_tos->use_ff)
	{
	  pass_char (FORM_FEED);
	  parser_state_tos->use_ff = false;
	}
      else
	pass_char (EOL);

      ++out_lines;
      if (parser_state_tos->just_saw_decl == 1
	  && blanklines_after_declarations)
	{
	  if (!parser_state_tos->in_comment)
	    prefix_blankline_requested = 1;
	  parser_state_tos->just_saw_decl = 0;
	}
      else
	prefix_blankline_requested = postfix_blankline_requested;
      postfix_blankline_requested = 0;
    }

  /* if we are in the middle of a declaration, remember that fact
     for proper comment indentation */
  parser_state_tos->decl_on_line = parser_state_tos->in_decl;

  /* next line should be indented if we have not completed this
     stmt and if we are not in the middle of a declaration */
  parser_state_tos->ind_stmt = (parser_state_tos->in_stmt
				& ~parser_state_tos->in_decl);

  parser_state_tos->dumped_decl_indent = 0;
  *(e_lab = s_lab) = '\0';	/* reset buffers */
  if (not_truncated)
    {
      *(e_code = s_code) = '\0';
    }

  break_line = 0;
  buf_break = NULL;

  *(e_com = s_com) = '\0';
  parser_state_tos->ind_level = parser_state_tos->i_l_follow;
  parser_state_tos->paren_level = parser_state_tos->p_l_follow;
  if (parser_state_tos->paren_level > 0)
    {
      /* If we broke the line and the following line will
         begin with a rparen, the indentation is set for
         the column of the rparen *before* the break - reset
         the column to the position after the break. */
      if (!not_truncated && *s_code == '('
	  && parser_state_tos->paren_level >= 2)
	{
	  paren_target =
	    -parser_state_tos->paren_indents[parser_state_tos->paren_level - 2];
	  parser_state_tos->paren_indents[parser_state_tos->paren_level - 1]
	    = parser_state_tos->paren_indents[parser_state_tos->paren_level
					      - 2] - 1;
	}
      else
	paren_target =
	  -parser_state_tos->paren_indents[parser_state_tos->paren_level - 1];
    }
  else
    paren_target = 0;
  not_first_line = 1;

  return;
}

/* Return the column in which we should place the code about to be output. */

int
compute_code_target (void)
{
  int target_col = parser_state_tos->ind_level + 1;
  int rc;

  if (!parser_state_tos->paren_level)
    {
      if (parser_state_tos->ind_stmt)
	target_col += continuation_indent;
      rc = target_col;
    }
  else if (lineup_to_parens)
    {
      rc = paren_target;
    }
  else
    {
      rc = target_col + (continuation_indent * parser_state_tos->paren_level);
    }
  return rc;
}

int
compute_label_target (void)
{
  return
    (parser_state_tos->pcase
     ? parser_state_tos->cstk[parser_state_tos->tos] + 1
     : (*s_lab == '#'
	? 1
	: parser_state_tos->ind_level - LABEL_OFFSET + 1));
}

/* VMS defines it's own read routine, `vms_read' */
#ifndef SYS_READ
#define SYS_READ read
#endif

/* Read file FILENAME into a `fileptr' structure, and return a pointer to
   that structure. */

static struct file_buffer fileptr;

struct file_buffer *
read_file (const char *filename)
{
  int fd;
  long size;

  struct stat file_stats;
  size_t namelen = strlen (filename);

  fd = open (filename, O_RDONLY, 0777);
  if (fd < 0)
    fatal ("Can't open input file %s", filename);

  if (fstat (fd, &file_stats) < 0)
    fatal ("Can't stat input file %s", filename);

  if (file_stats.st_size == 0)
    message (-1, "Warning: Zero-length file %s", filename);

  if (file_stats.st_size < 0)
    fatal ("System problem reading file %s", filename);
  fileptr.size = (unsigned long) file_stats.st_size;
  if (fileptr.data != 0)
    fileptr.data = (char *) xrealloc (fileptr.data,
				      (size_t) file_stats.st_size + 1);
  else
    fileptr.data = (char *) xmalloc ((size_t) file_stats.st_size + 1);

  size = SYS_READ (fd, fileptr.data, fileptr.size);
  if (size < 0)
    fatal ("Error reading input file %s", filename);
  if (close (fd) < 0)
    fatal ("Error closing input file %s", filename);

  /* Apparently, the DOS stores files using CR-LF for newlines, but
     then the DOS `read' changes them into '\n'.  Thus, the size of the
     file on disc is larger than what is read into memory.  Thanks, Bill. */
  if ((size_t) size < fileptr.size)
    fileptr.size = (size_t) size;

  if (fileptr.name != 0)
    fileptr.name = (char *) xrealloc (fileptr.name, namelen + 1);
  else
    fileptr.name = (char *) xmalloc (namelen + 1);
  (void) memcpy (fileptr.name, filename, namelen);
  fileptr.name[namelen] = '\0';

  fileptr.data[fileptr.size] = '\0';

  return &fileptr;
}

/* Suck the standard input into a file_buffer structure, and
   return a pointer to that structure. */

static struct file_buffer stdinptr;

struct file_buffer *
read_stdin (void)
{
  /* This should be some system-optimal number */
  size_t size = 15 * BUFSIZ;
  int ch = EOF;
  char *p;

  if (stdinptr.data != 0)
    free (stdinptr.data);

  stdinptr.data = (char *) xmalloc (size + 1);
  stdinptr.size = 0;
  p = stdinptr.data;
  do
    {
      while (stdinptr.size < size)
	{
	  ch = getc (stdin);
	  if (ch == EOF)
	    break;

	  *p++ = (char) ch;
	  stdinptr.size++;
	}

      if (ch != EOF)
	{
	  size += (2 * BUFSIZ);
	  stdinptr.data = xrealloc (stdinptr.data, size);
	  p = stdinptr.data + stdinptr.size;
	}
    }
  while (ch != EOF);

  stdinptr.name = xstrdup ("Standard Input");

  stdinptr.data[stdinptr.size] = '\0';

  return &stdinptr;
}

static void
unterminated_inhibit (int start_no)
{
  int save_line_no = in_line_no;
  in_line_no = start_no + 1;
  message (1, "unterminated INDENT-OFF comment");
  in_line_no = save_line_no;
}

/* Advance `buf_ptr' so that it points to the next line of input.

   If the next input line contains an indent control comment turning
   off formatting (a comment, C or C++, beginning with *INDENT-OFF*),
   then simply print out input lines without formatting until we find
   a corresponding comment containing *INDENT-0N* which re-enables
   formatting.

   Note that if this is a C comment we do not look for the closing
   delimiter.  Note also that older version of this program also
   skipped lines containing *INDENT** which represented errors
   generated by indent in some previous formatting.  This version does
   not recognize such lines. */

void
fill_buffer (void)
{
  char *p;
  int finished_a_line;

  /* indent() may be saving the text between "if (...)" and the following
     statement.  To do so, it uses another buffer (`save_com').  Switch
     back to the previous buffer here. */
  if (bp_save != 0)
    {
      buf_ptr = bp_save;
      buf_end = be_save;
      bp_save = be_save = 0;

      /* only return if there is really something in this buffer */
      if (!at_buffer_end (buf_ptr))
	{
	  return;
	}
    }

  if (*in_prog_pos == '\0')
    {
      cur_line = buf_ptr = in_prog_pos;
      had_eof = true;
      return;
    }

  /* Here if we know there are chars to read.  The file is
     NULL-terminated, so we can always look one character ahead
     safely. */
  p = cur_line = in_prog_pos;
  finished_a_line = 0;
  do
    {
      /*
       * Anything after the second "%%" line is passed to the formatter.
       * Before the second "%%" line, "%{" and "%}" bracket lines that are
       * passed to the formatter.  None of those control lines are passed to
       * the formatter.
       */
      if (lex_or_yacc)
	{
	  int pass_lexcode = (lex_section >= 2);
	  int skip_line = 0;

	  if (!pass_lexcode)
	    {
	      if (!strncmp (p, "%%", (size_t) 2))
		{
		  ++lex_section;
		  if (lex_section >= 0)
		    {
		      reset_output ();
		      skip_line = (lex_section >= 2);
		    }
		}
	      else if (!strncmp (p, "%{", (size_t) 2))
		{
		  next_lexcode = 1;
		  skip_line = 1;
		}
	      else if (!strncmp (p, "%}", (size_t) 2))
		{
		  if (next_lexcode)
		    {
		      if (next_lexcode < 0)
			{
			  dump_line ();
			}
		      pass_char ('\n');
		      next_lexcode = 0;
		    }
		}
	      else if (next_lexcode)
		{
		  if (next_lexcode > 0)
		    {
		      next_lexcode = -1;
		      reset_output ();
		    }
		  pass_lexcode = 1;
		}
	    }

	  if (!pass_lexcode)
	    {
	      p = pass_line (p);
	      if (skip_line)
		{
		  pass_char ('\n');
		}
	      if (!p)
		return;
	      continue;
	    }
	}

      while (isblank (*p))
	p++;

      /* If we are looking at the beginning of a comment, see
         if it turns off formatting with off-on directives. */
      if (beginning_comment (p))
	{
	  p += 2;
	  while (isblank (*p))
	    p++;

	  /* Skip all lines between the indent off and on directives. */
	  if (!strncmp (p, "*INDENT-OFF*", (size_t) 12))
	    {
	      char *q = cur_line;
	      int inhibited = 1;
	      int starting_no = in_line_no;

	      drain_blanklines ();
	      if (s_com != e_com || s_lab != e_lab || s_code != e_code)
		dump_line ();
	      suppress_formatting = 1;
	      while (q < p)
		pass_char (*q++);

	      do
		{
		  while (*p != '\0' && *p != EOL)
		    pass_char (*p++);
		  if (*p == '\0' && at_current_eof (p))
		    {
		      buf_ptr = buf_end = in_prog_pos = p;
		      had_eof = true;
		      unterminated_inhibit (starting_no);
		      return;
		    }

		  if (*p == EOL)
		    {
		      ++in_line_no;
		      cur_line = p + 1;
		    }
		  pass_char (*p++);
		  while (isblank (*p))
		    pass_char (*p++);

		  if (beginning_comment (p))
		    {
		      /* We've hit a comment.  See if it turns formatting
		         back on. */
		      pass_char (*p++);
		      pass_char (*p++);
		      while (isblank (*p))
			pass_char (*p++);
		      if (!strncmp (p, "*INDENT-ON*", (size_t) 11))
			{
			  do
			    {
			      while (*p != '\0' && *p != EOL)
				pass_char (*p++);
			      if (*p == '\0' && at_current_eof (p))
				{
				  buf_ptr = buf_end = in_prog_pos = p;
				  had_eof = true;
				  return;
				}
			      else
				{
				  if (*p == EOL)
				    {
				      ++in_line_no;
				      inhibited = 0;
				      suppress_formatting = 0;
				      cur_line = p + 1;
				      if (*cur_line == '\0' &&
					  at_current_eof (cur_line))
					{
					  pass_char (EOL);
					}
				      else
					{
					  in_line_no--;
					  if (p[0] == EOL && p[1] == EOL)
					    {
					      pass_char (EOL);
					    }
					}
				    }
				  else
				    {
				      pass_char (*p++);
				    }
				}
			    }
			  while (inhibited);
			}
		      else if (!strncmp (p, "*INDENT-OFF*", (size_t) 12))
			{
			  unterminated_inhibit (starting_no);
			  starting_no = in_line_no;
			}
		      else if (!strncmp (p, "*INDENT-", (size_t) 8))
			{
			  message (1, "unexpected INDENT-comment");
			}
		    }
		}
	      while (inhibited);
	    }
	  else if (!strncmp (p, "*INDENT-EQLS*", (size_t) 13))
	    {
	      indent_eqls = -1;
	    }
	}

      while (*p != '\0' && *p != EOL)
	p++;

      /* Here for newline -- finish up unless formatting is off */
      if (*p == EOL)
	{
	  ++in_line_no;
	  finished_a_line = 1;
	  in_prog_pos = p + 1;
	}
      /* Here for embedded NULLs */
      else if ((unsigned) (p - current_input->data) < current_input->size)
	{
	  message (-1, "Warning: File %s contains NULL-characters\n",
		   current_input->name);
	  p++;
	}
      /* Here for EOF with no terminating newline char. */
      else
	{
	  in_prog_pos = p;
	  break;
	}
    }
  while (!finished_a_line);

  buf_ptr = cur_line;
  buf_end = in_prog_pos;
  buf_break = NULL;

  /*
   * Turn off the indent-eqls feature on the next blank line.
   */
  if (indent_eqls > 0)
    {
      int blank = true;

      for (p = buf_ptr; p < buf_end; ++p)
	{
	  if (!isspace (UChar (*p)))
	    {
	      blank = false;
	      break;
	    }
	}
      if (blank)
	{
	  indent_eqls = 0;
	  indent_eqls_1st = 0;
	}
    }

  if (debug)
    {
      printf ("%6d: %s%.*s%s",
	      in_line_no,
	      finished_a_line ? "" : "*",
	      (int) (buf_end - buf_ptr), buf_ptr,
	      finished_a_line ? "" : "\n");
    }
}
