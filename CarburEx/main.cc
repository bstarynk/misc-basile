// file misc-basile/CarburEx/main.cc

#include "carbex.hh"

const char*carbex_progname;

void
show_version(void) {
  std::cout << carbex_progname
	    << " version " << GITID
	    << " built " << __DATE__ "@" __TIME__ << std::endl;
} // end show_version

void
show_help(void) {
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
