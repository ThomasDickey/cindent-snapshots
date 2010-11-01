/*
   Copyright 1999-2002,2010, Thomas E. Dickey

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
   OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE. */


/* Argument scanning and profile reading code.  Default parameters are set
   here as well. */

#include "sys.h"
#include "indent.h"
#include "version.h"

int else_endif_col;

/* profile types */
enum profile
  {
    PRO_BOOL,			/* boolean */
    PRO_INT,			/* integer */
    PRO_FONT,			/* troff font */
    PRO_IGN,			/* ignore it */
    PRO_KEY,			/* -T switch */
    PRO_SETTINGS,		/* bundled set of settings */
    PRO_PRSTRING,		/* Print string and exit */
    PRO_FUNCTION		/* Call the associated function. */
  };

/* profile specials for booleans */
enum on_or_off
  {
    ONOFF_NA,			/* Not applicable.  Used in table for
				   non-booleans.  */
    OFF,			/* This option turns on the boolean variable
				   in question.  */
    ON				/* it turns it off */
  };

/* Explicit flags for each option.  */
static int exp_D = 0;
static int exp_T = 0;
static int exp_bacc = 0;
static int exp_bad = 0;
static int exp_badp = 0;
static int exp_bap = 0;
static int exp_bbb = 0;
static int exp_bc = 0;
static int exp_bl = 0;
static int exp_bli = 0;
static int exp_bs = 0;
static int exp_c = 0;
static int exp_cd = 0;
static int exp_cdb = 0;
static int exp_ce = 0;
static int exp_ci = 0;
static int exp_cli = 0;
static int exp_cp = 0;
static int exp_cs = 0;
static int exp_d = 0;
static int exp_di = 0;
static int exp_dj = 0;
static int exp_eei = 0;
static int exp_ei = 0;
static int exp_fb = 0;
static int exp_fbc = 0;
static int exp_fbx = 0;
static int exp_fc = 0;
static int exp_fc1 = 0;
static int exp_fca = 0;
static int exp_fk = 0;
static int exp_fs = 0;
static int exp_gnu = 0;
static int exp_i = 0;
static int exp_ip = 0;
static int exp_kr = 0;
static int exp_l = 0;
static int exp_lc = 0;
static int exp_lp = 0;
static int exp_lps = 0;
static int exp_ly = 0;
static int exp_nip = 0;
static int exp_orig = 0;
static int exp_pcs = 0;
static int exp_pro = 0;
static int exp_psl = 0;
static int exp_sc = 0;
static int exp_sob = 0;
static int exp_ss = 0;
static int exp_st = 0;
static int exp_ts = 0;
static int exp_v = 0;
static int exp_version = 0;

/* The following variables are controlled by command line parameters and
   their meaning is explained in indent.h.  */
int leave_comma;
int decl_com_ind;
int case_indent;
int com_ind;
int decl_indent;
int ljust_decl;
int unindent_displace;
int else_if;
int indent_parameters;
int ind_size;
int tabsize;
int blanklines_after_procs;
int use_stdout;
int blanklines_after_declarations;
int blanklines_before_blockcomments;
int blanklines_around_conditional_compilation;
int swallow_optional_blanklines;
int n_real_blanklines;
int prefix_blankline_requested;
int postfix_blankline_requested;
int brace_indent;
int btype_2;

int space_sp_semicolon;
int max_col;
int verbose;
int cuddle_else;
int star_comment_cont;
int comment_delimiter_on_blankline;
int troff;
int procnames_start_line;
int proc_calls_space;
int cast_space;
int format_col1_comments;
int format_comments;
int continuation_indent;
int lex_or_yacc;
int lineup_to_parens;
int leave_preproc_space;
int blank_after_sizeof;
int blanklines_after_declarations_at_proctop;
int comment_max_col;
int extra_expression_indent;
int space_after_pointer_type;

int debug;
int expect_output_file;

#define NumberData(p) ((p)->d_number)
#define FunctnData(p) ((p)->d_functn)
#define StringData(p) ((p)->d_string)

/* N.B.: because of the way the table here is scanned, options whose names
   are substrings of other options must occur later; that is, with -lp vs -l,
   -lp must be first.  Also, while (most) booleans occur more than once, the
   last default value is the one actually assigned. */
