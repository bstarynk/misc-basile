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

/// BSD like error functions
#include "err.h"

/// Boehm conservative garbage collector:
#include "gc_cpp.h"

/// GNU guile 3.0
#include "libguile.h"


// unistring library
#include "unistr.h"

// https://github.com/vimpunk/mio/tree/master/single_include/mio
#include "mio.hpp"

#define TRP_WARNING_AT_BIS(Fil,Lin,Fmt,...) do {  \
    warn("[from %s:%d]" Fmt "\n",     \
   (Fil),(Lin), __VA_ARGS__); } while (0)

#define TRP_WARNING_AT(Fil,Lin,Fmt,...) \
  TRP_WARNING_AT_BIS(Fil,Lin,Fmt,__VA_ARGS__)

#define TRP_WARNING(Fmt,...) \
  TRP_WARNING_AT(__FILE__,__LINE__,Fmt,##__VA_ARGS__)

#define TRP_EXIT_ERROR 100
#define TRP_ERROR_AT_BIS(Fil,Lin,Fmt,...) do {    \
    err(TRP_EXIT_ERROR,"[from %s:%d]" Fmt "\n",   \
   (Fil),(Lin), __VA_ARGS__); } while (0)

#define TRP_ERROR_AT(Fil,Lin,Fmt,...) \
  TRP_ERROR_AT_BIS(Fil,Lin,Fmt,__VA_ARGS__)

#define TRP_ERROR(Fmt,...) \
  TRP_ERROR_AT(__FILE__,__LINE__,Fmt,##__VA_ARGS__)

///naming convention: the trp_ prefix
extern "C" const char trp_git_id[];
extern "C" char*trp_progname;
extern "C" int64_t trp_prime_ranked (int rk);
extern "C" int64_t trp_prime_above (int64_t n);
extern "C" int64_t trp_prime_greaterequal_ranked (int64_t n, int*prank);
extern "C" int64_t trp_prime_below (int64_t n);
extern "C" int64_t trp_prime_lessequal_ranked (int64_t n, int*prank);

class Trp_InputFile;
class Trp_SymbolicName;
class Trp_Syntax;
class Trp_Token;
class Trp_StringToken;
class Trp_IntToken;
class Trp_NameToken;
class Trp_KeywordToken;
class Trp_DoubleToken;
class Trp_ChunkToken;

class Trp_InputFile : public mio::mmap_source
{
  const std::string _inp_path;
  const char* _inp_start;
  const char* _inp_end;
  const char* _inp_cur;
  mutable const char* _inp_eol;
  int _inp_line, _inp_col;
public:
  Trp_InputFile(const std::string path);
  Trp_Token*next_token(void);
  void skip_spaces(void);
  const char* eol(void) const;
  ucs4_t peek_utf8(bool*goodp=nullptr) const;
  ucs4_t peek_utf8(const char*&nextp) const;
  int lineno() const
  {
    return _inp_line;
  };
  int colno() const
  {
    return _inp_col;
  };
  const std::string path() const
  {
    return _inp_path;
  };
  virtual ~Trp_InputFile();
};        // end Trp_InputFile

class Trp_SymbolicName : public gc_cleanup
{
  const std::string _name_str;
  static std::map<std::string,Trp_SymbolicName*> _name_dict_;
  Trp_SymbolicName(const std::string);
public:
  static Trp_SymbolicName* find(const std::string n);
};        // end Trp_SymbolicName

enum Trp_TokenKind
{
  Tokd__None=0,
  Tokd_Int,
  Tokd_Double,
  Tokd_String,
  Tokd_Name,
};

class Trp_Token : public gc_cleanup
{
private:
  Trp_InputFile*tok_src;
  int tok_lin, tok_col;
protected:
  Trp_Token(Trp_InputFile*src, int lin, int col);
  Trp_Token(Trp_InputFile*src, /*lineno taken from src*/ int col);
  virtual ~Trp_Token();
public:
  virtual Trp_TokenKind token_kind() const
  {
    return Tokd__None;
  };
};        // end class Trp_Token

class Trp_NameToken : public Trp_Token
{
  Trp_SymbolicName*_tok_symb_name;
  virtual Trp_TokenKind token_kind() const
  {
    return Tokd_Name;
  };
public:
  Trp_NameToken(const std::string namstr, Trp_InputFile*src, int lin, int col);
  Trp_NameToken(const std::string namstr, Trp_InputFile*src, /*line from src*/ int col);
  virtual ~Trp_NameToken();
};        // end class Trp_NameToken

#endif //TRANSPILER_REFPERSYS_INCLUDED
/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make transpiler-refpersys" ;;
 ** End: ;;
 ****************/

// end of file misc-basile/transpiler-refpersys.hh
