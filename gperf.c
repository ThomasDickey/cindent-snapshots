/* C code produced by gperf version 2.7.1 (19981006 egcs) */
/* Command-line: gperf -c -p -t -T -g -j1 -o -K rwd -N is_reserved indent.gperf  */
/* Command-line: gperf -c -p -t -T -g -j1 -o -K rwd -N is_reserved indent.gperf  */

#define TOTAL_KEYWORDS 31
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 8
#define MIN_HASH_VALUE 4
#define MAX_HASH_VALUE 36
/* maximum key range = 33, duplicates = 0 */

#ifdef __GNUC__
__inline
#endif
static unsigned int
hash (str, len)
     register const char *str;
     register unsigned int len;
{
  static unsigned char asso_values[] =
    {
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 27, 11,
      14,  0,  7,  4, 25, 24, 37,  0,  0, 25,
      16, 20, 37, 37,  8,  0,  0,  4,  1,  6,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37, 37, 37, 37, 37,
      37, 37, 37, 37, 37, 37
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#endif
struct templ *
is_reserved (str, len)
     register const char *str;
     register unsigned int len;
{
  static struct templ wordlist[] =
    {
      {""}, {""}, {""}, {""},
      {"else", rw_sp_nparen,},
      {"short", rw_decl,},
      {"struct", rw_struct_like,},
      {"va_dcl", rw_decl,},
      {"long", rw_decl,},
      {"volatile", rw_decl,},
      {"global", rw_decl,},
      {"while", rw_sp_paren,},
      {"float", rw_decl,},
      {"sizeof", rw_sizeof,},
      {"typedef", rw_decl,},
      {"case", rw_case,},
      {"const", rw_decl,},
      {"static", rw_decl,},
      {"for", rw_sp_paren,},
      {"void", rw_decl,},
      {"double", rw_decl,},
      {"default", rw_case,},
      {"extern", rw_decl,},
      {"char", rw_decl,},
      {"register", rw_decl,},
      {"union", rw_struct_like,},
      {"unsigned", rw_decl,},
      {"int", rw_decl,},
      {"goto", rw_break,},
      {"enum", rw_struct_like,},
      {"return", rw_return,},
      {"switch", rw_switch,},
      {"break", rw_break,},
      {"if", rw_sp_paren,},
      {""}, {""},
      {"do", rw_sp_nparen,}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].rwd;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1))
            return &wordlist[key];
        }
    }
  return 0;
}
