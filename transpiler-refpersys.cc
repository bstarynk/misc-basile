// file misc-basile/transpiler-refpersys.cc
// SPDX-License-Identifier: GPL-3.0-or-later
/***
 *   ©  Copyright Basile Starynkevitch 2024
 *  program released under GNU General Public License
 *
 *  this is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 3, or (at your option) any later
 *  version.
 *
 *  this is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 *  License for more details.
 ***/

#include "transpiler-refpersys.hh"

const char trp_git_id[] = GIT_ID;

char* trp_prog_name;



////////////////////////////////////////////////////////////////

//// support for prime numbers copied from refpersys.org
static const int64_t trp_primes_tab[] =
{
//// piping primesieve -t18 -p 2 2333444555666
  2, 3, 5, 7,
  11, 13, 17, 19,
  23, 29, 37, 41,
  47, 53, 59, 67,
  79, 89, 101, 113,
  127, 149, 167, 191,
  211, 233, 257, 283,
  313, 347, 383, 431,
  479, 541, 599, 659,
  727, 809, 907, 1009,
  1117, 1229, 1361, 1499,
  1657, 1823, 2011, 2213,
  2437, 2683, 2953, 3251,
  3581, 3943, 4339, 4783,
  5273, 5801, 6389, 7039,
  7753, 8537, 9391, 10331,
  11369, 12511, 13763, 15149,
  16673, 18341, 20177, 22229,
  24469, 26921, 29629, 32603,
  35869, 39461, 43411, 47777,
  52561, 57829, 63617, 69991,
  76991, 84691, 93169, 102497,
  112757, 124067, 136481, 150131,
  165161, 181693, 199873, 219871,
  241861, 266051, 292661, 321947,
//#100 of 27763
  354143, 389561, 428531, 471389,
  518533, 570389, 627433, 690187,
  759223, 835207, 918733, 1010617,
  1111687, 1222889, 1345207, 1479733,
  1627723, 1790501, 1969567, 2166529,
  2383219, 2621551, 2883733, 3172123,
  3489347, 3838283, 4222117, 4644329,
  5108767, 5619667, 6181639, 6799811,
  7479803, 8227787, 9050599, 9955697,
  10951273, 12046403, 13251047, 14576161,
  16033799, 17637203, 19400929, 21341053,
  23475161, 25822679, 28404989, 31245491,
  34370053, 37807061, 41587807, 45746593,
  50321261, 55353391, 60888739, 66977621,
  73675391, 81042947, 89147249, 98061979,
  107868203, 118655027, 130520531, 143572609,
  157929907, 173722907, 191095213, 210204763,
  231225257, 254347801, 279782593, 307760897,
  338536987, 372390691, 409629809, 450592801,
  495652109, 545217341, 599739083, 659713007,
  725684317, 798252779, 878078057, 965885863,
  1062474559, 1168722059, 1285594279, 1414153729,
  1555569107, 1711126033, 1882238639, 2070462533,
  2277508787, 2505259681, 2755785653, 3031364227,
  3334500667, 3667950739, 4034745863, 4438220467,
//#200 of 209734681
  4882042547, 5370246803, 5907271567, 6497998733,
  7147798607, 7862578483, 8648836363, 9513720011,
  10465092017, 11511601237, 12662761381, 13929037523,
  15321941293, 16854135499, 18539549051, 20393503969,
  22432854391, 24676139909, 27143753929, 29858129341,
  32843942389, 36128336639, 39741170353, 43715287409,
  48086816161, 52895497877, 58185047677, 64003552493,
  70403907883, 77444298689, 85188728633, 93707601497,
  103078361647, 113386197853, 124724817647, 137197299431,
  150917029411, 166008732391, 182609605691, 200870566261,
  220957622911, 243053385209, 267358723741, 294094596143,
  323504055803, 355854461419, 391439907569, 430583898359,
  473642288209, 521006517137, 573107168903, 630417885871,
  693459674461, 762805641919, 839086206131, 922994826779,
  1015294309507, 1116823740479, 1228506114527, 1351356725987,
  1486492398631, 1635141638587, 1798655802451, 1978521382723,
  2176373521033,
/// end, read 85041558143 primes, printed 265 primes, so 0.00000% cpu 13439.95 s
};

