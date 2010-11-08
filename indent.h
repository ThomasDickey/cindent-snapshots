/*
   Copyright 1999-2002,2010, Thomas E. Dickey

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
   OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */


/* Values that `indent' can return for exit status.

    `total_success' means no errors or warnings were found during a successful
      invocation of the program.

    `invocation_error' is returned if an invocation problem (like an incorrect
      option) prevents any formatting to occur.

    `indent_error' is returned if errors occur during formatting which
      do not prevent completion of the formatting, but which appear to be
      manifested by incorrect code (i.e, code which wouldn't compile).

    `indent_punt' is returned if formatting of a file is halted because of
      an error with the file which prevents completion of formatting.  If more
      than one input file was specified, indent continues to the next file.

    `indent_fatal' is returned if a serious internal problem occurs and
      the entire indent process is terminated, even if all specified files
      have not been processed. */

enum exit_values
  {
    total_success = 0,
    invocation_error = 1,
    indent_error = 2,
    indent_punt = 3,
    indent_fatal = 4,
    system_error = 5
  };

enum codes
  {
    code_eof = 0,		/* end of file */
    newline,
    lparen,			/* '(' or '['.  Also '{' in an
				   initialization.  */
    rparen,			/* ')' or ']'.  Also '}' in an
				   initialization.  */
    start_token,
    unary_op,
    binary_op,
    postop,

    question,
    casestmt,
    colon,
    doublecolon,		/* For C++ class methods. */
    semicolon,
    lbrace,
    rbrace,

    ident,			/* string or char literal,
				   identifier, number */

    overloaded,			/* For C++ overloaded operators (like +) */
    cpp_operator,

    comma,
    comment,			/* A "slash-star" comment */
    cplus_comment,		/* A C++ "slash-slash" */
    swstmt,
    preesc,			/* '#'.  */

    form_feed,
    decl,

    sp_paren,			/* if, for, or while token */
    sp_nparen,
    ifstmt,
    whilestmt,

    forstmt,
    stmt,
    stmtl,
    elselit,
    dolit,
    dohead,
    dostmt,
    ifhead,

    elsehead,
    struct_delim		/* '.' or "->" */
  };

enum rwcodes
  {
    rw_none,
    rw_operator,		/* For C++ operator overloading. */
    rw_break,
    rw_switch,
    rw_case,
    rw_struct_like,		/* struct, enum, union */
    rw_decl,
    rw_sp_paren,		/* if, while, for */
    rw_sp_nparen,		/* do, else */
    rw_sizeof,
    rw_return
  };

#define false 0
#define true  1

#define L_CURL '{'
#define R_CURL '}'

#define UChar(c) (0xff & ((unsigned char)(c)))

#define DEFAULT_RIGHT_MARGIN 78

#define DEFAULT_RIGHT_COMMENT_MARGIN 78

#define at_buffer_end(p) ((p) >= buf_end)
#define at_line_end(p)   (at_buffer_end(p) || (*p) == EOL)

extern const char *progname;	/* actual name of this program */

extern const char *in_name;	/* Name of input file.  */

extern char *in_prog;		/* pointer to the null-terminated input
				   program */

/* Point to the position in the input program which we are currently looking
   at.  */
extern char *in_prog_pos;

/* Point to the start of the current line.  */
extern char *cur_line;

/* Size of the input program, not including the ' \n\0' we add at the end */
extern unsigned long in_prog_size;

/* The output file. */
extern FILE *output;

/* True if we're handling C++ code. */
extern int c_plus_plus;

/* Section if we're handling lex/yacc code. */
extern int lex_section;
extern int next_lexcode;

extern char *labbuf;		/* buffer for label */
extern char *s_lab;		/* start ... */
extern char *e_lab;		/* .. and end of stored label */
extern char *l_lab;		/* limit of label buffer */

extern char *codebuf;		/* buffer for code section */
extern char *s_code;		/* start ... */
extern char *e_code;		/* .. and end of stored code */
extern char *l_code;		/* limit of code section */

extern char *combuf;		/* buffer for comments */
extern char *s_com;		/* start ... */
extern char *e_com;		/* ... and end of stored comments */
extern char *l_com;		/* limit of comment buffer */

extern char *buf_ptr;		/* ptr to next character to be taken from
				   in_buffer */
