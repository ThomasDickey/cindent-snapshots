/*
   Copyright 1999-2022,2025 Thomas Dickey

   Copyright (c) 1994, Joseph Arceneaux.  All rights reserved

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


/* Here we have the token scanner for indent.  It scans off one token and
   puts it in the global variable "token".  It returns a code, indicating the
   type of token scanned. */

#include "sys.h"
#include "indent.h"

/* Stuff that needs to be shared with the rest of indent. Documented in
   indent.h.  */
char *token;
char *token_end;
int token_len;
char *token_buf;

#define alphanum 1
#define opchar 3

struct templ
  {
    const char *rwd;
    enum rwcodes rwcode;
  };

/* Pointer to a vector of keywords specified by the user.  */
static struct templ *user_specials = NULL;

/* Allocated size of user_specials.  */
static unsigned int user_specials_max;

/* Index in user_specials of the first unused entry.  */
static unsigned int user_specials_idx;

/* this is used to facilitate the decision of what type (alphanumeric,
 * operator) each character is
 */
static char chartype[256];

#include "gperf.c"

/* We do not always increment token_len when we increment buf_ptr, but the
 * reverse is true.  Make the result a little more readable.
 */
#define inc_token_len() token_len += (token_buf == cur_line), ++buf_ptr

static int
ident_is_cast (void)
{
  int result;
  if (parser_state_tos->p_l_follow
      && !(parser_state_tos->noncast_mask
	   & 1 << parser_state_tos->p_l_follow))
    /* inside parens: cast */
    {
      parser_state_tos->cast_mask
	|= 1 << parser_state_tos->p_l_follow;
      result = true;
    }
  else
    {
      result = false;
    }
  return result;
}

