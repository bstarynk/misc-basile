/// file  misc-basile/ExBisonCpp/maintestb.cc


// ©2023 CEA and Basile Starynkevitch <basile@starynkevitch.net> and
// <basile.starynkevitch@cea.fr>

#include "testb.hh"


void tb_fatal_error_at(const char*fil, int lin)
{
  std::clog << progname << " ***°°°*** FATAL ERROR AT " << fil << ":" << lin
	    << std::endl;
  abort();
} // end tb_fatal_error_at

int
main(int argc, char**argv)
{
  progname = argv[0];
  gethostname(myhost, sizeof(myhost));
} // end main
