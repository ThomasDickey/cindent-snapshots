/* C code produced by gperf version 2.7.1 (19981006 egcs) */
/* Command-line: gperf -D -c -l -p -t -T -g -j1 -o -K rwd -N is_reserved_cc -H hash_cc indent-cc.gperf  */

#define TOTAL_KEYWORDS 48
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 9
#define MIN_HASH_VALUE 4
#define MAX_HASH_VALUE 70
/* maximum key range = 67, duplicates = 1 */

#ifdef __GNUC__
__inline
#endif
static unsigned int
hash_cc (str, len)
     register const char *str;
     register unsigned int len;
{
  static unsigned char asso_values[] =
    {
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71,  5, 18,
      14,  0,  9,  1, 28,  0, 71,  0,  0,  4,
      25,  7, 71, 71, 18,  4, 38, 15, 50, 27,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71, 71, 71, 71, 71,
      71, 71, 71, 71, 71, 71
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#endif
struct templ *
is_reserved_cc (str, len)
     register const char *str;
     register unsigned int len;
{
  static unsigned char lengthtable[] =
    {
       4,  4,  6,  6,  4,  4,  5,  2,  4,  9,  5,  6,  6,  6,
       4,  2,  6,  5,  6,  6,  3,  6,  5,  8,  7,  8,  6,  4,
       3,  6,  8,  5,  8,  5,  6,  6,  5,  5,  6,  7,  3,  6,
       7,  8,  7,  5,  4,  5
    };
  static struct templ wordlist[] =
    {
      {"else", rw_sp_else,},
      {"long", rw_decl,},
      {"inline", rw_decl,},
      {"global", rw_decl,},
      {"enum", rw_enum,},
      {"bool", rw_decl,},
      {"break", rw_break,},
      {"if", rw_sp_paren,},
      {"goto", rw_break,},
      {"signature", rw_struct_like,},
      {"sigof", rw_sizeof,},
      {"sizeof", rw_sizeof,},
      {"delete", rw_return,},
      {"double", rw_decl,},
      {"case", rw_case,},
      {"do", rw_sp_nparen,},
      {"signed", rw_decl,},
      {"class", rw_struct_like,},
      {"static", rw_decl,},
      {"friend", rw_decl,},
      {"for", rw_sp_paren,},
      {"extern", rw_decl,},
      {"while", rw_sp_paren,},
      {"operator", rw_operator,},
      {"classof", rw_sizeof,},
      {"unsigned", rw_decl,},
      {"switch", rw_switch,},
      {"char", rw_decl,},
      {"int", rw_decl,},
      {"headof", rw_sizeof,},
      {"register", rw_decl,},
      {"union", rw_struct_like,},
      {"template", rw_decl,},
      {"short", rw_decl,},
      {"struct", rw_struct_like,},
      {"return", rw_return,},
      {"catch", rw_sp_paren,},
      {"float", rw_decl,},
      {"typeof", rw_sizeof,},
      {"typedef", rw_decl,},
      {"new", rw_return,},
      {"va_dcl", rw_decl,},
      {"virtual", rw_decl,},
      {"volatile", rw_decl,},
      {"default", rw_case,},
      {"const", rw_decl,},
      {"void", rw_decl,},
      {"throw", rw_return,}
    };

  static short lookup[] =
    {
       -1,  -1,  -1,  -1,   0,   1,   2,   3,   4,   5,
        6,   7,   8,   9,  -1,  -1,  -1,  -1,  10,  11,
      -74,  -1,  14,  15,  16, -36,  -2,  17,  18,  19,
       20,  21,  22,  23,  24,  -1,  -1,  25,  26,  -1,
       27,  28,  -1,  29,  30,  31,  32,  33,  34,  35,
       -1,  36,  37,  38,  39,  40,  41,  42,  43,  44,
       -1,  45,  -1,  -1,  -1,  -1,  -1,  -1,  46,  -1,
       47
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash_cc (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int index = lookup[key];

          if (index >= 0)
            {
              if (len == lengthtable[index])
                {
                  register const char *s = wordlist[index].rwd;

                  if (*str == *s && !strncmp (str + 1, s + 1, len - 1))
                    return &wordlist[index];
                }
            }
          else if (index < -TOTAL_KEYWORDS)
            {
              register int offset = - 1 - TOTAL_KEYWORDS - index;
              register const unsigned char *lengthptr = &lengthtable[TOTAL_KEYWORDS + lookup[offset]];
              register struct templ *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
              register struct templ *wordendptr = wordptr + -lookup[offset + 1];

              while (wordptr < wordendptr)
                {
                  if (len == *lengthptr)
                    {
                      register const char *s = wordptr->rwd;

                      if (*str == *s && !strncmp (str + 1, s + 1, len - 1))
                        return wordptr;
                    }
                  lengthptr++;
                  wordptr++;
                }
            }
        }
    }
  return 0;
}