struct pro
  {
    const char *p_name;		/* option name, e.g. "-bl", "-cli" */
    enum profile p_type;
    int p_default;		/* the default value (if int) */

    /* If p_type == PRO_BOOL, ON or OFF to tell how this switch affects the
       variable. Not used for other p_type's.  */
    enum on_or_off p_special;

    /* if p_type == PRO_SETTINGS, a (char *) pointing to a list of the switches
       to set, separated by NULs, terminated by 2 NULs. if p_type == PRO_BOOL,
       PRO_INT, or PRO_FONT, address of the variable that gets set by the
       option. if p_type == PRO_PRSTRING, a (char *) pointing to the string.
       if p_type == PRO_FUNCTION, a pointer to a function to be called. */
    int *d_number;
    const char *d_string;
    void (*d_functn) (void);

    /* Points to a nonzero value (allocated statically for all options) if the
       option has been specified explicitly.  This is necessary because for
       boolean options, the options to set and reset the variable must share
       the explicit flag.  */
    int *p_explicit;
  };

#define INIT_BOOL(name, dft, subtype, obj, flag) \
  {name, PRO_BOOL, dft, subtype, obj,0,0, flag}

#define INIT_FONT(name, dft, subtype, obj, flag) \
  {name, PRO_FONT, dft, subtype, obj,0,0, flag}

#define INIT_FUNCTION(name, dft, subtype, obj, flag) \
  {name, PRO_FUNCTION, dft, subtype, 0,0,obj, flag}

#define INIT_IGN(name, dft, subtype, obj, flag) \
  {name, PRO_IGN, dft, subtype, obj,0,0, flag}

#define INIT_INT(name, dft, subtype, obj, flag) \
  {name, PRO_INT, dft, subtype, obj,0,0, flag}

#define INIT_KEY(name, dft, subtype, obj, flag) \
  {name, PRO_KEY, dft, subtype, obj,0,0, flag}

#define INIT_PRSTRING(name, dft, subtype, obj, flag) \
  {name, PRO_PRSTRING, dft, subtype, 0,obj,0, flag}

#define INIT_SETTINGS(name, dft, subtype, obj, flag) \
  {name, PRO_SETTINGS, dft, subtype, 0,obj,0, flag}

#define SETTINGS_BSD \
"-nbap\0-nbad\0-bc\0-br\0-c33\0-cd33\0-cdb\0-ce\0-ci4\0\
-cli0\0-cp33\0-di16\0-fc1\0-fca\0-i4\0-ip4\0-l75\0-lp\0\
-npcs\0-psl\0-sc\0-nsob\0-nss\0-ts8\0"

#define SETTINGS_GNU \
"-nbad\0-bap\0-nbc\0-bl\0-ncdb\0-cs\0-nce\0-di2\0-ndj\0\
-ei\0-nfc1\0-i2\0-ip5\0-lp\0-pcs\0-npsl\0-psl\0-nsc\0-nsob\0-bli2\0\
-cp1\0-nfca\0"

#define SETTINGS_KR \
"-nbad\0-bap\0-nbc\0-br\0-c33\0-cd33\0-ncdb\0-ce\0\
-ci4\0-cli0\0-d0\0-di1\0-nfc1\0-i4\0-ip0\0-l75\0-lp\0-npcs\0-npsl\0-cs\0\
-nsc\0-nsob\0-nfca\0-cp33\0-nss\0"

/* Settings for original defaults vs gnu */
#ifdef BERKELEY_DEFAULTS
#define DFT_BAP  false
#define DFT_BLI  0
#define DFT_BR   true
#define DFT_CDB  true
#define DFT_CE   true
#define DFT_CI   4
#define DFT_CP   33
#define DFT_DI   16
#define DFT_FC1  true
#define DFT_FCA  true
#define DFT_IP   4
#define DFT_I    4
#define DFT_PCS  false
#define DFT_SC   true
#else
#define DFT_BAP  true
#define DFT_BLI  2
#define DFT_BR   false
#define DFT_CDB  false
#define DFT_CE   false
#define DFT_CI   0
#define DFT_CP   1
#define DFT_DI   2
#define DFT_FC1  false
#define DFT_FCA  false
#define DFT_IP   5
#define DFT_I    2
#define DFT_PCS  true
#define DFT_SC   false
#endif

