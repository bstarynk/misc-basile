// file misc-basile/CarburEx/carbex.hh
// by Basile Starynkevitch 2023
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
  //  tky_name,
};
class Tok
{
  enum TokType tk_type;
  union
  {
    int tk_int;
    double tk_double;
    std::string tk_string;
  };
protected:
  Tok(TokType ty, int n): tk_type(ty), tk_int(n) {};
  Tok(TokType ty, double d): tk_type(ty), tk_double(d) {};
  Tok(TokType ty, std::string s): tk_type(ty), tk_string(s) {};
public:
  virtual ~Tok();
  Tok(int n) : Tok(tky_int, n) {};
  Tok(double d) : Tok(tky_double, d) {};
  Tok(std::string s): Tok(tky_string, s) {};
};

#endif /*CARBEX_INCLUDED*/
