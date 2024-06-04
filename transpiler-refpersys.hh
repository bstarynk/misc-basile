// file misc-basile/transpiler-refpersys.hh
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
#ifndef TRANSPILER_REFPERSYS_INCLUDED
#define TRANSPILER_REFPERSYS_INCLUDED

#ifndef GIT_ID
#error GIT_ID should be defined by compilation command
#endif

#include <cstdint>
#include <cstring>
#include <iostream>

#include <map>
#include <vector>
#include <set>

/// Boehm conservative garbage collector:
#include "gc_cpp.h"


///naming convention: the trp_ prefix
extern "C" const char trp_git_id[];
extern "C" char*trp_progname;
extern "C" int64_t trp_prime_ranked (int rk);
extern "C" int64_t trp_prime_above (int64_t n);
extern "C" int64_t trp_prime_greaterequal_ranked (int64_t n, int*prank);
extern "C" int64_t trp_prime_below (int64_t n);
extern "C" int64_t trp_prime_lessequal_ranked (int64_t n, int*prank);

class Trp_Syntax;
class Trp_Token;
class Trp_StringToken;
class Trp_IntToken;
class Trp_NameToken;
class Trp_KeywordToken;
class Trp_DoubleToken;
class Trp_ChunkToken;

extern "C" Trp_Token*trp_parse_token(std::istream&ins);

class Trp_Token : public gc_cleanup
{
  friend  Trp_Token*trp_parse_token(std::istream&ins);
private:
  std::string tok_file;
  int tok_lin, tok_col;
protected:
  Trp_Token(std::string fil, int lin, int col=0);
  virtual ~Trp_Token();
};				// end class Trp_Token

#endif //TRANSPILER_REFPERSYS_INCLUDED
/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make transpiler-refpersys" ;;
 ** End: ;;
 ****************/

// end of file misc-basile/transpiler-refpersys.hh
