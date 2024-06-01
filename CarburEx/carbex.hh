// file misc-basile/CarburEx/carbex.hh
// SPDX-License-Identifier: GPL-3.0-or-later
// © Copyright by Basile Starynkevitch 2023 - 2024
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

#include <map>
#include <vector>
#include <iostream>
#include <cstring>

extern "C" const char*carbex_progname;

enum TokType
{
  tky_none,
  tky_int,
  tky_double,
  tky_string,
  tky_chunk,
  tky_name,
  tky_keyword
};

/// In simple cases, we could just use std::variant, but this code is
/// an exercise for the rule of five. See
/// https://en.cppreference.com/w/cpp/language/rule_of_three and
/// https://www.codementor.io/@sandesh87/the-rule-of-five-in-c-1pdgpzb04f
class Tok
{
  enum TokType tk_type;
  int tk_lineno;
  union
  {
    void* tk_ptr;
    int tk_int;
    double tk_double;
    std::string tk_string;
  };
protected:
  Tok(nullptr_t) : tk_type(tky_none), tk_lineno(0), tk_ptr(nullptr) {};
  Tok(TokType ty, int n): tk_type(ty),  tk_lineno(0), tk_int(n) {};
  Tok(TokType ty, double d): tk_type(ty),  tk_lineno(0), tk_double(d) {};
  Tok(TokType ty, std::string s): tk_type(ty),  tk_lineno(0), tk_string(s) {};
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
  Tok(int n) : Tok(tky_int, n) {};
  Tok(double d) : Tok(tky_double, d) {};
  Tok(std::string s): Tok(tky_string, s) {};
  Tok(const Tok&);		// copy constructor
  Tok(Tok&&);			// move constructor
  Tok& operator = (const Tok&); // copy assignment
  Tok& operator = (Tok&&r); 	// move assignment
  int lineno(void) const
  {
    return tk_lineno;
  };
};

using Tk_stdstring_t = std::string;

class TokNull : public Tok
{
public:
  TokNull() : Tok(nullptr) {};
};

class TokInt : public Tok
{
public:
  TokInt(int n) : Tok(tky_int, n) {};
};

class TokDouble : public Tok
{
public:
  TokDouble(double d) : Tok(tky_double, d) {};
};

class  TokString : public Tok
{
public:
  TokString(const std::string&s) : Tok(tky_string, s) {};
  TokString(const char*s) : TokString(std::string(s)) {};
};

#endif /*CARBEX_INCLUDED*/
