/* Copyright (c) 1994, Joseph Arceneaux.  All rights reserved.

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

#include <ctype.h>
#include <stdlib.h>

#if defined (HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <string.h>

#ifdef VMS
#   include <file.h>
#   include <types.h>
#   include <stat.h>
#else  /* not VMS */

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
int  break_line;
int had_eof;
int out_lines;
int com_lines;

int suppress_blanklines = 0;
static int comment_open;

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

    bufp  = buffer;
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
pass_char(int ch)
{
  if (ch == '\n')
    ++line_no;
  putc (ch, output);
}

static void
pass_text(char *s)
{
  while (*s != 0)
    pass_char(*s++);
}

static void
pass_n_text(char *s, int n)
{
  while (n-- > 0)
    pass_char(*s++);
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
	case 014:		/* form feed */
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

  while (p < buf_ptr)
    {
      switch (*p)
	{
	case EOL:
	case 014:		/* form feed */
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

      p++;
    }

  return column;
}

/* Fill the output line with whitespace up to TARGET_COLUMN, given that
   the line is currently in column CURRENT_COLUMN.  Returns the ending
   column. */

static int
pad_output (
  int my_current_col,
  int target_column)
{
  if (my_current_col >= target_column)
    return my_current_col;

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

  return my_current_col;
}

void
dump_line (void)
{				/* dump_line is the routine that actually
				   effects the printing of the new source. It
				   prints the label section, followed by the
				   code section with the appropriate nesting
				   level, followed by any comments */
  int cur_col;
  int target_col = 0;
  static int not_first_line;
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
	  pass_char ('\014');
	  parser_state_tos->use_ff = false;
	}
      else
	{
	  if (suppress_blanklines > 0)
	    suppress_blanklines--;
	  else
	    {
	      parser_state_tos->bl_line = true;
	      n_real_blanklines++;
	    }
	}
    }
  else
    {
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
      if (parser_state_tos->ind_level == 0)
	parser_state_tos->ind_stmt = 0;	/* This is a class A kludge. Don't do
					   additional statement indentation
					   if we are at bracket level 0 */

      if (e_lab != s_lab || e_code != s_code)
	++code_lines;		/* keep count of lines with code */

      if (e_lab != s_lab)
	{			/* print lab, if any */
	  if (comment_open)
	    {
	      comment_open = 0;
	      pass_text (".*/\n");
	    }
	  while (e_lab > s_lab && (e_lab[-1] == ' ' || e_lab[-1] == TAB))
	    e_lab--;
	  cur_col = pad_output (1, compute_label_target ());
	  if (s_lab[0] == '#' && (strncmp (s_lab, "#else", 5) == 0
				  || strncmp (s_lab, "#endif", 6) == 0))
	    {
	      /* Treat #else and #endif as a special case because any text
	         after #else or #endif should be converted to a comment.  */
	      char *s = s_lab;
	      if (e_lab[-1] == EOL)
		e_lab--;
	      do
		pass_char (*s++);
	      while (s < e_lab && 'a' <= *s && *s <= 'z');
	      while ((*s == ' ' || *s == TAB) && s < e_lab)
		s++;
	      if (s < e_lab)
		{
		  pass_char((tabsize > 1) ? '\t' : ' ');
		  if (s[0] == '/' && (s[1] == '*' || s[1] == '/'))
		    {
		      pass_n_text (s, e_lab - s);
		    }
		  else
		    {
		      pass_text("/* ");
		      pass_n_text (s, e_lab - s);
		      pass_text(" */");
		    }
		}
	    }
	  else
	    pass_n_text (s_lab, e_lab - s_lab);
	  cur_col = count_columns (cur_col, s_lab, NULL_CHAR);
	}
      else
	cur_col = 1;		/* there is no label section */

      parser_state_tos->pcase = false;

      /* Remove trailing spaces */
      while (*(e_code - 1) == ' ' && e_code > s_code)
	*(--e_code) = NULL_CHAR;
      if (s_code != e_code)
	{			/* print code section, if any */
	  char *p;
	  int i;

	  if (comment_open)
	    {
	      comment_open = 0;
	      pass_text (".*/\n");
	    }

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
	      int len;

	      for (p = s_code; p < buf_break; p++)
		pass_char (*p);

	      *buf_break = '\0';
	      cur_col = count_columns (cur_col, s_code, NULL_CHAR);

	      not_truncated = 0;
	      len = (e_code - buf_break - 1);
	      memmove (s_code, buf_break + 1, (unsigned) len);
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

	      if (cur_col > target)
		{
		  pass_char (EOL);
		  cur_col = 1;
		  ++out_lines;
		}

	      cur_col = pad_output (cur_col, target);
	      pass_n_text (com_st, e_com - com_st);
	      cur_col += e_com - com_st;
	      com_lines++;
	    }
	}
      else if (embedded_comment_on_line)
	com_lines++;
      embedded_comment_on_line = 0;

      if (parser_state_tos->use_ff)
	{
	  pass_char ('\014');
	  parser_state_tos->use_ff = false;
	}
      else
	pass_char (EOL);

      ++out_lines;
      if (parser_state_tos->just_saw_decl == 1
	  && blanklines_after_declarations)
	{
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
  *(e_lab  = s_lab) = '\0';	/* reset buffers */
  if (not_truncated)
    {
      *(e_code = s_code) = '\0';
    }

  break_line = 0;
  buf_break = NULL;

  *(e_com  = s_com) = '\0';
  parser_state_tos->ind_level = parser_state_tos->i_l_follow;
  parser_state_tos->paren_level = parser_state_tos->p_l_follow;
  if (parser_state_tos->paren_level > 0)
    {
      /* If we broke the line and the following line will
	 begin with a rparen, the indentation is set for
	 the column of the rparen *before* the break - reset
	 the column to the position after the break. */
      if (! not_truncated && *s_code == '('
	  && parser_state_tos->paren_level >= 2)
	{
	  paren_target = -parser_state_tos->paren_indents[parser_state_tos->paren_level - 2];
	  parser_state_tos->paren_indents[parser_state_tos->paren_level - 1]
	    = parser_state_tos->paren_indents[parser_state_tos->paren_level - 2] - 1;
	}
      else
	paren_target = -parser_state_tos->paren_indents[parser_state_tos->paren_level - 1];
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

  if (! parser_state_tos->paren_level)
    {
      if (parser_state_tos->ind_stmt)
	target_col += continuation_indent;
      return target_col;
    }

  if (! lineup_to_parens)
    return target_col + (continuation_indent * parser_state_tos->paren_level);
  return paren_target;
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
read_file (
     char *filename)
{
  int fd;
  /* Required for MSDOS, in order to read files larger than 32767
     bytes in a 16-bit world... */
  unsigned int size;

  struct stat file_stats;
  unsigned namelen = strlen (filename);

  fd = open (filename, O_RDONLY, 0777);
  if (fd < 0)
    fatal ("Can't open input file %s", filename);

  if (fstat (fd, &file_stats) < 0)
    fatal ("Can't stat input file %s", filename);

  if (file_stats.st_size == 0)
    ERROR ("Warning: Zero-length file %s", filename, 0);

#ifdef __MSDOS__
  if (file_stats.st_size < 0 || file_stats.st_size > (0xffff - 1))
    fatal ("File %s is too big to read", filename);
#else
  if (file_stats.st_size < 0)
    fatal ("System problem reading file %s", filename);
#endif
  fileptr.size = file_stats.st_size;
  if (fileptr.data != 0)
    fileptr.data = (char *) xrealloc (fileptr.data,
				      (unsigned) file_stats.st_size + 1);
  else
    fileptr.data = (char *) xmalloc ((unsigned) file_stats.st_size + 1);

  size = SYS_READ (fd, fileptr.data, fileptr.size);
#ifdef __MSDOS__
  if (size == 0xffff) /* -1 = 0xffff in 16-bit world */
#else
  if (size < 0)
#endif
    fatal ("Error reading input file %s", filename);
  if (close (fd) < 0)
    fatal ("Error closing input file %s", filename);

  /* Apparently, the DOS stores files using CR-LF for newlines, but
     then the DOS `read' changes them into '\n'.  Thus, the size of the
     file on disc is larger than what is read into memory.  Thanks, Bill. */
  if (size < fileptr.size)
    fileptr.size = size;

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

struct file_buffer stdinptr;

struct file_buffer *
read_stdin (void)
{
  /* This should be some system-optimal number */
  unsigned int size = 15 * BUFSIZ;
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

	  *p++ = ch;
	  stdinptr.size++;
	}

      if (ch != EOF)
	{
	  size += (2 * BUFSIZ);
	  stdinptr.data = xrealloc (stdinptr.data, (unsigned) size);
	  p = stdinptr.data + stdinptr.size;
	}
    }
  while (ch != EOF);

  stdinptr.name = "Standard Input";

  stdinptr.data[stdinptr.size] = '\0';

  return &stdinptr;
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
      if (buf_ptr < buf_end)
	return;
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
      while (*p == ' ' || *p == TAB)
	p++;

      /* If we are looking at the beginning of a comment, see
	 if it turns off formatting with off-on directives. */
      if (*p == '/' && (*(p + 1) == '*' || *(p + 1) == '/'))
	{
	  p += 2;
	  while (*p == ' ' || *p == TAB)
	    p++;

	  /* Skip all lines between the indent off and on directives. */
	  if (! strncmp (p, "*INDENT-OFF*", 12))
	    {
	      char *q = cur_line;
	      int inhibited = 1;

	      if (s_com != e_com || s_lab != e_lab || s_code != e_code)
		dump_line ();
	      while (q < p)
		pass_char (*q++);

	      do
		{
		  while (*p != '\0' && *p != EOL)
		    pass_char (*p++);
		  if (*p == '\0'
		      && ((unsigned int) (p - current_input->data) == current_input->size))
		    {
		      buf_ptr = buf_end = in_prog_pos = p;
		      had_eof = 1;
		      return;
		    }

		  if (*p == EOL)
		    cur_line = p + 1;
		  pass_char (*p++);
		  while (*p == ' ' || *p == TAB)
		    pass_char (*p++);

		  if (*p == '/' && (*(p + 1) == '*' || *(p + 1) == '/'))
		    {
		      /* We've hit a comment.  See if turns formatting
			 back on. */
		      pass_char (*p++);
		      pass_char (*p++);
		      while (*p == ' ' || *p == TAB)
			pass_char (*p++);
		      if (! strncmp (p, "*INDENT-ON*", 11))
			{
			  do
			    {
			      while (*p != '\0' && *p != EOL)
				pass_char (*p++);
			      if (*p == '\0'
				  && ((unsigned int) (p - current_input->data) == current_input->size))
				{
				  buf_ptr = buf_end = in_prog_pos = p;
				  had_eof = 1;
				  return;
				}
			      else
				{
				  if (*p == EOL)
				    {
				      inhibited = 0;
				      cur_line = p + 1;
				    }
				  pass_char (*p++);
				}
			    }
			  while (inhibited);
			}
		    }
		}
	      while (inhibited);
	    }
	}

      while (*p != '\0' && *p != EOL)
	p++;

      /* Here for newline -- finish up unless formatting is off */
      if (*p == EOL)
	{
	  finished_a_line = 1;
	  in_prog_pos = p + 1;
	}
      /* Here for embedded NULLs */
      else if ((unsigned int) (p - current_input->data) < current_input->size)
	{
	  WARNING ("Warning: File %s contains NULL-characters\n",
		   current_input->name, 0);
	  p++;
	}
      /* Here for EOF with no terminating newline char. */
      else
	{
	  in_prog_pos = p;
	  finished_a_line = 1;
	}
    }
  while (!finished_a_line);

  buf_ptr = cur_line;
  buf_end = in_prog_pos;
  buf_break = NULL;
}

#ifdef DEBUG
void
dump_debug_line (void)
{
  pass_text ("\n*** Debug output marker line ***\n");
}

#endif
