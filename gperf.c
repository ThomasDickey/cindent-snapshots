/* *INDENT-OFF* */
/* ANSI-C code produced by gperf version 3.1 */
/* Command-line: gperf -LANSI-C -C -c -p -t -T -g -j1 -o -K rwd -N is_reserved indent.gperf  */
/* Computed positions: -k'1-2,5' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gperf@gnu.org>."
#endif

#line 1 "indent.gperf"

/* Command-line: gperf -c -p -t -T -g -j1 -o -K rwd -N is_reserved indent.gperf  */

#define TOTAL_KEYWORDS 64
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 12
#define MIN_HASH_VALUE 3
#define MAX_HASH_VALUE 82
/* maximum key range = 80, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register size_t len)
{
  static const unsigned char asso_values[] =
    {
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 22,
      55, 19, 52, 83, 20, 83, 18, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 11, 14, 83, 83,
      83, 83, 83,  0, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 23, 83,  8, 21, 11,
      29,  0,  6, 41, 37,  0,  5,  3,  1,  9,
       0, 10, 12, 83,  2,  1,  6, 33, 19,  8,
       1,  0, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83, 83, 83, 83, 83,
      83, 83, 83, 83, 83, 83
    };
  register unsigned hval = (unsigned) len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[4]];
      /*FALLTHROUGH*/
      case 4:
      case 3:
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      /*FALLTHROUGH*/
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval;
}

static const struct templ *
is_reserved (register const char *str, register size_t len)
{
  static const struct templ wordlist[] =
    {
      {"", rw_none}, {"", rw_none}, {"", rw_none},
#line 26 "indent.gperf"
      {"int", rw_decl,},
#line 18 "indent.gperf"
      {"enum", rw_struct_like,},
#line 17 "indent.gperf"
      {"else", rw_sp_nparen,},
#line 25 "indent.gperf"
      {"inline", rw_decl,},
#line 32 "indent.gperf"
      {"signed", rw_decl,},
#line 24 "indent.gperf"
      {"if", rw_sp_paren,},
#line 19 "indent.gperf"
      {"extern", rw_decl,},
#line 30 "indent.gperf"
      {"return", rw_return,},
#line 28 "indent.gperf"
      {"register", rw_decl,},
#line 29 "indent.gperf"
      {"restrict", rw_decl,},
#line 34 "indent.gperf"
      {"static", rw_decl,},
#line 52 "indent.gperf"
      {"intptr_t", rw_decl,},
#line 27 "indent.gperf"
      {"long", rw_decl,},
#line 54 "indent.gperf"
      {"intmax_t", rw_decl,},
#line 33 "indent.gperf"
      {"sizeof", rw_sizeof,},
#line 20 "indent.gperf"
      {"float", rw_decl,},
#line 21 "indent.gperf"
      {"for", rw_sp_paren,},
#line 44 "indent.gperf"
      {"float_t", rw_decl,},
#line 47 "indent.gperf"
      {"sig_atomic_t", rw_decl,},
#line 57 "indent.gperf"
      {"clock_t", rw_decl,},
#line 11 "indent.gperf"
      {"case", rw_case,},
#line 35 "indent.gperf"
      {"struct", rw_struct_like,},
#line 48 "indent.gperf"
      {"ptrdiff", rw_decl,},
#line 36 "indent.gperf"
      {"switch", rw_switch,},
#line 63 "indent.gperf"
      {"int16_t", rw_decl,},
#line 50 "indent.gperf"
      {"wchar_t", rw_decl,},
#line 62 "indent.gperf"
      {"int8_t", rw_decl,},
#line 49 "indent.gperf"
      {"size_t", rw_decl,},
#line 10 "indent.gperf"
      {"break", rw_break,},
#line 13 "indent.gperf"
      {"const", rw_decl,},
#line 41 "indent.gperf"
      {"void", rw_decl,},
#line 51 "indent.gperf"
      {"va_list", rw_decl,},
#line 58 "indent.gperf"
      {"time_t", rw_decl,},
#line 60 "indent.gperf"
      {"wctrans_t", rw_decl,},
#line 59 "indent.gperf"
      {"wint_t", rw_decl,},
#line 38 "indent.gperf"
      {"union", rw_struct_like,},
#line 61 "indent.gperf"
      {"wctype_t", rw_decl,},
#line 6 "indent.gperf"
      {"_Bool", rw_decl,},
#line 15 "indent.gperf"
      {"do", rw_sp_nparen,},
#line 37 "indent.gperf"
      {"typedef", rw_decl,},
#line 42 "indent.gperf"
      {"volatile", rw_decl,},
#line 40 "indent.gperf"
      {"va_dcl", rw_decl,},
#line 9 "indent.gperf"
      {"auto", rw_decl,},
#line 16 "indent.gperf"
      {"double", rw_decl,},
#line 56 "indent.gperf"
      {"fpos_t", rw_decl,},
#line 45 "indent.gperf"
      {"double_t", rw_decl,},
#line 31 "indent.gperf"
      {"short", rw_decl,},
#line 43 "indent.gperf"
      {"while", rw_sp_paren,},
#line 55 "indent.gperf"
      {"uintmax_t", rw_decl,},
#line 12 "indent.gperf"
      {"char", rw_decl,},
#line 46 "indent.gperf"
      {"jmpbuf", rw_decl,},
#line 53 "indent.gperf"
      {"uintptr_t", rw_decl,},
#line 23 "indent.gperf"
      {"goto", rw_break,},
#line 22 "indent.gperf"
      {"global", rw_decl,},
#line 7 "indent.gperf"
      {"_Complex", rw_decl,},
#line 66 "indent.gperf"
      {"uint8_t", rw_decl,},
#line 65 "indent.gperf"
      {"int64_t", rw_decl,},
#line 68 "indent.gperf"
      {"uint32_t", rw_decl,},
#line 69 "indent.gperf"
      {"uint64_t", rw_decl,},
#line 64 "indent.gperf"
      {"int32_t", rw_decl,},
#line 67 "indent.gperf"
      {"uint16_t", rw_decl,},
      {"", rw_none}, {"", rw_none}, {"", rw_none}, {"", rw_none}, {"", rw_none},
#line 14 "indent.gperf"
      {"default", rw_case,},
      {"", rw_none}, {"", rw_none}, {"", rw_none}, {"", rw_none},
#line 8 "indent.gperf"
      {"_Imaginary", rw_decl,},
      {"", rw_none}, {"", rw_none}, {"", rw_none}, {"", rw_none}, {"", rw_none}, {"", rw_none}, {"", rw_none},
#line 39 "indent.gperf"
      {"unsigned", rw_decl,}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register unsigned int key = hash (str, len);

      if (key <= MAX_HASH_VALUE)
        {
          register const char *s = wordlist[key].rwd;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return &wordlist[key];
        }
    }
  return NULL;
}
/* *INDENT-ON* */
