/*
   Copyright 1999-2010,2018, Thomas E. Dickey

   Copyright (c) 1994, Joseph Arceneaux.  All rights reserved

   Copyright (c) 1985 Sun Microsystems, Inc. Copyright (c) 1980 The Regents
   of the University of California. Copyright (c) 1976 Board of Trustees of
   the University of Illinois. All rights reserved.

   Redistribution and use in source and binary forms are permitted provided
   that the above copyright notice and this paragraph are duplicated in all
   such forms and that any documentation, advertising materials, and other
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

struct parser_state *parser_state_tos;

#define INITIAL_BUFFER_SIZE ((size_t) 1000)
#define INITIAL_STACK_SIZE 2

const char *
parsecode2s (enum codes value)
{
  const char *result = "?";
  switch (value)
    {
    case code_eof:
      result = "EOF";
      break;
    case newline:
      result = "newline";
      break;
    case lparen:
      result = "left parenthesis";
      break;
    case rparen:
      result = "right parenthesis";
      break;
    case start_token:
      result = "start_token";
      break;
    case unary_op:
      result = "unary_op";
      break;
    case binary_op:
      result = "binary_op";
      break;
    case postop:
      result = "postop";
      break;
    case question:
      result = "question mark";
      break;
    case casestmt:
      result = "casestmt";
      break;
    case colon:
      result = "colon";
      break;
    case doublecolon:		/* For C++ class methods. */
      result = "doublecolon";
      break;
    case semicolon:
      result = "semicolon";
      break;
    case lbrace:
      result = "left brace";
      break;
    case rbrace:
      result = "right brace";
      break;
    case ident:
      result = "identifier or string";
      break;
    case overloaded:		/* For C++ overloaded operators (like +) */
      result = "overloaded";
      break;
    case cpp_operator:
      result = "cpp_operator";
      break;
    case comma:
      result = "comma";
      break;
    case comment:		/* A "slash-star" comment */
      result = "comment";
      break;
    case cplus_comment:	/* A C++ "slash-slash" */
      result = "cplus_comment";
      break;
    case swstmt:
      result = "switch-statement";
      break;
    case preesc:		/* '#'.  */
      result = "preesc";
      break;
    case form_feed:
      result = "form_feed";
      break;
    case decl:
      result = "declaration";
      break;
    case sp_paren:		/* if, for, or while token */
      result = "sp_paren";
      break;
    case sp_nparen:
      result = "sp_nparen";
      break;
    case ifstmt:
      result = "if-statement";
      break;
    case whilestmt:
      result = "while-statement";
      break;
    case forstmt:
      result = "for-statement";
      break;
    case stmt:
      result = "statement";
      break;
    case stmtl:
      result = "statement list";
      break;
    case elselit:
      result = "elselit";
      break;
    case dolit:
      result = "dolit";
      break;
    case dohead:
      result = "do-head";
      break;
    case dostmt:
      result = "do-statement";
      break;
    case ifhead:
      result = "if-head";
      break;
    case elsehead:
      result = "else-head";
      break;
    case struct_delim:		/* '.' or "->" */
      result = "struct_delim";
      break;
    case ellipsis:		/* "..." */
      result = "ellipsis";
      break;
    }
  return result;
}

void
init_parser (void)
{
  parser_state_tos
    = (struct parser_state *) xmalloc (sizeof (struct parser_state));

  parser_state_tos->p_stack_size = INITIAL_STACK_SIZE;
  parser_state_tos->p_stack
    = (enum codes *) xmalloc (INITIAL_STACK_SIZE * sizeof (enum codes));
  parser_state_tos->il
    = (int *) xmalloc (INITIAL_STACK_SIZE * sizeof (int));
  parser_state_tos->cstk
    = (int *) xmalloc (INITIAL_STACK_SIZE * sizeof (int));
  parser_state_tos->paren_indents = (int *) xmalloc (sizeof (int));

  /* Although these are supposed to grow if we reach the end,
     I can find no place in the code which does this. */
  combuf = (char *) xmalloc (INITIAL_BUFFER_SIZE);
  labbuf = (char *) xmalloc (INITIAL_BUFFER_SIZE);
  codebuf = (char *) xmalloc (INITIAL_BUFFER_SIZE);

  save_com.size = INITIAL_BUFFER_SIZE;
  save_com.end = save_com.ptr = xmalloc (save_com.size);
  save_com.len = 0;
  save_com.column = 0;

  di_stack_alloc = 2;
  di_stack = (int *) xmalloc (di_stack_alloc * sizeof (*di_stack));

}