extern char *buf_end;		/* ptr to first after last char in in_buffer */

extern char *buf_break;		/* Where we can break the line. */
extern int break_line;		/* Whether or not we should break the line. */

extern int indent_eqls;		/* column on which to align "=", etc. */
extern int indent_eqls_1st;	/* reference column */

/* pointer to the token that lexi() has just found */
extern char *token;
/* points to the first char after the end of token */
extern char *token_end;
/* length of token */
extern int token_len;
/* points to the beginning of the buffer containing token */
extern char *token_buf;

/* Functions from lexi.c */
extern enum codes lexi (void);
extern int token_col (void);
extern int first_token_col (void);
extern void addkey (const char *, enum rwcodes);
extern void init_lexi (void);

/* Used to keep track of buffers.  */
struct buf
  {
    char *ptr;			/* points to the start of the buffer */
    char *end;			/* points to the character beyond the last
				   one (e.g. is equal to ptr if the buffer is
				   empty).  */
    size_t size;		/* how many chars are currently allocated.  */
    size_t len;			/* how many chars we're actually using. */
    int column;			/* Column we were in when we switched buffers. */
  };

/* Buffer in which to save a comment which occurs between an if(), while(),
   etc., and the statement following it.  Note: the fact that we point into
   this buffer, and that we might realloc() it (via the need_chars macro) is
   a bad thing (since when the buffer is realloc'd its address might change,
   making any pointers into it point to garbage), but since the filling of
   the buffer (hence the need_chars) and the using of the buffer (where
   buf_ptr points into it) occur at different times, we can get away with it
   (it would not be trivial to fix).  */
extern struct buf save_com;

extern char *bp_save;		/* saved value of buf_ptr when taking input
				   from save_com */
extern char *be_save;		/* similarly saved value of buf_end */

/*
 * Variables set by command-line options.
 */
extern int auto_typedefs;	/* true to treat "xxx_t" as typedef'd */
extern int blank_after_sizeof;	/* true iff a blank should always be inserted
				   after sizeof */
extern int blanklines_after_declarations;
extern int blanklines_after_declarations_at_proctop;
				/* This is vaguely similar
				   to blanklines_after_declarations except
				   that it only applies to the first set of
				   declarations in a procedure (just after the
				   first '{') and it causes a blank line to be
				   generated even if there are no
				   declarations */
extern int blanklines_after_procs;
extern int blanklines_around_conditional_compilation;
extern int blanklines_before_blockcomments;
extern int brace_indent;	/* number of spaces to indent braces from the
				   surrounding if, while, etc. in -bl
				   (bype_2 == 0) code */
extern int btype_2;		/* when true, brace should be on same line as
				   if, while, etc */
extern int case_indent;		/* The distance to indent case labels from
				   the switch statement */
extern int cast_space;		/* If true, casts look like:
				   (char *) bar rather than (char *)bar */
extern int com_ind;		/* the column in which comments to the right
				   of code should start */
extern int comment_delimiter_on_blankline;
extern int comment_max_col;
extern int continuation_indent;	/* set to the indentation between the edge of
				   code and continuation lines in spaces */
extern int cuddle_else;		/* true if else should cuddle up to '}' */
extern int debug;		/* when true, debugging messages are printed */
extern int decl_com_ind;	/* the column in which comments after
				   declarations should be put */
extern int decl_indent;		/* column to indent declared identifiers to */
extern int else_if;		/* True iff else if pairs should be handled
				   specially */
extern int expect_output_file;	/* Means "-o" was specified. */
extern int extra_expression_indent;
				/* True if continuation lines from
				   the expression part of "if(e)", "while(e)",
				   "for(e;e;e)" should be indented an extra tab
				   stop so that they don't conflict with the
				   code that follows */
extern int format_col1_comments;
				/* If comments which start in column 1 are to
				   be magically reformatted */
extern int format_comments;	/* If any comments are to be reformatted */
extern int ind_size;		/* The size of one indentation level in spaces.  */
extern int indent_parameters;	/* Number of spaces to indent parameters.  */
extern int leave_comma;		/* if true, never break declarations after
				   commas */
extern int leave_preproc_space;	/* if true, leave the spaces between
				   '#' and preprocessor commands. */
extern int lex_or_yacc;		/* if true, format lex/yacc source */
extern int lineup_to_parens;	/* if true, continued code within parens will
				   be lined up to the open paren */
