// file misc-basile/CarburEx/main.cc

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
            << " version " << GITID
            << " built " << __DATE__ "@" __TIME__ << std::endl;
} // end show_version

void
show_help(void)
{
  std::cout << carbex_progname << "usage:" << std::endl;
  std::cout << "\t --version # show version" << std::endl;
  std::cout << "\t --help # show this help" << std::endl;
} // end show_help

int
main(int argc, char**argv)
{
  carbex_progname = argv[0];
  if (argc>1 && !strcmp(argv[1], "--version"))
    show_version();
} // end main