int64_t
trp_prime_ranked (int rk)
{
  constexpr unsigned numprimes = sizeof (trp_primes_tab) / sizeof (trp_primes_tab[0]);
  if (rk < 0)
    return 0;
  if (rk < (int)numprimes)
    return trp_primes_tab[rk];
  return 0;
} // end of trp_prime_ranked

int64_t
trp_prime_above (int64_t n)
{
  constexpr unsigned numprimes = sizeof (trp_primes_tab) / sizeof (trp_primes_tab[0]);
  int lo = 0, hi = numprimes;
  if (n >= trp_primes_tab[numprimes - 1])
    return 0;
  if (n < 2)
    return 2;
  while (lo + 8 < hi)
    {
      int md = (lo + hi) / 2;
      if (trp_primes_tab[md] > n)
        hi = md;
      else
        lo = md;
    };
  if (hi < (int) numprimes - 1)
    hi++;
  if (hi < (int) numprimes - 1)
    hi++;
  for (int ix = lo; ix < hi; ix++)
    if (trp_primes_tab[ix] > n)
      return trp_primes_tab[ix];
  return 0;
}

int64_t
trp_prime_greaterequal_ranked (int64_t n, int*prank)
{
  constexpr unsigned numprimes = sizeof (trp_primes_tab) / sizeof (trp_primes_tab[0]);
  if (prank) *prank = -1;
  int lo = 0, hi = numprimes;
  if (n >= trp_primes_tab[numprimes - 1])
    return 0;
  if (n < 2)
    return 2;
  while (lo + 8 < hi)
    {
      int md = (lo + hi) / 2;
      if (trp_primes_tab[md] > n)
        hi = md;
      else
        lo = md;
    };
  if (hi < (int) numprimes - 1)
    hi++;
  if (hi < (int) numprimes - 1)
    hi++;
  for (int ix = lo; ix < hi; ix++)
    if (trp_primes_tab[ix] >= n)
      {
        if (prank)
          *prank =  ix;
        return trp_primes_tab[ix];
      }
  return 0;
} // end of trp_prime_greaterequal_ranked



int64_t
trp_prime_below (int64_t n)
{
  constexpr unsigned numprimes =
    sizeof (trp_primes_tab) / sizeof (trp_primes_tab[0]);
  int lo = 0, hi = numprimes;
  if (n >= trp_primes_tab[numprimes - 1])
    return 0;
  if (n < 2)
    return 2;
  while (lo + 8 < hi)
    {
      int md = (lo + hi) / 2;
      if (trp_primes_tab[md] > n)
        hi = md;
      else
        lo = md;
    };
  if (hi < (int) numprimes - 1)
    hi++;
  if (hi < (int) numprimes - 1)
    hi++;
  for (int ix = hi; ix >= 0; ix--)
    if (trp_primes_tab[ix] < n)
      return trp_primes_tab[ix];
  return 0;
} // end trp_prime_below


int64_t
trp_prime_lessequal_ranked (int64_t n, int*prank)
{
  constexpr unsigned numprimes = sizeof (trp_primes_tab) / sizeof (trp_primes_tab[0]);
  if (prank) *prank = -1;
  int lo = 0, hi = numprimes;
  if (n >= trp_primes_tab[numprimes - 1])
    return 0;
  if (n < 2)
    return 2;
  while (lo + 8 < hi)
    {
      int md = (lo + hi) / 2;
      if (trp_primes_tab[md] > n)
        hi = md;
      else
        lo = md;
    };
  if (hi < (int) numprimes - 1)
    hi++;
  if (hi < (int) numprimes - 1)
    hi++;
  for (int ix = hi; ix >= 0; ix--)
    if (trp_primes_tab[ix] <= n)
      {
        if (prank)
          *prank = ix;
        return trp_primes_tab[ix];
      }
  return 0;
} // end trp_prime_lessequal_ranked