static enum codes
parse_token (void)
{

  int unary_delim;		/* this is set to 1 if the current token
				   forces a following operator to be unary */
  static enum codes last_code;	/* the last token type returned */
  static int l_struct;		/* set to 1 if the last token was 'struct' */
  enum codes code;		/* internal code to be returned */
  char qchar;			/* the delimiter character for a string */

  static int count;		/* debugging counter */
  count++;

  unary_delim = false;
  /* tell world that this token started in column 1 iff the last
     thing scanned was nl */
  parser_state_tos->col_1 = parser_state_tos->last_nl;
  parser_state_tos->last_nl = false;

  if (isblank (*buf_ptr))
    {
      parser_state_tos->col_1 = false;
      while (isblank (*buf_ptr))
	if (at_buffer_end (++buf_ptr))
	  fill_buffer ();
    }

  /* INCREDIBLY IMPORTANT WARNING!!!

     Note that subsequent calls to `fill_buffer ()' may switch `buf_ptr'
     to a different buffer.  Thus when `token_end' gets set later, it
     may be pointing into a different buffer than `token'. */
  token = buf_ptr;
  token_len = 0;
  token_buf = cur_line;

  /* Scan an alphanumeric token */
  if ((!(buf_ptr[0] == 'L' && (buf_ptr[1] == '"' || buf_ptr[1] == '\''))
       && chartype[UChar (*buf_ptr)] == alphanum)
      || (buf_ptr[0] == '.' && isdigit (UChar (buf_ptr[1]))))
    {
      /* we have a character or number */
      const struct templ *p;

      if (isdigit (UChar (*buf_ptr))
	  || (buf_ptr[0] == '.' && isdigit (UChar (buf_ptr[1]))))
	{
	  int seendot = 0, seenexp = 0;
	  if (*buf_ptr == '0' &&
	      (buf_ptr[1] == 'x' || buf_ptr[1] == 'X'))
	    {
	      buf_ptr += 2;
	      while (isxdigit (UChar (*buf_ptr)))
		inc_token_len ();
	    }
	  else
	    while (1)
	      {
		if (*buf_ptr == '.')
		  {
		    if (seendot)
		      break;
		    else
		      seendot++;
		  }
		inc_token_len ();
		if (!isdigit (UChar (*buf_ptr)) && *buf_ptr != '.')
		  {
		    if ((*buf_ptr != 'E' && *buf_ptr != 'e') || seenexp)
		      break;
		    else
		      {
			seenexp++;
			seendot++;
			inc_token_len ();
			if (*buf_ptr == '+' || *buf_ptr == '-')
			  inc_token_len ();
		      }
		  }
	      }

	  if (*buf_ptr == 'F' || *buf_ptr == 'f'
	      || *buf_ptr == 'i' || *buf_ptr == 'j')
	    inc_token_len ();
	  else
	    {
	      if (*buf_ptr == 'U' || *buf_ptr == 'u')
		inc_token_len ();
	      if (*buf_ptr == 'L' || *buf_ptr == 'l')
		inc_token_len ();
	      if (*buf_ptr == 'L' || *buf_ptr == 'l')
		inc_token_len ();
	    }
	}
      else
	while (chartype[UChar (*buf_ptr)] == alphanum)
	  {			/* copy it over */
	    inc_token_len ();
	    if (at_buffer_end (buf_ptr))
	      fill_buffer ();
	  }

      token_end = buf_ptr;

      while (isblank (*buf_ptr))
	{
	  if (at_buffer_end (++buf_ptr))
	    fill_buffer ();
	}
      parser_state_tos->its_a_keyword = false;
      parser_state_tos->sizeof_keyword = false;

      /* if last token was 'struct', then this token should be treated
         as a declaration */
      if (l_struct)
	{
	  l_struct = false;
	  last_code = ident;
	  parser_state_tos->last_u_d = true;
	  if (parser_state_tos->last_token == cpp_operator)
	    return overloaded;
	  return (decl);
	}

      /* Operator after identifier is binary */
      parser_state_tos->last_u_d = false;
      last_code = ident;

      /* Check whether the token is a reserved word.  Use perfect hashing... */
      p = is_reserved (token, (unsigned) (token_end - token));

      if (!p && user_specials != NULL)
	{
	  for (p = &user_specials[0];
	       p < &user_specials[0] + user_specials_idx;
	       p++)
	    {
	      const char *q = token;
	      const char *r = p->rwd;

	      /* This string compare is a little nonstandard because token
	         ends at the character before token_end and p->rwd is
	         null-terminated.  */
	      while (1)
		{
		  /* If we have come to the end of both the keyword in
		     user_specials and the keyword in token they are equal.  */
		  if (q >= token_end && !*r)
		    goto found_keyword;

		  /* If we have come to the end of just one, they are not
		     equal.  */
		  if (q >= token_end || !*r)
		    break;

		  /* If the characters in corresponding characters are not
		     equal, the strings are not equal.  */
		  if (*q++ != *r++)
		    break;
		}
	    }
	  /* Didn't find anything in user_specials.  */
	  p = NULL;
	}

      if (p)
	{			/* we have a keyword */
	  enum codes value;

	found_keyword:
	  if (debug)
	    {
	      printf ("keyword \"%.*s\"\n", token_len, token);
	    }
	  value = ident;
	  parser_state_tos->its_a_keyword = true;
	  parser_state_tos->last_u_d = true;
	  parser_state_tos->last_rw = p->rwcode;
	  parser_state_tos->last_rw_depth = parser_state_tos->paren_depth;
	  switch (p->rwcode)
	    {
	    case rw_operator:	/* C++ operator overloading. */
	      value = cpp_operator;
	      parser_state_tos->in_parameter_declaration = 1;
	      break;
	    case rw_switch:	/* it is a switch */
	      value = (swstmt);
	      break;
	    case rw_case:	/* a case or default */
	      value = (casestmt);
	      break;

	    case rw_struct_like:	/* a "struct" */
	      if (!ident_is_cast ())
		{
		  l_struct = true;
		  last_code = decl;
		  value = (decl);
		}
	      break;

	      /* Next time around, we will want to know that we have had a
	         'struct' */
	    case rw_decl:	/* one of the declaration keywords */
	      if (!ident_is_cast ())
		{
		  last_code = decl;
		  value = (decl);
		}
	      break;

	    case rw_sp_paren:	/* if, while, for */
	      value = (sp_paren);
	      break;

	    case rw_sp_nparen:	/* do, else */
	      value = (sp_nparen);
	      break;

	    case rw_sizeof:
	      parser_state_tos->sizeof_keyword = true;
	      value = (ident);
	      break;

	    case rw_return:
	    case rw_break:
	    default:		/* all others are treated like any other
				   identifier */
	      value = (ident);
	    }			/* end of switch */

	  if (parser_state_tos->last_token == cpp_operator)
	    return overloaded;
	  return value;
	}			/* end of if (found_it) */
      else if (*buf_ptr == '('
	       && parser_state_tos->tos <= 1
	       && parser_state_tos->ind_level == 0
	       && parser_state_tos->paren_depth == 0)
	{
	  /* We have found something which might be the name in a function
	     definition.  */
	  const char *tp;
	  int paren_count = 1;

	  /* Skip to the matching ')'.  */
	  for (tp = buf_ptr + 1;
	       paren_count > 0 && tp < in_prog + in_prog_size;
	       tp++)
	    {
	      if (*tp == '(')
		paren_count++;
	      if (*tp == ')')
		paren_count--;
	      /* Can't occur in parameter list; this way we don't search the
	         whole file in the case of unbalanced parens.  */
	      if (*tp == ';')
		goto not_proc;
	    }

	  if (paren_count == 0)
	    {
	      parser_state_tos->procname = token;
	      parser_state_tos->procname_end = token_end;

	      while (isspace (UChar (*tp)))
		tp++;

	      /* If the next char is ';' or ',' or '(' we have a function
	         declaration, not a definition.

	         I've added '=' to this list to keep from breaking
	         a non-valid C macro from libc.  -jla */
	      if (*tp != ';' && *tp != ',' && *tp != '(' && *tp != '=')
		{
		  parser_state_tos->in_parameter_declaration = 1;
		}
	    }

	not_proc:;
	}
      else if (*buf_ptr == ':' && *(buf_ptr + 1) == ':'
	       && parser_state_tos->tos <= 1
	       && parser_state_tos->ind_level == 0
	       && parser_state_tos->paren_depth == 0)
	{
	  parser_state_tos->classname = token;
	  parser_state_tos->classname_end = token_end;
	}
      /* The following hack attempts to guess whether or not the
         current token is in fact a declaration keyword -- one that
         has been typedef'd */
      else if (((*buf_ptr == '*' && buf_ptr[1] != '=')
		|| isalpha (UChar (*buf_ptr)) || *buf_ptr == '_')
	       && !parser_state_tos->p_l_follow
	       && !parser_state_tos->block_init
	       && (parser_state_tos->last_token == rparen
		   || parser_state_tos->last_token == semicolon
		   || parser_state_tos->last_token == rbrace
		   || parser_state_tos->last_token == decl
		   || parser_state_tos->last_token == lbrace
		   || parser_state_tos->last_token == start_token))
	{
	  parser_state_tos->its_a_keyword = true;
	  parser_state_tos->last_u_d = true;
	  last_code = decl;
	  if (parser_state_tos->last_token == cpp_operator)
	    return overloaded;

	  return decl;
	}
      /*
       * If auto-typedefs, treat any identifier ending with "_t" as a type.
       */
      else if (auto_typedefs)
	{
	  if (token_len > 2
	      && !strncmp (token_end - 2, "_t", (size_t) 2))
	    {
	      parser_state_tos->its_a_keyword = true;
	      parser_state_tos->last_u_d = true;
	      parser_state_tos->last_rw_depth = parser_state_tos->paren_depth;
	      if (!ident_is_cast ())
		{
		  parser_state_tos->last_rw = rw_decl;
		  last_code = decl;
		  return (decl);
		}
	    }
	}

      last_code = ident;
      if (parser_state_tos->last_token == cpp_operator)
	return overloaded;

      return (ident);		/* the ident is not in the list */
    }				/* end of processing for alphanum character */
  /* Scan a non-alphanumeric token */

  /* If it is not a one character token, token_end will get changed later.  */
  token_end = buf_ptr + 1;

  /* THIS MAY KILL YOU!!!

     Note that it may be possible for this to kill us--if `fill_buffer'
     at any time switches `buf_ptr' to the other input buffer, `token'
     and `token_end' will point to different storage areas!!! */
  if (at_buffer_end (++buf_ptr))
    fill_buffer ();

  switch (*token)
    {
    case '\0':
      code = code_eof;
      break;

    case EOL:
      unary_delim = parser_state_tos->last_u_d;
      parser_state_tos->last_nl = true;
      code = newline;
      break;

      /* Handle wide strings and chars. */
    case 'L':
      if (buf_ptr[0] != '"' && buf_ptr[0] != '\'')
	{
	  token_end = buf_ptr;
	  code = ident;
	  break;
	}

      qchar = buf_ptr[0];
      inc_token_len ();
      goto handle_string;

    case '\'':			/* start of quoted character */
    case '"':			/* start of string */
      qchar = *token;

    handle_string:
      /* Find out how big the literal is so we can set token_end.  */

      /* Invariant:  before loop test buf_ptr points to the next */
      /* character that we have not yet checked. */
      while (*buf_ptr != qchar && *buf_ptr != 0 && *buf_ptr != EOL)
	{
	  if (*buf_ptr == '\\')
	    {
	      inc_token_len ();
	      if (at_buffer_end (buf_ptr))
		fill_buffer ();
	      if (*buf_ptr == 0)
		break;
	    }
	  inc_token_len ();
	  if (at_buffer_end (buf_ptr))
	    fill_buffer ();
	}

      if (*buf_ptr == EOL || *buf_ptr == 0)
	{
	  message (1, (qchar == '\''
		       ? "Unterminated character constant"
		       : "Unterminated string constant"));
	}
      else
	{
	  /* Advance over end quote char.  */
	  inc_token_len ();
	  if (at_buffer_end (buf_ptr))
	    fill_buffer ();
	}

      token_end = buf_ptr;
      code = ident;
      break;

    case ('('):
    case ('['):
      unary_delim = true;
      code = lparen;
      break;

    case (')'):
    case (']'):
      code = rparen;
      break;

    case '#':
      unary_delim = parser_state_tos->last_u_d;
      code = preesc;

      /* Make spaces between '#' and the directive be part of
         the token if user specified "-lps" and did not specify "-ppi" */
      if (leave_preproc_space && !preprocessor_indentation)
	{
	  while (isblank (*buf_ptr) && !at_buffer_end (buf_ptr))
	    inc_token_len ();
	  token_end = buf_ptr;
	}
      else
	{
	  while (isblank (*buf_ptr) && !at_buffer_end (buf_ptr))
	    ++buf_ptr;
	}
      break;

    case '?':
      unary_delim = true;
      code = question;
      break;

    case (':'):
      /* Deal with C++ class::method */
      if (*buf_ptr == ':')
	{
	  code = doublecolon;
	  inc_token_len ();
	  token_end = buf_ptr;
	  break;
	}

      code = colon;
      unary_delim = true;
      if (squest && !isblank (*e_com))
	{
	  if (e_code == s_code)
	    parser_state_tos->want_blank = false;
	  else
	    parser_state_tos->want_blank = true;
	}
      break;

    case (';'):
      unary_delim = true;
      code = semicolon;
      break;

    case (L_CURL):
      unary_delim = true;

      /* The following neat hack causes the braces in structure
         initializations to be treated as parentheses, thus causing
         initializations to line up correctly, e.g. struct foo bar = {{a, b,
         c}, {1, 2}}; If lparen is returned, token can be used to distinguish
         between left curly-brace and '(' where necessary.  */

      code = parser_state_tos->block_init ? lparen : lbrace;
      break;

    case (R_CURL):
      unary_delim = true;
      /* The following neat hack is explained under left curly-brace above.  */
      code = parser_state_tos->block_init ? rparen : rbrace;

      break;

    case FORM_FEED:		/* a form feed */
      unary_delim = parser_state_tos->last_u_d;
      parser_state_tos->last_nl = true;		/* remember this so we can set
						   'parser_state_tos->col_1' right */
      code = form_feed;
      break;

    case (','):
      unary_delim = true;
      code = comma;
      break;

    case '.':
      /* check for "..." */
      if ((buf_end - buf_ptr) >= 2 &&
	  buf_ptr[0] == token[0] &&
	  buf_ptr[1] == token[0])
	{
	  inc_token_len ();
	  inc_token_len ();
	  token_end = buf_ptr;
	  inc_token_len ();
	  buf_ptr--;
	  unary_delim = false;
	  /* pretend it is an identifier, with corresponding spacing */
	  code = (ident);
	  parser_state_tos->its_a_keyword = true;
	  parser_state_tos->last_u_d = true;
	  parser_state_tos->last_rw_depth = parser_state_tos->paren_depth;
	  if (debug)
	    {
	      printf ("keyword \"%.*s\"\n", token_len, token);
	    }
	}
      else
	{
	  unary_delim = false;
	  code = struct_delim;
	}
      break;

    case '-':
    case '+':			/* check for -, +, --, ++ */
      code = (parser_state_tos->last_u_d ? unary_op : binary_op);
      unary_delim = true;

      if (*buf_ptr == token[0])
	{
	  /* check for doubled character */
	  inc_token_len ();
	  /* buffer overflow will be checked at end of loop */
	  if (last_code == ident || last_code == rparen)
	    {
	      code = (parser_state_tos->last_u_d ? unary_op : postop);
	      /* check for following ++ or -- */
	      unary_delim = false;
	    }
	}
      else if (*buf_ptr == '=')
	/* check for operator += */
	inc_token_len ();
      else if (*buf_ptr == '>')
	{
	  /* check for operator -> */
	  inc_token_len ();
	  code = struct_delim;
	}

      token_end = buf_ptr;
      break;			/* buffer overflow will be checked at end of
				   switch */

    case '=':
      if (parser_state_tos->in_or_st &&
	  parser_state_tos->last_token != cpp_operator)
	parser_state_tos->block_init = true;

      if (*buf_ptr == '=')	/* == */
	inc_token_len ();
      else if (*buf_ptr == '-'
	       || *buf_ptr == '+'
	       || *buf_ptr == '*'
	       || *buf_ptr == '&')
	{
	  /* Something like x=-1, which can mean x -= 1 ("old style" in K&R1)
	     or x = -1 (ANSI).  Note that this is only an ambiguity if the
	     character can also be a unary operator.  If not, just produce
	     output code that produces a syntax error (the theory being that
	     people want to detect and eliminate old style assignments but
	     they don't want indent to silently change the meaning of their
	     code).  */
	  message (1,
		   "old style assignment ambiguity in \"=%c\".  Assuming \"= %c\"\n",
		   (int) *buf_ptr, (int) *buf_ptr);
	}

      code = binary_op;
      unary_delim = true;
      token_end = buf_ptr;
      if (indent_eqls < 0)
	{
	  indent_eqls = token_col ();
	}
      break;
      /* can drop through!!! */

    case '>':
    case '<':
    case '!':
      /* ops like <, <<, <=, !=, <<=, etc */
      /* This will of course scan sequences like "<=>", "!=>", "<<>", etc. as
         one token, but I don't think that will cause any harm.  */
      while (*buf_ptr == '>' || *buf_ptr == '<' || *buf_ptr == '=')
	{
	  inc_token_len ();
	  if (at_buffer_end (buf_ptr))
	    fill_buffer ();
	  if (*buf_ptr == '=')
	    {
	      inc_token_len ();
	      if (at_buffer_end (buf_ptr))
		fill_buffer ();
	    }
	}

      code = (parser_state_tos->last_u_d ? unary_op : binary_op);
      unary_delim = true;
      token_end = buf_ptr;
      break;

    default:
      if (token[0] == '/' && (*buf_ptr == '*' || *buf_ptr == '/'))
	{
	  /* A C or C++ comment */

	  if (*buf_ptr == '*')
	    code = comment;
	  else
	    code = cplus_comment;

	  inc_token_len ();
	  if (at_buffer_end (buf_ptr))
	    fill_buffer ();

	  unary_delim = parser_state_tos->last_u_d;
	}
      else if (parser_state_tos->last_token == cpp_operator)
	{
	  /* For C++ overloaded operators. */
	  code = overloaded;
	  last_code = overloaded;
	}
      else
	{
	  while (*(buf_ptr - 1) == *buf_ptr || *buf_ptr == '=')
	    {
	      /* handle ||, &&, etc, and also things as in int *****i */
	      inc_token_len ();
	      if (at_buffer_end (buf_ptr))
		fill_buffer ();
	    }
	  code = (parser_state_tos->last_u_d ? unary_op : binary_op);
	  unary_delim = true;
	}

      token_end = buf_ptr;

    }				/* end of switch */

  if (code != newline)
    {
      l_struct = false;
      last_code = code;
    }

  if (at_buffer_end (buf_ptr))
    fill_buffer ();
  parser_state_tos->last_u_d = unary_delim;

  if (parser_state_tos->last_token == cpp_operator)
    return overloaded;
  return (code);
}