/* other settings (note that the "n" options use the same default) */
#define DFT_BACC false
#define DFT_BADP false
#define DFT_BAD  false
#define DFT_BBB  false
#define DFT_BC   true
#define DFT_BS   false
#define DFT_CS   true
#define DFT_DJ   false
#define DFT_EEI  false
#define DFT_EI   true
#define DFT_LPS  true
#define DFT_LP   true
#define DFT_PSL  true
#define DFT_SOB  false
#define DFT_SS   false
#define DFT_V    false

struct pro pro[] =
{
  INIT_INT ("D", 0, ONOFF_NA, &debug, &exp_D),
  INIT_KEY ("T", 0, ONOFF_NA, 0, &exp_T),
  INIT_BOOL ("bacc", DFT_BACC, ON,
	     &blanklines_around_conditional_compilation, &exp_bacc),
  INIT_BOOL ("badp", DFT_BADP, ON,
	     &blanklines_after_declarations_at_proctop, &exp_badp),
  INIT_BOOL ("bad", DFT_BAD, ON, &blanklines_after_declarations, &exp_bad),
  INIT_BOOL ("bap", DFT_BAP, ON, &blanklines_after_procs, &exp_bap),
  INIT_BOOL ("bbb", DFT_BBB, ON, &blanklines_before_blockcomments, &exp_bbb),
  INIT_BOOL ("bc", DFT_BC, OFF, &leave_comma, &exp_bc),
  INIT_INT ("bli", DFT_BLI, ONOFF_NA, &brace_indent, &exp_bli),
  INIT_BOOL ("bl", true, OFF, &btype_2, &exp_bl),
  INIT_BOOL ("br", DFT_BR, ON, &btype_2, &exp_bl),
  INIT_BOOL ("bs", DFT_BS, ON, &blank_after_sizeof, &exp_bs),
  INIT_BOOL ("cdb", DFT_CDB, ON, &comment_delimiter_on_blankline, &exp_cdb),
  INIT_INT ("cd", 33, ONOFF_NA, &decl_com_ind, &exp_cd),
  INIT_BOOL ("ce", DFT_CE, ON, &cuddle_else, &exp_ce),
  INIT_INT ("ci", DFT_CI, ONOFF_NA, &continuation_indent, &exp_ci),
  INIT_INT ("cli", 0, ONOFF_NA, &case_indent, &exp_cli),
  INIT_INT ("cp", DFT_CP, ONOFF_NA, &else_endif_col, &exp_cp),
  INIT_BOOL ("cs", DFT_CS, ON, &cast_space, &exp_cs),
  INIT_INT ("c", 33, ONOFF_NA, &com_ind, &exp_c),
  INIT_INT ("di", DFT_DI, ONOFF_NA, &decl_indent, &exp_di),
  INIT_BOOL ("dj", DFT_DJ, ON, &ljust_decl, &exp_dj),
  INIT_INT ("d", 0, ONOFF_NA, &unindent_displace, &exp_d),
  INIT_BOOL ("eei", DFT_EEI, ON, &extra_expression_indent, &exp_eei),
  INIT_BOOL ("ei", DFT_EI, ON, &else_if, &exp_ei),
  INIT_FONT ("fbc", 0, ONOFF_NA, (int *) &blkcomf, &exp_fbc),
  INIT_FONT ("fbx", 0, ONOFF_NA, (int *) &boxcomf, &exp_fbx),
  INIT_FONT ("fb", 0, ONOFF_NA, (int *) &bodyf, &exp_fb),
  INIT_BOOL ("fc1", DFT_FC1, ON, &format_col1_comments, &exp_fc1),
  INIT_BOOL ("fca", DFT_FCA, ON, &format_comments, &exp_fca),
  INIT_FONT ("fc", 0, ONOFF_NA, (int *) &scomf, &exp_fc),
  INIT_FONT ("fk", 0, ONOFF_NA, (int *) &keywordf, &exp_fk),
  INIT_FONT ("fs", 0, ONOFF_NA, (int *) &stringf, &exp_fs),
  INIT_SETTINGS ("gnu", 0, ONOFF_NA, SETTINGS_GNU, &exp_gnu),
  INIT_FUNCTION ("h", 0, ONOFF_NA, usage, &exp_version),
  INIT_INT ("ip", DFT_IP, ON, &indent_parameters, &exp_ip),
  INIT_INT ("i", DFT_I, ONOFF_NA, &ind_size, &exp_i),
  INIT_SETTINGS ("kr", 0, ONOFF_NA, SETTINGS_KR, &exp_kr),
  INIT_INT ("lc", DEFAULT_RIGHT_COMMENT_MARGIN,
	    ONOFF_NA, &comment_max_col, &exp_lc),
  INIT_BOOL ("lps", DFT_LPS, ON, &leave_preproc_space, &exp_lps),
  INIT_BOOL ("lp", DFT_LP, ON, &lineup_to_parens, &exp_lp),
  INIT_BOOL ("ly", false, ON, &lex_or_yacc, &exp_ly),
  INIT_INT ("l", DEFAULT_RIGHT_MARGIN, ONOFF_NA, &max_col, &exp_l),
  INIT_BOOL ("nbacc", DFT_BACC, OFF,
	     &blanklines_around_conditional_compilation, &exp_bacc),
  INIT_BOOL ("nbadp", DFT_BADP, OFF,
	     &blanklines_after_declarations_at_proctop, &exp_badp),
  INIT_BOOL ("nbad", DFT_BAD, OFF, &blanklines_after_declarations, &exp_bad),
  INIT_BOOL ("nbap", DFT_BAP, OFF, &blanklines_after_procs, &exp_bap),
  INIT_BOOL ("nbbb", DFT_BBB, OFF, &blanklines_before_blockcomments, &exp_bbb),
  INIT_BOOL ("nbc", DFT_BC, ON, &leave_comma, &exp_bc),
  INIT_BOOL ("nbs", DFT_BS, OFF, &blank_after_sizeof, &exp_bs),
  INIT_BOOL ("ncdb", DFT_CDB, OFF, &comment_delimiter_on_blankline, &exp_cdb),
  INIT_BOOL ("nce", DFT_CE, OFF, &cuddle_else, &exp_ce),
  INIT_BOOL ("ncs", DFT_CS, OFF, &cast_space, &exp_cs),
  INIT_BOOL ("ndj", DFT_DJ, OFF, &ljust_decl, &exp_dj),
  INIT_BOOL ("neei", DFT_EEI, OFF, &extra_expression_indent, &exp_eei),
  INIT_BOOL ("nei", DFT_EI, OFF, &else_if, &exp_ei),
  INIT_BOOL ("nfc1", DFT_FC1, OFF, &format_col1_comments, &exp_fc1),
  INIT_BOOL ("nfca", DFT_FCA, OFF, &format_comments, &exp_fca),
  INIT_SETTINGS ("nip", 0, ONOFF_NA, "-ip0\0", &exp_nip),
  INIT_BOOL ("nlps", DFT_LPS, OFF, &leave_preproc_space, &exp_lps),
  INIT_BOOL ("nlp", DFT_LP, OFF, &lineup_to_parens, &exp_lp),
  INIT_BOOL ("npcs", DFT_PCS, OFF, &proc_calls_space, &exp_pcs),
  INIT_IGN ("npro", 0, ONOFF_NA, 0, &exp_pro),
  INIT_BOOL ("npsl", DFT_PSL, OFF, &procnames_start_line, &exp_psl),
  INIT_BOOL ("nsc", DFT_SC, OFF, &star_comment_cont, &exp_sc),
  INIT_BOOL ("nsob", DFT_SOB, OFF, &swallow_optional_blanklines, &exp_sob),
  INIT_BOOL ("nss", DFT_SS, OFF, &space_sp_semicolon, &exp_ss),
  INIT_BOOL ("nv", DFT_V, OFF, &verbose, &exp_v),
  INIT_SETTINGS ("orig", 0, ONOFF_NA, SETTINGS_BSD, &exp_orig),
  INIT_BOOL ("o", false, ON, &expect_output_file, &expect_output_file),
  INIT_BOOL ("pcs", DFT_PCS, ON, &proc_calls_space, &exp_pcs),
  INIT_BOOL ("psl", DFT_PSL, ON, &procnames_start_line, &exp_psl),
  INIT_BOOL ("sc", DFT_SC, ON, &star_comment_cont, &exp_sc),
  INIT_BOOL ("sob", DFT_SOB, ON, &swallow_optional_blanklines, &exp_sob),
  INIT_BOOL ("ss", DFT_SS, ON, &space_sp_semicolon, &exp_ss),
  INIT_BOOL ("st", false, ON, &use_stdout, &exp_st),
  INIT_INT ("ts", 8, ONOFF_NA, &tabsize, &exp_ts),
  INIT_PRSTRING ("version", 0, ONOFF_NA, VERSION_STRING, &exp_version),
  INIT_BOOL ("v", DFT_V, ON, &verbose, &exp_v),

/* Signify end of structure.  */
  INIT_IGN (0, 0, ONOFF_NA, 0, 0)
};