Trp_InputFile::Trp_InputFile(const std::string path)
  : mio::mmap_source(path), _inp_path(path),
    _inp_start(nullptr), _inp_end(nullptr), _inp_cur(nullptr), _inp_line(0), _inp_col(0)
{
  _inp_start = data();
  _inp_end = data() + size();
  _inp_cur = _inp_start;
  _inp_line = 1;
  _inp_col = 1;
} // end Trp_InputFile::Trp_InputFile

Trp_InputFile::~Trp_InputFile()
{
  _inp_start=nullptr;
  _inp_end=nullptr;
  _inp_line=0;
  _inp_col=0;
  _inp_cur=nullptr;
} // end Trp_InputFile::~Trp_InputFile

void
Trp_InputFile::skip_spaces(void)
{
  while (_inp_cur && isspace(*_inp_cur))
    {
      if (*_inp_cur=='\n')
        {
          _inp_cur++;
          _inp_line++;
          _inp_eol=nullptr;
          _inp_col=1;
        }
      else if (*_inp_cur=='\t')
        {
          _inp_cur++;
          _inp_col = ((1+_inp_col)|7)+1;
        }
      else
        {
          _inp_cur++;
          _inp_col++;
        }
    }
} // end Trp_InputFile::skip_spaces

const char*
Trp_InputFile::eol(void) const
{
  if (_inp_cur>=_inp_end) return nullptr;
  while (_inp_eol<_inp_end && *_inp_eol!='\n')
    _inp_eol++;
  if (_inp_eol>_inp_cur) return _inp_eol;
} // end Trp_InputFile::eol

ucs4_t
Trp_InputFile::peek_utf8(bool*goodp) const
{
  ucs4_t u=0;
  int l = u8_mbtoucr(&u, (const uint8_t*)_inp_cur, eol()-_inp_cur);
  if (l>0)
    {
      if (goodp)
        *goodp=true;
      return u;
    }
  if (goodp)
    *goodp=false;
  return 0;
} // end Trp_InputFile::peek_utf8

ucs4_t
Trp_InputFile::peek_utf8(const char*&nextp) const
{
  ucs4_t u=0;
  int l = u8_mbtoucr(&u, (const uint8_t*)_inp_cur, eol()-_inp_cur);
  if (l>0)
    {
      nextp = _inp_cur+l;
      return u;
    }
  nextp = _inp_cur;
  return 0;
} // end Trp_InputFile::peek_utf8


Trp_Token*
Trp_InputFile::next_token(void)
{
  bool goodch = false;
  skip_spaces();
  ucs4_t ch = peek_utf8(&goodch);
  if (!goodch)
    return nullptr;
  bool ascii = ch > 0 && ch <= 0x7f;
} // end trp_token::next_token

////////////////////////////////////////////////////////////////
Trp_Token::Trp_Token(Trp_InputFile*src, int lin, int col)
  : tok_src(src), tok_lin(lin), tok_col(col)
{
} // end Trp_Token::Trp_Token

Trp_Token::Trp_Token(Trp_InputFile*src, int col)
  : tok_src(src), tok_lin(src->lineno()), tok_col(col)
{
} // end Trp_Token::Trp_Token

Trp_Token::~Trp_Token()
{
  tok_src=nullptr;
  tok_lin=0;
  tok_col=0;
} // end Trp_Token::~Trp_Token



Trp_NameToken::Trp_NameToken(const std::string namstr, Trp_InputFile*src, int lin, int col)
  : Trp_Token(src, lin, col),
    _tok_symb_name(nullptr)
{
#warning unimplemented Trp_NameToken::Trp_NameToken
  TRP_ERROR("unimplemented Trp_NameToken nam=%s src=%s lin#%d col#%d",
            namstr.c_str(), src->path().c_str(), lin, col);
} // end Trp_NameToken::Trp_NameToken
Trp_NameToken::Trp_NameToken(const std::string namstr, Trp_InputFile*src, /*lin from src*/ int col)
  : Trp_Token(src, col),
    _tok_symb_name(nullptr)
{
#warning unimplemented Trp_NameToken::Trp_NameToken
  TRP_ERROR("unimplemented Trp_NameToken nam=%s src=%s col#%d",
            namstr.c_str(), src->path().c_str(),  col);
} // end Trp_NameToken::Trp_NameToken

