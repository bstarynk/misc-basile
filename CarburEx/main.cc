// file misc-basile/CarburEx/main.cc
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

#include "carbex.hh"

const char*carbex_progname;

Tok::~Tok()
{
  switch (tk_type)
    {
    case tky_none:
      return;
    case tky_int:
      tk_int=0;
      break;
    case tky_double:
      tk_double=0.0;
      break;
    case tky_string:
      tk_string.empty();
      tk_string.~Tk_stdstring_t();
      break;
    };
  tk_type = tky_none;
  tk_ptr = nullptr;
} // end destructor of Tok



Tok::Tok(const Tok& ts) : // copy constructor
  Tok::Tok(nullptr)
{
  if (&ts == this) return;
  switch (ts.tk_type)
    {
    case tky_none:
      tk_ptr = nullptr;
      return;
    case tky_int:
      tk_type = tky_int;
      tk_int = ts.tk_int;
      break;
    case tky_double:
      tk_type = tky_double;
      tk_double = ts.tk_double;
      break;
    case tky_string:
      tk_type = tky_string;
      new (&tk_string) std::string(ts.tk_string);
      break;
    }
}

Tok::Tok(Tok&&ts) : // move constructor
  Tok::Tok(nullptr)
{
  if (&ts == this) return;
  switch (ts.tk_type)
    {
    case tky_none:
      tk_ptr = nullptr;
      tk_type = tky_none;
      return;
    case tky_int:
      tk_type = tky_int;
      tk_int = ts.tk_int;
      break;
    case tky_double:
      tk_type = tky_double;
      tk_double = ts.tk_double;
      break;
    case tky_string:
      tk_type = tky_string;
      new (&tk_string) std::string(ts.tk_string);
      break;
    }
} // end move constructor

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
  std::cout << carbex_progname << "usage:" << std::endl;
  std::cout << "\t --version # show version" << std::endl;
  std::cout << "\t --help    # show this help" << std::endl;
  std::cout << "no warranty, since GPLv3+ licensed" << std::endl
	    << "see CarburEx under github.com/bstarynk/misc-basile"
	    << std::endl;
} // end show_help

int
main(int argc, char**argv)
{
  carbex_progname = argv[0];
  if (argc>1 && !strcmp(argv[1], "--version"))
    show_version();
  else if (argc>1 && !strcmp(argv[1], "--help"))
    show_help();
} // end main