enum codes
lexi (void)
{
  enum codes code = parse_token ();
  if (debug > 1)
    {
      switch (code)
	{
	case lparen:
	case rparen:
	case comma:
	case semicolon:
	case colon:
	case question:
	case binary_op:
	  printf ("lexi: %s '%c'\n", parsecode2s (code), token[0]);
	  break;
	default:
	  printf ("lexi: %s\n", parsecode2s (code));
	  break;
	}
    }
  return code;
}

int
token_col (void)
{
  int col;
  int n;

  for (n = col = 0; n <= (token - token_buf); ++n)
    {
      if (token_buf[n] == '\t')
	col = (col | 7) + 1;
      else
	++col;
    }
  if (col == 0)
    col = 1;
  return col;
}

int
first_token_col (void)
{
  int column;
  int tabbed = 0;
  int n;

  for (n = column = 0; n <= (buf_ptr - cur_line); ++n)
    {
      if (cur_line[n] == TAB)
	{
	  column += tabsize - (column - 1) % tabsize;
	  tabbed = 1;
	}
      else if (cur_line[n] == ' ')
	{
	  ++column;
	}
      else
	{
	  break;
	}
    }
  if (!tabbed)
    ++column;
  return column;
}

/* Add the given keyword to the keyword table, using val as
   the keyword type */