struct long_option_conversion
  {
    const char *long_name;
    const char *short_name;
  };

struct long_option_conversion option_conversions[] =
{
  {"blank-lines-after-ifdefs", "bacc"},
  {"blank-lines-after-procedure-declarations", "badp"},
  {"blank-lines-after-declarations", "bad"},
  {"blank-lines-after-procedures", "bap"},
  {"blank-lines-after-block-comments", "bbb"},
  {"blank-lines-after-commas", "bc"},
  {"brace-indent", "bli"},
  {"braces-after-if-line", "bl"},
  {"braces-on-if-line", "br"},
  {"Bill-Shannon", "bs"},
  {"blank-before-sizeof", "bs"},
  {"comment-delimiters-on-blank-lines", "cdb"},
  {"declaration-comment-column", "cd"},
  {"cuddle-else", "ce"},
  {"continuation-indentation", "ci"},
  {"case-indentation", "cli"},
  {"else-endif-column", "cp"},
  {"space-after-cast", "cs"},
  {"comment-indentation", "c"},
  {"debug", "D"},
  {"declaration-indentation", "di"},
  {"left-justify-declarations", "dj"},
  {"line-comments-indentation", "d"},
  {"extra-expression-indentation", "eei"},
  {"else-if", "ei"},
  {"*", "fbc"},
  {"*", "fbx"},
  {"*", "fb"},
  {"format-first-column-comments", "fc1"},
  {"format-all-comments", "fca"},
  {"*", "fc"},
  {"*", "fk"},
  {"*", "fs"},
  {"gnu-style", "gnu"},
  {"help", "h"},
  {"usage", "h"},
  {"parameter-indentation", "ip"},
  {"indentation-level", "i"},
  {"indent-level", "i"},
  {"k-and-r-style", "kr"},
  {"kernighan-and-ritchie-style", "kr"},
  {"kernighan-and-ritchie", "kr"},
  {"comment-line-length", "lc"},
  {"lex-or-yacc", "ly"},
  {"continue-at-parentheses", "lp"},
  {"leave-preprocessor-space", "lps"},
  {"remove-preprocessor-space", "nlps"},
  {"line-length", "l"},
  {"no-blank-lines-after-ifdefs", "nbacc"},
  {"no-blank-lines-after-procedure-declarations", "nbadp"},
  {"no-blank-lines-after-declarations", "nbad"},
  {"no-blank-lines-after-procedures", "nbap"},
  {"no-blank-lines-after-block-comments", "nbbb"},
  {"no-blank-lines-after-commas", "nbc"},
  {"no-Bill-Shannon", "nbs"},
  {"no-blank-before-sizeof", "nbs"},
  {"no-comment-delimiters-on-blank-lines", "ncdb"},
  {"dont-cuddle-else", "nce"},
  {"no-space-after-casts", "ncs"},
  {"dont-left-justify-declarations", "ndj"},
  {"no-extra-expression-indentation", "neei"},
  {"no-else-if", "nei"},
  {"dont-format-first-column-comments", "nfc1"},
  {"dont-format-comments", "nfca"},
  {"no-parameter-indentation", "nip"},
  {"dont-indent-parameters", "nip"},
  {"dont-line-up-parentheses", "nlp"},
  {"no-space-after-function-call-names", "npcs"},
  {"ignore-profile", "npro"},
  {"dont-break-procedure-type", "npsl"},
  {"dont-star-comments", "nsc"},
  {"leave-optional-blank-lines", "nsob"},
  {"dont-space-special-semicolon", "nss"},
  {"no-verbosity", "nv"},
  {"output", "o"},
  {"output-file", "o"},
  {"original", "orig"},
  {"original-style", "orig"},
  {"berkeley-style", "orig"},
  {"berkeley", "orig"},
  {"space-after-procedure-calls", "pcs"},
  {"procnames-start-lines", "psl"},
  {"start-left-side-of-comments", "sc"},
  {"swallow-optional-blank-lines", "sob"},
  {"space-special-semicolon", "ss"},
  {"standard-output", "st"},
  {"tab-size", "ts"},
  {"version", "version"},
  {"verbose", "v"},
  {0, 0},
};

