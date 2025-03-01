// file misc-basile/CarburEx/carbex.hh
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
#ifndef CARBEX_INCLUDED
#define CARBEX_INCLUDED

#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <err.h>

#include <map>
#include <vector>
#include <iostream>
#include <cstring>


extern "C" const char*carbex_progname;
extern "C" const char*carbex_filename;
extern "C" int carbex_curline, carbex_curcol;
extern "C" bool carbex_verbose;

#define CARB_LOG_AT2(Fil,Lin,Log) do {				\
if (carbex_verbose)						\
  std::cout << Fil << ":" << Lin << ":" << Log << std::endl;	\
} while(0)

#define CARB_LOG_AT(Fil,Lin,Log) CARB_LOG_AT2(Fil,Lin,Log)

#define CARB_LOG(Log) CARB_LOG_AT(__FILE__,__LINE__,Log)

/// forward declarations
class Tok;
class TokNull;
class TokInt;
class TokDouble;
class TokString;
class TokName;
class TokKeyword;
class TokChunk;
class TokOid;

enum TokType
{
  Tkty_none,
  Tkty_int,
  Tkty_double,
  Tkty_string,
  Tkty_chunk,
  Tkty_name,
  Tkty_oid,
  Tkty_keyword
};

enum CarbKeyword
{
};
/// In simple cases, we could just use std::variant, but this code is
/// an exercise for the rule of five. See
/// https://en.cppreference.com/w/cpp/language/rule_of_three and
/// https://www.codementor.io/@sandesh87/the-rule-of-five-in-c-1pdgpzb04f
class Tok
{
  enum TokType tk_type;
  int tk_lineno;
  int tk_colno;
  std::string tk_file;
  union
  {
    void* tk_ptr;
    intptr_t tk_int;
    double tk_double;
    std::string tk_string;
    TokName* tk_name;
    TokKeyword* tk_keyword;
    TokChunk* tk_chunk;
    TokOid* tk_oid;
  };
protected:
  Tok(nullptr_t)
    : tk_type(Tkty_none), tk_lineno(0), tk_colno(0), tk_file(),
      tk_ptr(nullptr) {};
  Tok(TokType ty, intptr_t n)
    : tk_type(ty),  tk_lineno(0), tk_colno(0), tk_file(),
      tk_int(n) {};
  Tok(TokType ty, double d, std::nullptr_t)
    : tk_type(ty),  tk_lineno(0), tk_colno(0), tk_file(),
      tk_double(d) {};
  Tok(TokType ty, std::string s)
    : tk_type(ty),  tk_lineno(0), tk_colno(0), tk_file(),
      tk_string(s) {};
  Tok(TokType ty, TokName& nm)
    : tk_type(ty),  tk_lineno(0), tk_colno(0), tk_file(),
      tk_name(&nm) {};
  Tok(TokType ty, TokKeyword& kw)
    : tk_type(ty),  tk_lineno(0), tk_colno(0), tk_file(),
      tk_keyword(&kw) {};
  Tok(TokType ty, TokChunk& ch)
    : tk_type(ty),  tk_lineno(0), tk_colno(0), tk_file(),
      tk_chunk(&ch) {};
public:
  enum TokType get_type(void) const
  {
    return tk_type;
  };
  virtual ~Tok();
  void set_lineno(int ln)
  {
    if (tk_lineno==0) tk_lineno=ln;
  };
#warning should follow the rule of five
  Tok(const Tok&);		// copy constructor
  Tok(Tok&&);			// move constructor
  Tok& operator = (const Tok&); // copy assignment
  Tok& operator = (Tok&&r); 	// move assignment
  int lineno(void) const
  {
    return tk_lineno;
  };
};				// end class Tok

using Tk_stdstring_t = std::string;

class TokNull : public Tok
{
public:
  TokNull() : Tok(nullptr) {};
  virtual ~TokNull() {};
};				// end TokNull

class TokInt : public Tok
{
public:
  TokInt(intptr_t n) : Tok(Tkty_int, n) {};
  virtual ~TokInt() {};
};

class TokDouble : public Tok
{
public:
  TokDouble(double d) : Tok(Tkty_double, d, nullptr) {};
  virtual ~TokDouble() {};
};

class TokString : public Tok
{
public:
  TokString(const std::string&s) : Tok(Tkty_string, s) {};
  TokString(const char*s) : TokString(std::string(s)) {};
  virtual ~TokString() {};
};				// end TokString

class TokKeyword : public Tok
{
};				// end TokKeyword

class TokName : public Tok
{
};				// end TokName

class TokChunk : public Tok
{
};				// end TokChunk




/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make -j2 carbex" ;;
 ** End: ;;
 ****************/

#endif /*CARBEX_INCLUDED*/