Trp_NameToken::~Trp_NameToken()
{
#warning unimplemented Trp_NameToken::~Trp_NameToken
} // end Trp_NameToken::~Trp_NameToken

////////////////////////////////////////////////////////////////


///// main function and usual GNU inspired program options

static void
trp_show_version(void)
{
  GC_word gv = GC_get_version();
  std::clog << trp_prog_name << " version git " <<  trp_git_id
            << " using Boehm GC " << (gv >> 16) << "."
            << ((gv & 0xffff) >> 8) << "." << (gv & 0xff) << std::endl
            << " built " << __DATE__ "@" << __TIME__ << std::endl;
} // end trp_show_version

static void
trp_show_help(void)
{
  std::cout << trp_prog_name << " usage:" << std::endl
            << "\t --version                   # show version" << std::endl
            << "\t --help                      # this usage" << std::endl;
  std::cout << "\t --guile=<GUILE-source>      # processed by GNU guile" << std::endl
            << "\t                             # see www.gnu.org/software/guile/" << std::endl;
  std::cout << "\t --output=<C++-code>         # generated C++ file" << std::endl;
  std::cout << "GPLv3+ licensed, so without warranty!" << std::endl
            << "See its source file " << __FILE__ << " under github.com/bstarynk/misc-basile/"
            << std::endl;
  std::cout << "See also refpersys.org and github.com/RefPerSys/RefPerSys"
            << std::endl;
} // end trp_show_help

// to handle program argument --foo=, return position of char after =
// or else -1 if called as trp_position_equal_option("--foo",argv[XX])
int
trp_position_equal_option(const char*prefix, const char*progarg)
{
  if (!prefix || !progarg)
    return -1;
  if (prefix[0] != '-')
    return -1;
  size_t preflen = strlen(prefix);
  size_t arglen = strlen(progarg);
  if (preflen >= arglen)
    return -1;
  if (strncmp(progarg,prefix,preflen) != 0)
    return -1;
  if (progarg[preflen]!='=')
    return -1;
  return preflen+1;
} // end trp_position_equal_option

void
trp_parse_program_options(int &argc, char**argv)
{
  for (int ix=0; ix<argc; ix++) {
    char*curarg = argv[ix];
    int curguilepos= trp_position_equal_option("--guile", curarg);
    if (curguilepos>0) {
      const char*restguile = curarg+curguilepos;
      if (!access(restguile, R_OK)) {
	printf("loading GUILE file %s\n", restguile);
	fflush(nullptr);
	scm_c_primitive_load(restguile);
      }
      else if (restguile[0]=='(') {
	printf("evaluating GUILE expression %s\n", restguile);
	fflush(nullptr);
	SCM val = scm_c_eval_string(restguile);
	if (scm_is_false(val)) {
	  TRP_ERROR("GUILE expression %s was evaluated to false", restguile);
	  exit(EXIT_FAILURE);
	};
      }
    };
  }
#warning trp_parse_program_options unimplemented
  TRP_WARNING("unimplemented trp_parse_program_options argc=%d", argc);
} // end trp_parse_program_options

int
main(int argc, char**argv)
{
  trp_prog_name = argv[0];
  GC_INIT();
  if (argc == 2)
    {
      if (!strcmp(argv[1], "--version"))
        {
          trp_show_version();
          return 0;
        }
      else if (!strcmp(argv[1], "--help"))
        {
          trp_show_help();
          return 0;
        }
    };
  scm_init_guile();
  trp_parse_program_options(argc, argv);
} // end of main

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make transpiler-refpersys" ;;
 ** End: ;;
 ****************/


// end of file misc-basile/transpiler-refpersys.cc