/* S1 should be a string.  S2 should be a string, perhaps followed by an
   argument.  Compare the two, returning true if they are equal, and if they
   are equal set *START_PARAM to point to the argument in S2.  */

static int
eqin (
       const char *s1,
       const char *s2,
       const char **start_param)
{
  while (*s1)
    {
      if (*s1++ != *s2++)
	return (false);
    }
  *start_param = s2;
  return (true);
}

/* Set the defaults. */
void
set_defaults (void)
{
  struct pro *p;

  for (p = pro; p->p_name; p++)
    if (p->p_type == PRO_BOOL || p->p_type == PRO_INT)
      *NumberData (p) = p->p_default;
}

/* Stings which can prefix an option, longest first. */
static const char *option_prefixes[] =
{
  "--",
  "-",
  "+",
  0
};

static int
option_prefix (const char *arg)
{
  const char **prefixes = option_prefixes;
  const char *this_prefix;
  const char *argp;

  do
    {
      this_prefix = *prefixes;
      argp = arg;
      while (*this_prefix == *argp)
	{
	  this_prefix++;
	  argp++;
	}
      if (*this_prefix == '\0')
	return (int) (this_prefix - *prefixes);
    }
  while (*++prefixes);

  return 0;
}

/* Process an option ARG (e.g. "-l60").  PARAM is a possible value
   for ARG, if PARAM is nonzero.  EXPLICT should be nonzero iff the
   argument is being explicitly specified (as opposed to being taken from a
   PRO_SETTINGS group of settings).

   Returns 1 if the option had a value, returns 0 otherwise. */