void
reset_parser (void)
{
  /* *INDENT-EQLS* */
  parser_state_tos->next               = 0;
  parser_state_tos->tos                = 0;
  parser_state_tos->paren_indents_size = 1;
  parser_state_tos->p_stack[0]         = stmt;
  parser_state_tos->last_nl            = true;
  parser_state_tos->last_token         = start_token;
  parser_state_tos->box_com            = false;
  parser_state_tos->cast_mask          = 0;
  parser_state_tos->noncast_mask       = 0;
  parser_state_tos->sizeof_mask        = 0;
  parser_state_tos->block_init         = false;
  parser_state_tos->block_init_level   = 0;
  parser_state_tos->col_1              = false;
  parser_state_tos->com_col            = 0;
  parser_state_tos->dec_nest           = 0;
  parser_state_tos->i_l_follow         = 0;
  parser_state_tos->ind_level          = 0;
  parser_state_tos->inner_stmt         = 0;
  parser_state_tos->last_u_d           = false;
  parser_state_tos->p_l_follow         = 0;
  parser_state_tos->paren_level        = 0;
  parser_state_tos->paren_depth        = 0;
  parser_state_tos->search_brace       = false;
  parser_state_tos->use_ff             = false;
  parser_state_tos->its_a_keyword      = false;
  parser_state_tos->sizeof_keyword     = false;
  parser_state_tos->dumped_decl_indent = false;
  parser_state_tos->in_parameter_declaration = false;
  parser_state_tos->in_comment         = false;
  parser_state_tos->just_saw_decl      = false;
  parser_state_tos->in_decl            = false;
  parser_state_tos->decl_on_line       = false;
  parser_state_tos->in_or_st           = false;
  parser_state_tos->bl_line            = true;
  parser_state_tos->want_blank         = false;
  parser_state_tos->in_stmt            = false;
  parser_state_tos->ind_stmt           = false;
  parser_state_tos->procname           = "\0";
  parser_state_tos->procname_end       = "\0";
  parser_state_tos->classname          = "\0";
  parser_state_tos->classname_end      = "\0";
  parser_state_tos->pcase              = false;
  parser_state_tos->dec_nest           = 0;
  parser_state_tos->preprocessor_indent = false;
  parser_state_tos->il[0]              = 0;
  parser_state_tos->cstk[0]            = 0;
  parser_state_tos->paren_indents[0]   = 0;

  save_com.len = 0;
  save_com.column = 0;

  di_stack[parser_state_tos->dec_nest] = 0;

  /* *INDENT-EQLS* */
  l_com     = combuf + INITIAL_BUFFER_SIZE - 5;
  l_lab     = labbuf + INITIAL_BUFFER_SIZE - 5;
  l_code    = codebuf + INITIAL_BUFFER_SIZE - 5;
  combuf[0] = codebuf[0] = labbuf[0] = ' ';
  combuf[1] = codebuf[1] = labbuf[1] = '\0';

  else_if = 1;
  else_or_endif = false;
  s_lab = e_lab = labbuf + 1;
  s_code = e_code = codebuf + 1;
  s_com = e_com = combuf + 1;

  in_line_no = 0;
  out_line_no = 1;
  had_eof = false;
  break_comma = false;
  bp_save = 0;
  be_save = 0;

  if (tabsize <= 0)
    tabsize = 1;

  indent_eqls = 0;
  indent_eqls_1st = 0;
}

/* like ++parser_state_tos->tos but checks for stack overflow and extends
   stack if necessary.  */

int
inc_pstack (void)
{
  if (++parser_state_tos->tos >= (int) parser_state_tos->p_stack_size)
    {
      parser_state_tos->p_stack_size *= 2;
      parser_state_tos->p_stack = (enum codes *)
	xrealloc ((char *) parser_state_tos->p_stack,
		  parser_state_tos->p_stack_size * sizeof (enum codes));
      parser_state_tos->il = (int *)
	xrealloc ((char *) parser_state_tos->il,
		  parser_state_tos->p_stack_size * sizeof (int));
      parser_state_tos->cstk = (int *)
	xrealloc ((char *) parser_state_tos->cstk,
		  parser_state_tos->p_stack_size * sizeof (int));
    }

  parser_state_tos->cstk[parser_state_tos->tos]
    = parser_state_tos->cstk[parser_state_tos->tos - 1];
  return parser_state_tos->tos;
}