extern int ljust_decl;		/* true if declarations should be left
				   justified */
extern int max_col;		/* the maximum allowable line length */
extern int n_real_blanklines;
extern int postfix_blankline_requested;
extern int prefix_blankline_requested;
extern int preprocessor_indentation;
				/* if nonzero, override -lps and
				   indent preprocessor keywords */
extern int proc_calls_space;	/* If true, procedure calls look like:
				   foo (bar) rather than foo(bar) */
extern int procnames_start_line;
				/* if true, the names of procedures being
				   defined get placed in column 1 (ie. a
				   newline is placed between the type of the
				   procedure and its name) */
extern int space_after_pointer_type;
				/* Space after star in:  char * foo(); */
extern int space_sp_semicolon;	/* If true, a space is inserted between if,
				   while, or for, and a semicolon
				   for example while (*p++ == ' ') ; */
extern int star_comment_cont;	/* true iff comment continuation lines should
				   have stars at the beginning of each line. */
extern int swallow_optional_blanklines;
extern int tabsize;		/* The number of columns a tab character
				   generates. */
extern int troff;		/* true iff were generating troff input */
extern int unindent_displace;	/* comments not to the right of code will be
				   placed this many indentation levels to the
				   left of code */
extern int use_stdout;
extern int verbose;		/* when true, non-essential error messages
				   are printed */

/*
 * Working state.
 */
extern int break_comma;		/* when true and not in parens, break after a
				   comma */

/* True if there is an embedded comment on this code line */
extern int embedded_comment_on_line;

extern int else_or_endif;
extern size_t di_stack_alloc;
extern int *di_stack;

/* True if a #else or #endif has been encountered.  */
extern int else_or_endif;

extern int code_lines;		/* count of lines with code */

extern int out_coms;		/* number of comments processed */
extern int out_lines;		/* the number of lines written, set by
				   dump_line */
extern int com_lines;		/* the number of lines with comments, set by
				   dump_line */


extern int had_eof;		/* set to true when input is exhausted */
extern int in_line_no;		/* the current input line number. */
extern int out_line_no;		/* the current output line number. */
extern int suppress_blanklines;	/* set iff following blanklines should be
				   suppressed */

/* The position that we will line the current line up with when it comes time
   to print it (if we are lining up to parentheses).  */
extern int paren_target;

/* Nonzero if we should use standard input/output when files are not
   explicitly specified.  */
extern int use_stdinout;

/* -troff font state information */

struct fstate
  {
    char font[4];
    char size;
    unsigned char allcaps;
  };

extern char *chfont (struct fstate *, struct fstate *, char *);

extern struct fstate
  keywordf,			/* keyword font */
  stringf,			/* string font */
  boxcomf,			/* Box comment font */
  blkcomf,			/* Block comment font */
  scomf,			/* Same line comment font */
  bodyf;			/* major body font */

/* This structure contains information relating to the state of parsing the
   code.  The difference is that the state is saved on #if and restored on
   #else.  */