int
set_option (const char *option, const char *param, int explicit)
{
  struct pro *p = pro;
  const char *param_start;
  int option_length, val;

  val = 0;
  option_length = option_prefix (option);
  if (option_length > 0)
    {
      if (option_length == 1 && *option == '-')
	/* Short option prefix */
	{
	  option++;
	  for (p = pro; p->p_name; p++)
	    if (*p->p_name == *option
		&& eqin (p->p_name, option, &param_start))
	      goto found;
	}
      else
	/* Long prefix */
	{
	  struct long_option_conversion *o = option_conversions;
	  option += option_length;

	  while (o->short_name)
	    {
	      if (eqin (o->long_name, option, &param_start))
		break;
	      o++;
	    }

	  /* Searching twice means we don't have to keep the two tables in
	     sync. */
	  if (o->short_name)
	    for (p = pro; p->p_name; p++)
	      if (!strcmp (p->p_name, o->short_name))
		goto found;
	}
    }

  fprintf (stderr, "%s: unknown option \"%s\"\n", progname, option - 1);
  exit (invocation_error);

arg_missing:
  fprintf (stderr, "%s: missing argument to parameter %s\n", progname, option);
  exit (invocation_error);

found:
  /* If the parameter has been explicitly specified, we don't
     want a group of bundled settings to override the explicit
     setting.  */
  if (verbose)
    fprintf (stderr, "option: %s\n", p->p_name);
  if (explicit || !*(p->p_explicit))
    {
      if (explicit)
	*(p->p_explicit) = 1;

      switch (p->p_type)
	{

	case PRO_PRSTRING:
	  /* This is not really an error, but zero exit values are
	     returned only when code has been successfully formatted. */
	  puts (StringData (p));
	  exit (invocation_error);
	  /* NOTREACHED */

	case PRO_FUNCTION:
	  FunctnData (p) ();
	  break;

	case PRO_SETTINGS:
	  {
	    const char *t;	/* current position */

	    t = StringData (p);
	    do
	      {
		set_option (t, 0, 0);
		/* advance to character following next NUL */
		while (*t++);
	      }
	    while (*t);
	  }
	  break;

	case PRO_IGN:
	  break;

	case PRO_KEY:
	  {
	    char *str;
	    if (*param_start == 0)
	      {
		if (!(param_start = param))
		  goto arg_missing;
		else
		  val = 1;
	      }

	    str = (char *) xmalloc (strlen (param_start) + 1);
	    strcpy (str, param_start);
	    addkey (str, rw_decl);
	  }
	  break;

	case PRO_BOOL:
	  if (p->p_special == OFF)
	    *NumberData (p) = false;
	  else
	    *NumberData (p) = true;
	  break;

	case PRO_INT:
	  if (*param_start == 0)
	    {
	      if (!(param_start = param))
		goto arg_missing;
	      else
		val = 1;
	    }

	  if (!isdigit (UChar (*param_start)))
	    {
	      fprintf (stderr,
		       "%s: option ``%s'' requires a numeric parameter\n",
		       progname, option - 1);
	      exit (invocation_error);
	    }
	  *NumberData (p) = atoi (param_start);
	  break;

	default:
	  fprintf (stderr, "%s: set_option: internal error: p_type %d\n",
		   progname, (int) p->p_type);
	  exit (invocation_error);
	}
    }

  return val;
}