#define SHOW_PARSER(name) \
  if (s_##name < e_##name) \
    printf(#name ":%.*s\n", (int) (e_##name - s_##name), s_##name)

void
show_parser (const char *fn, int ln)
{
  printf ("<<%s@%d>>\n", fn, ln);
  SHOW_PARSER (lab);
  SHOW_PARSER (code);
  SHOW_PARSER (com);
}

void
show_pstack (void)
{

  if (debug)
    {
      int i;

      printf ("\nParseStack [%d]:\n", (int) parser_state_tos->p_stack_size);
      for (i = 1; i <= parser_state_tos->tos; ++i)
	printf ("  [%3d] =>  indent:%3d  stack: %2d (%s)\n",
		(int) i,
		(int) parser_state_tos->il[i],
		(int) parser_state_tos->p_stack[i],
		parsecode2s (parser_state_tos->p_stack[i]));
      printf ("\n");
    }
}

enum exit_values
parse (enum codes tk)		/* the code for the construct scanned */
{
  if (debug)
    {
      if (debug > 1)
	printf ("%s:%d:%d: ", in_name, in_line_no, token_col ());
      printf ("Parse: %s\n", parsecode2s (tk));
    }

  while (parser_state_tos->p_stack[parser_state_tos->tos] == ifhead
	 && tk != elselit)
    {
      /* true if we have an if without an else */

      /* apply the if(..) stmt ::= stmt reduction */
      parser_state_tos->p_stack[parser_state_tos->tos] = stmt;
      reduce ();		/* see if this allows any reduction */
    }


  switch (tk)
    {				/* go on and figure out what to do with the
				   input */

    case decl:			/* scanned a declaration word */
      parser_state_tos->search_brace = btype_2;
      /* indicate that following brace should be on same line */
      if (parser_state_tos->p_stack[parser_state_tos->tos] != decl)
	{			/* only put one declaration onto stack */
	  break_comma = true;	/* while in declaration, newline should be
				   forced after comma */
	  inc_pstack ();
	  parser_state_tos->p_stack[parser_state_tos->tos] = decl;
	  parser_state_tos->il[parser_state_tos->tos] = parser_state_tos->i_l_follow;

	  if (ljust_decl)
	    {			/* only do if we want left justified
				   declarations */
	      int i;
	      parser_state_tos->ind_level = 0;
	      for (i = parser_state_tos->tos - 1; i > 0; --i)
		if (parser_state_tos->p_stack[i] == decl)
		  /* indentation is number of declaration levels deep we are
		     times spaces per level */
		  parser_state_tos->ind_level += ind_size;
	      parser_state_tos->i_l_follow = parser_state_tos->ind_level;
	    }
	}
      break;

    case ifstmt:		/* scanned if (...) */
      if (parser_state_tos->p_stack[parser_state_tos->tos] == elsehead
	  && else_if)		/* "else if ..." */
	parser_state_tos->i_l_follow
	  = parser_state_tos->il[parser_state_tos->tos];
      /* FALLTHRU */
    case dolit:		/* 'do' */
      /* FALLTHRU */
    case forstmt:		/* for (...) */
      inc_pstack ();
      parser_state_tos->p_stack[parser_state_tos->tos] = tk;
      parser_state_tos->il[parser_state_tos->tos]
	= parser_state_tos->ind_level = parser_state_tos->i_l_follow;
      parser_state_tos->i_l_follow += ind_size;		/* subsequent statements
							   should be indented 1 */
      parser_state_tos->search_brace = btype_2;
      break;

    case lbrace:		/* scanned left curly-brace */
      break_comma = false;	/* don't break comma in an initial list */
      if (parser_state_tos->p_stack[parser_state_tos->tos] == stmt
	  || parser_state_tos->p_stack[parser_state_tos->tos] == stmtl)
	/* it is a random, isolated stmt group or a declaration */
	parser_state_tos->i_l_follow += ind_size;
      else if (parser_state_tos->p_stack[parser_state_tos->tos] == decl)
	{
	  parser_state_tos->i_l_follow += ind_size;
	  if (parser_state_tos->last_rw == rw_struct_like
	      && parser_state_tos->block_init_level == 0
	      && parser_state_tos->last_token != rparen
	      && !btype_2)
	    {
	      parser_state_tos->ind_level += brace_indent;
	      parser_state_tos->i_l_follow += brace_indent;
	    }
	}
      else
	{
	  if (s_code == e_code)
	    {
	      /* only do this if there is nothing on the line */

	      parser_state_tos->ind_level -= ind_size;
	      /* it is a group as part of a while, for, etc. */

	      /* For -bl formatting, indent by brace_indent additional spaces
	       * e.g.,
	       *        if (foo == bar) { <--> brace_indent spaces
	       *        }
	       * (in this example, 4)
	       */
	      if (!btype_2)
		{
		  parser_state_tos->ind_level += brace_indent;
		  parser_state_tos->i_l_follow += brace_indent;
		}

	      if (parser_state_tos->p_stack[parser_state_tos->tos] == swstmt
		  && case_indent > 0)
		parser_state_tos->i_l_follow += case_indent;
	      /* for a switch, brace should be two levels out from the code */
	    }
	}

      inc_pstack ();
      parser_state_tos->p_stack[parser_state_tos->tos] = lbrace;
      parser_state_tos->il[parser_state_tos->tos] = parser_state_tos->ind_level;
      inc_pstack ();
      parser_state_tos->p_stack[parser_state_tos->tos] = stmt;
      /* allow null stmt between braces */
      parser_state_tos->il[parser_state_tos->tos] = parser_state_tos->i_l_follow;
      break;

    case whilestmt:		/* scanned while (...) */
      if (parser_state_tos->p_stack[parser_state_tos->tos] == dohead)
	{
	  /* it is matched with do stmt */
	  parser_state_tos->ind_level = parser_state_tos->i_l_follow
	    = parser_state_tos->il[parser_state_tos->tos];
	  inc_pstack ();
	  parser_state_tos->p_stack[parser_state_tos->tos] = whilestmt;
	  parser_state_tos->il[parser_state_tos->tos]
	    = parser_state_tos->ind_level = parser_state_tos->i_l_follow;
	}
      else
	{			/* it is a while loop */
	  inc_pstack ();
	  parser_state_tos->p_stack[parser_state_tos->tos] = whilestmt;
	  parser_state_tos->il[parser_state_tos->tos] = parser_state_tos->i_l_follow;
	  parser_state_tos->i_l_follow += ind_size;
	  parser_state_tos->search_brace = btype_2;
	}

      break;

    case elselit:		/* scanned an else */

      if (parser_state_tos->p_stack[parser_state_tos->tos] != ifhead)
	{
	  message (0, "Unmatched 'else' (stack has %s rather than %s)",
		   parsecode2s (parser_state_tos->p_stack[parser_state_tos->tos]),
		   parsecode2s (ifhead));
	}
      else
	{
	  /* indentation for else should be same as for if */
	  parser_state_tos->ind_level
	    = parser_state_tos->il[parser_state_tos->tos];
	  /* everything following should be in 1 level */
	  parser_state_tos->i_l_follow = (parser_state_tos->ind_level
					  + ind_size);

	  parser_state_tos->p_stack[parser_state_tos->tos] = elsehead;
	  /* remember if with else */
	  parser_state_tos->search_brace = btype_2 | else_if;
	}
      break;

    case rbrace:		/* scanned a right curly-brace */
      /* stack should have <lbrace> <stmt> or <lbrace> <stmtl> */
      if (parser_state_tos->p_stack[parser_state_tos->tos - 1] == lbrace)
	{
	  parser_state_tos->ind_level = parser_state_tos->i_l_follow
	    = parser_state_tos->il[--parser_state_tos->tos];
	  parser_state_tos->p_stack[parser_state_tos->tos] = stmt;
	}
      else if (parser_state_tos->p_stack[parser_state_tos->tos - 1] == stmtl)
	{
	  message (token_col (), "Did not expect a statement here");
	  parser_state_tos->ind_level = parser_state_tos->i_l_follow
	    = parser_state_tos->il[--parser_state_tos->tos];
	}
      else
	{
	  message (0, "After %s, expected %s on stack, not %s",
		   parsecode2s (rbrace),
		   parsecode2s (lbrace),
		   parsecode2s
		   (parser_state_tos->p_stack[parser_state_tos->tos - 1]));
	}
      break;

    case swstmt:		/* had switch (...) */
      inc_pstack ();
      parser_state_tos->p_stack[parser_state_tos->tos] = swstmt;
      parser_state_tos->cstk[parser_state_tos->tos]
	= case_indent + parser_state_tos->i_l_follow;
      if (!btype_2)
	parser_state_tos->cstk[parser_state_tos->tos] += brace_indent;

      /* save current case indent level */
      parser_state_tos->il[parser_state_tos->tos]
	= parser_state_tos->i_l_follow;
      /* case labels should be one level down from switch, plus
         `case_indent' if any.  Then, statements should be the `ind_size'
         further. */
      parser_state_tos->i_l_follow += ind_size;
      parser_state_tos->search_brace = btype_2;
      break;

    case semicolon:		/* this indicates a simple stmt */
      break_comma = false;	/* turn off flag to break after commas in a
				   declaration */
      if (parser_state_tos->p_stack[parser_state_tos->tos] == dostmt)
	{
	  parser_state_tos->p_stack[parser_state_tos->tos] = stmt;
	}
      else
	{
	  inc_pstack ();
	  parser_state_tos->p_stack[parser_state_tos->tos] = stmt;
	  parser_state_tos->il[parser_state_tos->tos]
	    = parser_state_tos->ind_level;
	}
      break;

      /* This is a fatal error which causes the program to exit. */
    default:
      fatal ("Unknown code to parser");
    }

  reduce ();			/* see if any reduction can be done */

  show_pstack ();

  return total_success;
}

/* NAME: reduce

FUNCTION: Implements the reduce part of the parsing algorithm

ALGORITHM: The following reductions are done.  Reductions are repeated until
   no more are possible.

   Old TOS                   New TOS
   <stmt> <stmt>             <stmtl>
   <stmtl> <stmt>            <stmtl>
   do <stmt>                 dohead
   <dohead> <whilestmt>      <dostmt>
   if <stmt>                 "ifstmt"
   switch <stmt>             <stmt>
   decl <stmt>               <stmt>
   "ifelse" <stmt>           <stmt>
   for <stmt>                <stmt>
   while <stmt>              <stmt>
   "dostmt" while            <stmt>

On each reduction, parser_state_tos->i_l_follow (the indentation for the
   following line) is set to the indentation level associated with the old
   TOS.

PARAMETERS: None

RETURNS: Nothing

GLOBALS: parser_state_tos->cstk parser_state_tos->i_l_follow =
   parser_state_tos->il parser_state_tos->p_stack = parser_state_tos->tos =

CALLS: None

CALLED BY: parse

HISTORY: initial coding 	November 1976	D A Willcox of CAC

*/
/*----------------------------------------------*\
|   REDUCTION PHASE				    |
\*----------------------------------------------*/

void
reduce (void)
{
  int i;

  for (;;)
    {				/* keep looping until there is nothing left
				   to reduce */

      switch (parser_state_tos->p_stack[parser_state_tos->tos])
	{

	case stmt:
	  switch (parser_state_tos->p_stack[parser_state_tos->tos - 1])
	    {

	    case stmt:
	    case stmtl:
	      /* stmtl stmt or stmt stmt */
	      parser_state_tos->p_stack[--parser_state_tos->tos] = stmtl;
	      break;

	    case dolit:	/* <do> <stmt> */
	      parser_state_tos->p_stack[--parser_state_tos->tos] = dohead;
	      parser_state_tos->i_l_follow
		= parser_state_tos->il[parser_state_tos->tos];
	      break;

	    case ifstmt:
	      /* <if> <stmt> */
	      parser_state_tos->p_stack[--parser_state_tos->tos] = ifhead;
	      for (i = parser_state_tos->tos - 1;
		   (parser_state_tos->p_stack[i] != stmt
		    && parser_state_tos->p_stack[i] != stmtl
		    && parser_state_tos->p_stack[i] != lbrace);
		   --i);
	      parser_state_tos->i_l_follow = parser_state_tos->il[i];
	      /* for the time being, we will assume that there is no else on
	         this if, and set the indentation level accordingly. If an
	         else is scanned, it will be fixed up later */
	      break;

	    case swstmt:
	      /* <switch> <stmt> */

	    case decl:		/* finish of a declaration */
	    case elsehead:
	      /* <<if> <stmt> else> <stmt> */
	    case forstmt:
	      /* <for> <stmt> */
	    case whilestmt:
	      /* <while> <stmt> */
	      parser_state_tos->p_stack[--parser_state_tos->tos] = stmt;
	      parser_state_tos->i_l_follow = parser_state_tos->il[parser_state_tos->tos];
	      break;

	    default:		/* <anything else> <stmt> */
	      return;

	    }			/* end of section for <stmt> on top of stack */
	  break;

	case whilestmt:	/* while (...) on top */
	  if (parser_state_tos->p_stack[parser_state_tos->tos - 1] == dohead)
	    {
	      /* it is termination of a do while */
	      parser_state_tos->p_stack[--parser_state_tos->tos] = dostmt;
	      break;
	    }
	  else
	    return;

	default:		/* anything else on top */
	  return;

	}
    }
}

/* This kludge is called from main.  It is just like parse(semicolon) except
   that it does not clear break_comma.  Leaving break_comma alone is
   necessary to make sure that "int foo(), bar()" gets formatted correctly
   under -bc.  */

void
parse_lparen_in_decl (void)
{
  inc_pstack ();
  parser_state_tos->p_stack[parser_state_tos->tos] = stmt;
  parser_state_tos->il[parser_state_tos->tos] = parser_state_tos->ind_level;

  reduce ();
}
