/// file  misc-basile/ExBisonCpp/testb.hh

#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <cstring>
#include <map>
#include <string>
#include <memory>
#include <iostream>
#include <unistd.h>
#include <time.h>

#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <getopt.h>

char myhost[80];
const char*progname;
extern "C" [[noreturn]] void tb_fatal_error_at(const char*fil, int lin);
#define TB_FATAL_AT(Fil,Lin,Log) do {			\
    std::ostringstream out##Lin;			\
    out##Lin << Log << std::endl;			\
    std::clog << "FATAL ERROR (" << progname << ") "	\
	      << Log << std::endl;			\
    tb_fatal_error_at(Fil,Lin);				\
  } while(0)

#define TB_BISFATAL_AT(Fil,Lin,Log) TB_FATAL_AT((Fil),Lin,Log)

#define TB_FATAL(Log) TB_BISFATAL_AT(__FILE__,__LINE__,Log)


class TbParser {
#warning missing code for class TbParser;
  std::string tbpars_source;
public:
  TbParser(std::string src): tbpars_source(src) {
  };
  virtual ~TbParser() {
  };
  virtual void output(std::ostream&out);
};

inline std::ostream& operator << (std::ostream&out, TbParser&tbp) {
  tbp.outpout(out);
  return out;
};
// ©2023 CEA and Basile Starynkevitch <basile@starynkevitch.net> and
// <basile.starynkevitch@cea.fr>