int
addkey (const char *key, enum rwcodes val)
{
  struct templ *p;

  /* Check to see whether key is a reserved word or not. */
  if (is_reserved (key, (unsigned) strlen (key)) != NULL)
    return 0;

  if (user_specials == NULL)
    {
      user_specials = (struct templ *) xmalloc (5 * sizeof (struct templ));
      user_specials_max = 5;
      user_specials_idx = 0;
    }
  else if (user_specials_idx == user_specials_max)
    {
      user_specials_max += 5;
      user_specials = (struct templ *) xrealloc ((char *) user_specials,
						 user_specials_max
						 * sizeof (struct templ));
    }

  p = &user_specials[user_specials_idx++];
  p->rwd = key;
  p->rwcode = val;
  return 1;
}

static void
set_chartype (const char *list, int code)
{
  while (*list != 0)
    {
      chartype[UChar (*list++)] = (char) code;
    }
}

/*
 * We could build the program source using special tools such as gperf, and
 * hardcode the character table - or we could generate the tables once at
 * runtime.  The latter is not much overhead, and is more easily maintained.
 */
void
init_lexi (void)
{
  memset (chartype, 0, sizeof (chartype));
  set_chartype ("$_0123456789", alphanum);
  set_chartype ("abcdefghijklmnopqrstuvwxyz", alphanum);
  set_chartype ("ABCDEFGHIJKLMNOPQRSTUVWXYZ", alphanum);
  set_chartype ("!%&*+-/<=>?^|~", opchar);
}
