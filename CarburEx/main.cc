// file misc-basile/CarburEx/main.cc
// SPDX-License-Identifier: GPL-3.0-or-later
// © Copyright by Basile Starynkevitch 2023 - 2025
// program released under GNU General Public License v3+
//
// this is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 3, or (at your option) any later
// version.
//
// this is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details.

#include "carbex.hh"

///#include "carbex_parser.hh"

const char*carbex_progname;
const char*carbex_filename;
int carbex_lineno, carbex_colno;
bool carbex_verbose;

Tok::~Tok()
{
  switch (tk_type)
    {
    case Tkty_none:
      return;
    case Tkty_delim:
      tk_delim = delim__NONE;
      break;
    case Tkty_int:
      tk_int=0;
      break;
    case Tkty_double:
      tk_double=0.0;
      break;
    case Tkty_string:
      tk_string.clear();
      tk_string.~Tk_stdstring_t();
      break;
    case Tkty_name:
      tk_name=nullptr;
      break;
    case Tkty_keyword:
      tk_keyword=nullptr;
      break;
    case Tkty_chunk:
      tk_chunk=nullptr;
      break;
    case Tkty_oid:
      tk_oid = nullptr;
      break;
    };
  tk_type = Tkty_none;
  tk_ptr = nullptr;
} // end destructor of Tok



Tok::Tok(const Tok& ts) : // copy constructor
  Tok::Tok(nullptr)
{
  if (&ts == this) return;
  switch (ts.tk_type)
    {
    case Tkty_none:
      tk_ptr = nullptr;
      return;
    case Tkty_delim:
      tk_type = Tkty_delim;
      tk_delim = ts.tk_delim;
      break;
    case Tkty_int:
      tk_type = Tkty_int;
      tk_int = ts.tk_int;
      break;
    case Tkty_double:
      tk_type = Tkty_double;
      tk_double = ts.tk_double;
      break;
    case Tkty_string:
      tk_type = Tkty_string;
      new (&tk_string) std::string(ts.tk_string);
      break;
    case Tkty_keyword:
      tk_type = Tkty_keyword;
      tk_keyword = ts.tk_keyword;
      break;
    case Tkty_name:
      tk_type = Tkty_name;
      tk_name = ts.tk_name;
      break;
    case Tkty_chunk:
      tk_type = Tkty_chunk;
      tk_chunk = ts.tk_chunk;
      break;
    case Tkty_oid:
      tk_type = Tkty_oid;
      tk_oid = ts.tk_oid;
      break;
    }
} /// end of Tok::Tok(const Tok& ts) copy constructor




Tok::Tok(Tok&&ts) : // move constructor
  Tok::Tok(nullptr)
{
  if (&ts == this) return;
  switch (ts.tk_type)
    {
    case Tkty_none:
      tk_ptr = nullptr;
      tk_type = Tkty_none;
      return;
    case Tkty_delim:
      tk_type = Tkty_delim;
      tk_delim = ts.tk_delim;
      break;
    case Tkty_int:
      tk_type = Tkty_int;
      tk_int = ts.tk_int;
      break;
    case Tkty_double:
      tk_type = Tkty_double;
      tk_double = ts.tk_double;
      break;
    case Tkty_string:
      tk_type = Tkty_string;
      new (&tk_string) std::string(ts.tk_string);
      break;
    case Tkty_name:
      tk_type = Tkty_name;
      tk_name = ts.tk_name;
      break;
    case Tkty_keyword:
      tk_type = Tkty_keyword;
      tk_keyword = ts.tk_keyword;
      break;
    case Tkty_chunk:
      tk_type = Tkty_chunk;
      tk_chunk = ts.tk_chunk;
      break;
    case Tkty_oid:
      tk_type = Tkty_oid;
      tk_oid = ts.tk_oid;
      break;
    }
} // end Tok::Tok(Tok&&ts) move constructor

TokOid::TokOid(const char*idstr)  : Tok(Tkty_oid, _oid) {
  if (!idstr) {
    std::clog << "no idstr given" << std::endl;
    abort();
  };
  if (idstr[0] != '_') {
    std::clog << "oid should start with underline" << std::endl;
    abort();
  }
  int l = (int) strlen(idstr);
  if (l > (int)OID_SIZE) {
    std::clog << "too long idstr: " << idstr << std::endl;
    abort();
  }
  for (const char* pc= idstr+1; *pc; pc++) {
    if (!isalnum(*pc)) {
      std::clog << "invalid idstr: " << idstr << std::endl;
      abort();
    }
  }
  memcpy(_oid, idstr, l);
}


void
show_version(void)
{
  std::cout << carbex_progname
            << " version " << GITID << " with " << CARBURETTA_VERSION
            << " built " << __DATE__ "@" __TIME__ " from " __FILE__
            << std::endl;
  std::cout << "no warranty, since GPLv3+ licensed" << std::endl
            << "see CarburEx under github.com/bstarynk/misc-basile"
            << std::endl;
} // end show_version

void
show_help(void)
{
  std::cout << carbex_progname << " usage:" << std::endl;
  std::cout << "\t --version    # show version" << std::endl;
  std::cout << "\t --help       # show this help" << std::endl;
  std::cout << "\t --verbose    # verbose run" << std::endl;
  std::cout << "\t FILES...     # files to parse" << std::endl;
  std::cout << "no warranty, since GPLv3+ licensed" << std::endl
            << "see CarburEx under github.com/bstarynk/misc-basile"
            << std::endl;
} // end show_help


#define CARBEX_MACRO_FOR_MAKE_KEYWORD(Name)		\
  class TokKeyword* carbex_make_keyword_##Name(void) {	\
    if (isalpha(#Name[0]))				\
      return new TokKeyword(#Name,keyw_##Name);		\
    return nullptr; }

CARBEX_KEYWORDS(CARBEX_MACRO_FOR_MAKE_KEYWORD)
#undef CARBEX_MACRO_FOR_MAKE_KEYWORD


int
main(int argc, char**argv)
{
  carbex_progname = argv[0];
  GC_INIT();
  if (argc>1 && !strcmp(argv[1], "--version"))
    show_version();
  else if (argc>1 && !strcmp(argv[1], "--help"))
    show_help();
  for (int aix=1; aix<argc; aix++)
    {
      const char*curarg = argv[aix];
      if (!strcmp(curarg, "-v") || !strcmp(curarg, "--verbose"))
        {
          carbex_verbose = true;
        }
      if (curarg[0] != '-'
          && (isalnum(curarg[0]) || curarg[0]=='_'
              || curarg[0]=='/' || curarg[0]=='.'))
        {
          FILE* curfil = fopen(curarg, "r");
          if (!curfil)
            err(EXIT_FAILURE, "cannot open file %s", curarg);
          carbex_lineno = 1;
          carbex_colno = 1;
          carbex_filename = curarg;
#warning missing code to parse file
        }
      else if (curarg[0] == '|' || curarg[0]=='!') {
          FILE* curfil = popen(curarg+1, "r");
          if (!curfil)
            err(EXIT_FAILURE, "cannot open pipe %s", curarg);
          carbex_lineno = 1;
          carbex_colno = 1;
          carbex_filename = curarg;
#warning missing code to parse pipe
      }
    }
  return 0;
} // end main


/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make -j2 carbex" ;;
 ** End: ;;
 ****************/
