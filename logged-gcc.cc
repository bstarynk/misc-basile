// SPDX-License-Identifier: GPL-3.0-or-later
/* file logged-gcc.cc from https://github.com/bstarynk/misc-basile/

   a wrapper with logging of GCC compilation (for g++ or gcc on Linux)

   © Copyright Basile Starynkevitch 2020
   program released under GNU General Public License

   this is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   this is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.
*/

#include <string>
#include <vector>
#include <iostream>
#include <chrono>

#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <time.h>

#ifndef GCC_EXEC
#define GCC_EXEC "/usr/bin/gcc-10"
#endif

#ifndef GXX_EXEC
#define GXX_EXEC "/usr/bin/g++-10"
#endif

const char* mygcc;
const char* mygxx;

std::vector<const char*>
parse_logged_program_options(int &argc, char**argv,
                             std::string& cmdstr)
{
  std::vector<const char*> argvec;
  cmdstr.reserve(1+((argc*20+100)|0xff));
  argvec.reserve(argc+10);
  for (int ix=0; ix<argc; ix++)
    {
      if (ix>0) cmdstr += ' ';
      cmdstr += argv[ix];
    };
  ///
  for (int ix=1; ix<argc; ix++)
    {
      if (!strncmp(argv[ix],"--gcc=", strlen ("--gcc=")))
        mygcc=argv[ix]+strlen("--gcc=");
      else if (!strncmp(argv[ix],"--gxx=", strlen ("--gxx=")))
        mygxx=argv[ix]+strlen("--gxx=");
      else if (!strcmp(argv[ix], "--help"))
        {
          std::clog << argv[0]
                    << " is a logging wrapper to gcc or g++ compilers for Linux." << std::endl
                    << "  (see http://gcc.gnu.org/ and file " __FILE__ " on https://github.com/bstarynk/misc-basile/ for more)" << std::endl;
          std::clog << "A GPLv3+ licensed free software, see https://www.gnu.org/licenses/ for more" << std::endl;
          std::clog << " by Basile Starynkevitch (see http://starynkevitch.net/Basile/ email <basile@starynkevitch.net>), France" << std::endl;
          std::clog << " Acceptable options include:" << std::endl
                    << " --gcc=<some-executable> #e.g. --gcc=/usr/bin/gcc-9, overridding $LOGGED_GCC" << std::endl
                    << " --g++=<some-executable> #e.g. --g++=/usr/bin/g++-9, overridding $LOGGED_GXX" << std::endl
                    <<  "followed by program options passed to the GCC compiler..." << std::endl;
          std::clog << " Relevant environment variables are $LOGGED_GCC and $LOGGED_GXX for the compilers (when --gcc=... or --g++=... is not given)." << std::endl
                    << " When provided, the $LOGGED_CFLAGS may contain space-separated initial program options passed just after the C compiler $LOGGED_GCC." << std::endl
                    << " When provided, the $LOGGED_CXXFLAGS may contain space-separated initial program options passed just after the C++ compiler $LOGGED_GXX." << std::endl
                    << " When provided, the $LOGGED_LINKFLAGS may contain space-separated final program options passed just after the C or C++ compiler above." << std::endl;
        }
      else if (!strcmp(argv[ix], "--version"))
        {
          std::clog << argv[0] << " version " << GITID
                    <<  " of " << __FILE__ << " compiled on "
                    << __DATE__ "@" __TIME__ << std::endl
                    << "See https://github.com/bstarynk/misc-basile/ for more"
                    << std::endl
                    << " (a logging wrapper to GCC compiler on Linux)" << std::endl
                    << " so pass --help to get some help and usage." << std::endl
                    << " by Basile Starynkevitch (see http://starynkevitch.net/Basile/ email <basile@starynkevitch.net>), France" << std::endl;
          syslog(LOG_INFO, "version %s of %s from https://github.com/bstarynk/misc-basile/ compiled on %s@%s running %s",
                 GITID, argv[0], __DATE__, __TIME__, cmdstr.c_str());
          exit(EXIT_SUCCESS);
        }
      else
        argvec.push_back(argv[ix]);
    }
  return argvec;
} // end parse_logged_program_options



void
do_c_compilation(std::vector<const char*>argvec, std::string cmdstr, const char*linkflags)
{
  auto cflags= getenv("LOGGED_CFLAGS");
  if (cflags)
    {
      char*space=nullptr;
      std::vector<const char*> flagvec;
      flagvec.reserve(strlen(cflags)/3);
      for (auto pc = cflags; pc; pc = space?space+1:nullptr)
        {
          std::string curarg;
          space = strchr(pc, ' ');
          if (space)
            curarg = std::string(pc, space-pc-1);
          else
            curarg = std::string(pc);
          flagvec.push_back(curarg.c_str());
        }
    }
} // end do_c_compilation

void
do_cxx_compilation(std::vector<const char*>argvec, std::string cmdstr,  const char*linkflags)
{
  auto cxxflags= getenv("LOGGED_CXXFLAGS");
} // end do_cxx_compilation

int main(int argc, char**argv)
{
  openlog(argv[0], LOG_PERROR|LOG_PID, LOG_USER);
  std::string argstr;
  std::vector<const char*> argvec
    = parse_logged_program_options(argc, argv, argstr);
  bool for_cxx = strstr(argv[0], "++") != nullptr;
  if (!mygcc)
    mygcc = getenv("GCC");
  if (!mygcc)
    mygcc = getenv("LOGGED_GCC");
  if (!mygcc)
    mygcc = GCC_EXEC;
  if (!for_cxx && access(mygcc, X_OK))
    {
      syslog (LOG_WARNING, "%s is not executable - %m - for %s", mygcc,
              argstr.c_str());
      const char*defgcc = "/usr/bin/gcc";
      if (access(defgcc, X_OK))
        {
          syslog (LOG_ALERT, "%s is not executable - %m - for %s", defgcc,
                  argstr.c_str());
          exit(EXIT_FAILURE);
        };
      mygcc = defgcc;
    };
  ///
  mygxx = getenv("GXX");
  if (!mygxx)
    mygxx= getenv("LOGGED_GXX");
  if (!mygxx)
    mygxx = GXX_EXEC;
  if (for_cxx && access(mygxx, X_OK))
    {
      syslog (LOG_WARNING, "%s is not executable - %m - for %s", mygxx,
              argstr.c_str());
      const char*defgxx = "/usr/bin/g++";
      if (access(defgxx, X_OK))
        {
          syslog (LOG_ALERT, "%s is not executable - %m - for %s", defgxx,
                  argstr.c_str());
          exit(EXIT_FAILURE);
        };
      mygxx = defgxx;
    };
  auto linkflags = getenv("LOGGED_LINKFLAGS");
  if (for_cxx)
    do_c_compilation (argvec, argstr, linkflags);
  else
    do_cxx_compilation (argvec, argstr, linkflags);
} /* end main */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./compile-logged-gcc.sh" ;;
 ** End: ;;
 ****************/