struct parser_state
  {
    struct parser_state *next;
    enum codes last_token;
    struct fstate cfont;	/* Current font */

    /* This is the parsers stack, and the current allocated size.  */
    enum codes *p_stack;
    size_t p_stack_size;

    /* This stack stores indentation levels */
    /* Currently allocated size is stored in p_stack_size.  */
    int *il;

    /* true for preprocessor #if statements */
    int preprocessor_indent;

    /* If the last token was an ident and is a reserved word,
       remember the type. */
    enum rwcodes last_rw;

    /* also, remember its depth in parentheses */
    int last_rw_depth;

    /* Used to store case stmt indentation levels.  */
    /* Currently allocated size is stored in p_stack_size.  */
    int *cstk;

    /* Pointer to the top of stack of the p_stack, il and cstk arrays. */
    int tos;

    int box_com;		/* set to true when we are in a
				   "boxed" comment. In that case, the
				   first non-blank char should be
				   lined up with the / in the comment
				   closing delimiter */

    int cast_mask;		/* indicates which close parens close off
				   casts */
    /* A bit for each paren level, set if the open paren was in a context which
       indicates that this pair of parentheses is not a cast.  */
    int noncast_mask;

    int sizeof_mask;		/* indicates which close parens close off
				   sizeof''s */
    int block_init;		/* true iff inside a block initialization */
    int block_init_level;	/* The level of brace nesting in an
				   initialization */
    int last_nl;		/* this is true if the last thing scanned was
				   a newline */
    int in_or_st;		/* Will be true iff there has been a
				   declarator (e.g. int or char) and no left
				   paren since the last semicolon. When true,
				   a '{' is starting a structure definition
				   or an initialization list */
    int bl_line;		/* set to 1 by dump_line if the line is
				   blank */
    int col_1;			/* set to true if the last token started in
				   column 1 */
    int com_col;		/* this is the column in which the current
				   coment should start */
    int dec_nest;		/* current nesting level for structure or
				   init */
    int decl_on_line;		/* set to true if this line of code has part
				   of a declaration on it */
    int i_l_follow;		/* the level in spaces to which ind_level
				   should be set after the current line is
				   printed */
    int in_comment;		/* set to true while processing a C comment */
    int in_decl;		/* set to true when we are in a declaration
				   stmt.  The processing of braces is then
				   slightly different */
    int in_stmt;		/* set to 1 while in a stmt */
    int ind_level;		/* the current indentation level in spaces */
    int ind_stmt;		/* set to 1 if next line should have an extra
				   indentation level because we are in the
				   middle of a stmt */
    int inner_stmt;		/* set to true if processing ({statement}) */
    int last_u_d;		/* set to true after scanning a token which
				   forces a following operator to be unary */
    int p_l_follow;		/* used to remember how to indent following
				   statement */
    int paren_level;		/* parenthesization level. used to indent
				   within stmts */
    int paren_depth;		/* Depth of paren nesting anywhere. */
    /* Column positions of paren at each level.  If positive, it contains just
       the number of characters of code on the line up to and including the
       right parenthesis character.  If negative, it contains the opposite of
       the actual level of indentation in characters (that is, the indentation
       of the line has been added to the number of characters and the sign has
       been reversed to indicate that this has been done).  */
    int *paren_indents;		/* column positions of each paren */
    size_t paren_indents_size;	/* Currently allocated size.  */

    int pcase;			/* set to 1 if the current line label is a
				   case.  It is printed differently from a
				   regular label */
    int search_brace;		/* set to true by parse when it is necessary
				   to buffer up all info up to the start of a
				   stmt after an if, while, etc */
    int use_ff;			/* set to one if the current line should be
				   terminated with a form feed */
    int want_blank;		/* set to true when the following token
				   should be prefixed by a blank. (Said
				   prefixing is ignored in some cases.) */
    int its_a_keyword;
    int sizeof_keyword;
    int dumped_decl_indent;
    int in_parameter_declaration;
    const char *procname;	/* The name of the current procedure */
    const char *procname_end;	/* One char past the last one in procname */
    const char *classname;	/* The name of the current C++ class */
    const char *classname_end;	/* One char past the last one in classname */
    int just_saw_decl;
  };

/* All manipulations of the parser state occur at the top of stack (tos). A
   stack is kept for conditional compilation (unrelated to the p_stack, il, &
   cstk stacks)--it is implemented as a linked list via the next field.  */
extern struct parser_state *parser_state_tos;

/* The column in which comments to the right of #else and #endif should
   start.  */
extern int else_endif_col;



/* Declared in globs.c */
extern char *xmalloc (size_t);
extern char *xrealloc (char *, size_t);
extern char *xstrdup (const char *);
extern void message (int, const char *,...);
extern void fatal (const char *,...);

/* Declared in args.c */
extern char *set_profile (const char *);
extern int set_option (const char *, const char *, int);
extern void set_defaults (void);

/* Declared in comment.c */
extern void print_comment (void);

/* Declared in indent.c */
extern int squest;
extern struct file_buffer *current_input;

extern void usage (void);

/* Declared in io.c */
extern int compute_code_target (void);
extern int compute_label_target (void);
extern int count_columns (int, char *, int);
extern int current_column (void);
extern void dump_line (void);
extern void fill_buffer (void);

/* Declared in parse.c */
extern const char *parsecode2s (enum codes);
extern enum exit_values parse (enum codes);
extern int inc_pstack (void);
extern void init_parser (void);
extern void parse_lparen_in_decl (void);
extern void reduce (void);
extern void reset_parser (void);
extern void show_parser (const char *, int);
extern void show_pstack (void);