/* Scan the options in the file F. */

static void
scan_profile (FILE * f)
{
  int i;
  char *p, *this, *next, *temp;
  char b0[BUFSIZ];
  char b1[BUFSIZ];

  next = b0;
  this = 0;
  while (1)
    {
      for (p = next; ((i = getc (f)) != EOF
		      && (*p = (char) i) > ' '
		      && i != '/'
		      && p < next + BUFSIZ);
	   ++p);

      if (i == '/')
	{
	  i = getc (f);
	  switch (i)
	    {
	    case '/':
	      do
		i = getc (f);
	      while (i != EOF && i != EOL);
	      if (i == EOF)
		return;
	      continue;

	    case '*':
	      do
		{
		  do
		    i = getc (f);
		  while (i != EOF && i != '*');
		  if (i == '*')
		    i = getc (f);
		  if (i == EOF)
		    {
		      message (-1, "Profile contains unpalatable characters",
			       0, 0);
		      return;
		    }
		}
	      while (i != '/');
	      continue;

	    default:
	      message (-1, "Profile contains unpalatable characters");
	      if (i == EOF)
		return;
	      continue;
	    }
	}

      /* If we've scanned something... */
      if (p != next)
	{
	  *p++ = 0;
	  if (!this)
	    {
	      this = b0;
	      next = b1;
	      continue;
	    }

	  if (set_option (this, next, 1))
	    {
	      this = 0;
	      next = b0;
	      continue;
	    }

	  temp = this;
	  this = next;
	  next = temp;
	}
      else if (i == EOF)
	{
	  if (this)
	    set_option (this, 0, 1);
	  return;
	}
    }
}

#ifndef PROFILE_FORMAT
#define PROFILE_FORMAT "%s/%s"
#endif

/* set_profile looks for the given profile (usually .indent.pro) in the
   current directory, or in $HOME in that order, and reads the options
   given in that file.  Return the path of the file read.

   Note that as of version 1.3, indent only reads one file. */

char *
set_profile (const char *given)
{
  FILE *f;
  char *fname;
  char *homedir;

  if ((f = fopen (given, "r")) != NULL)
    {
      size_t len = strlen (given) + 3;

      scan_profile (f);
      (void) fclose (f);
      fname = xmalloc (len);
      if (*given != '/' && strncmp (given, "./", (size_t) 2))
	{
	  fname[0] = '.';
	  fname[1] = '/';
	  (void) memcpy (&fname[2], given, len - 3);
	  fname[len - 1] = '\0';
	}
      else
	strcpy (fname, given);
      return fname;
    }

  homedir = getenv ("HOME");
  if (homedir)
    {
      fname = xmalloc (strlen (homedir) + 10 + strlen (given) + 1);
      sprintf (fname, PROFILE_FORMAT, homedir, given);

      if ((f = fopen (fname, "r")) != NULL)
	{
	  scan_profile (f);
	  (void) fclose (f);
	  return fname;
	}

      free (fname);
    }

  return 0;
}
