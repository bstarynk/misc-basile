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
#include <sysexits.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#ifndef GCC_EXEC
#define GCC_EXEC "/usr/bin/gcc-10"
#endif

#ifndef GXX_EXEC
#define GXX_EXEC "/usr/bin/g++-10"
#endif

const char* mygcc;
const char* mygxx;

double get_float_time(clockid_t cid)
{
  struct timespec ts= {0,0};
  if (!clock_gettime(cid, &ts))
    return ts.tv_sec*1.0 + ts.tv_nsec*1.0e-9;
  else
    return NAN;
}

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
      else if (!strcmp(argv[ix], "--version")
               && strstr(argv[0], "logged"))
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


void fork_log_child_process(const char*cmdname, std::string progcmd, double startelapsedtime, std::vector<const char*>progargvec)
{
  auto pid = fork();
  if (pid<0)
    {
      syslog(LOG_ALERT, "fork failed for %s - %m", progcmd.c_str());
      exit(EX_OSERR);
    }
  else if (pid==0)
    {
      // child process
      execv(cmdname, (char* const*) (progargvec.data()));
      perror(cmdname);
      syslog(LOG_ALERT, "exec of %s failed for %s - %m", cmdname, progcmd.c_str());
      exit (EX_SOFTWARE);
    }
  else   // father process
    {
      struct rusage rus = {};
      int wst = 0;
      memset (&rus, 0, sizeof(rus));
      /// the below loop is likely to run once
      for(;;)
        {
          auto wpid = wait4(pid, &wst, WUNTRACED, &rus);
          if (wpid == pid)
            break;
          if (wpid < 0)
            {
              syslog(LOG_ALERT,
                     "wait of pid %d for %s failed for %s - %m",
                     (int)pid, cmdname, progcmd.c_str());
              exit(EX_OSERR);
            };
        };
      double endelapsedtime= get_float_time(CLOCK_MONOTONIC);
      double usertime = 1.0*rus.ru_utime.tv_sec + 1.0e-6*rus.ru_utime.tv_usec;
      double systime = 1.0*rus.ru_stime.tv_sec + 1.0e-6*rus.ru_stime.tv_usec;
      long maxrss = rus.ru_maxrss; //kilobtes
      long pageflt = rus.ru_minflt + rus.ru_majflt;
      if (wst==0)
        {
          syslog(LOG_INFO, "%s completed successfully compilation %s in %.4g elapsed seconds, %.4g user, %.4g sys cpu seconds, %ld Kbytes RSS, %ld pages faults (pid %d)",
                 cmdname, progcmd.c_str(), endelapsedtime-startelapsedtime,
                 usertime, systime, maxrss, pageflt,
                 (int)pid);
        }
      else if (WIFEXITED(wst))
        {
          syslog(LOG_WARNING, "%s failed compilation %s in %.4g elapsed seconds, %.4g user, %.4g sys cpu seconds, %ld Kbytes RSS, %ld pages faults (pid %d, exited %d)",
                 cmdname, progcmd.c_str(), endelapsedtime-startelapsedtime,
                 usertime, systime, maxrss, pageflt, WEXITSTATUS(wst),
                 (int)pid);
        }
      else if (WIFSIGNALED(wst))
        {
          syslog(LOG_ERR, "%s crashed compilation %s in %.4g elapsed seconds, %.4g user, %.4g sys cpu seconds, %ld Kbytes RSS, %ld pages faults (pid %d, signal %d=%s)",
                 cmdname, progcmd.c_str(), endelapsedtime-startelapsedtime,
                 usertime, systime, maxrss, pageflt,
                 (int)pid,
                 WTERMSIG(wst),
                 strsignal(WTERMSIG(wst)));
        }
    }
} // end fork_log_child_process

void
do_c_compilation(std::vector<const char*>argvec, std::string cmdstr, const char*linkflags)
{
  double startelapsedtime= get_float_time(CLOCK_MONOTONIC);
  auto cflags= getenv("LOGGED_CFLAGS");
  std::vector<const char*> progargvec;
  std::vector<const char*> cflagvec;
  std::vector<const char*> linkflagvec;
  if (cflags)
    {
      char*space=nullptr;
      cflagvec.reserve(strlen(cflags)/3);
      for (auto pc = cflags; pc; pc = space?space+1:nullptr)
        {
          std::string curarg;
          space = strchr(pc, ' ');
          if (space)
            curarg = std::string(pc, space-pc-1);
          else
            curarg = std::string(pc);
          cflagvec.push_back(curarg.c_str());
        }
    };
  if (linkflags)
    {
      char*space=nullptr;
      linkflagvec.reserve(strlen(cflags)/3);
      for (auto pc = linkflags; pc; pc = space?space+1:nullptr)
        {
          std::string curarg;
          space = strchr((char*)pc, ' ');
          if (space)
            curarg = std::string(pc, space-pc-1);
          else
            curarg = std::string(pc);
          linkflagvec.push_back(curarg.c_str());
        }
    }
  progargvec.reserve(cflagvec.size()
                     + linkflagvec.size() + argvec.size() + 2);
  assert (mygcc != nullptr && mygcc[0] != (char)0);
  progargvec.push_back(mygcc);
  for (auto itcfla : cflagvec)
    progargvec.push_back(itcfla);
  for (int ix=1; ix<(int)argvec.size(); ix++)
    progargvec.push_back(argvec[ix]);
  for (auto itlfla : linkflagvec)
    progargvec.push_back(itlfla);
  std::string progcmd;
  for (auto itarg: progargvec)
    progcmd += *itarg;
  progargvec.push_back(nullptr);
  syslog (LOG_INFO, "%s running C compilation %s for %s",
          argvec[0], progcmd.c_str(), cmdstr.c_str());
  fork_log_child_process(mygcc, progcmd, startelapsedtime, progargvec);
} // end do_c_compilation

void
do_cxx_compilation(std::vector<const char*>argvec, std::string cmdstr,  const char*linkflags)
{
  auto cxxflags= getenv("LOGGED_CXXFLAGS");
  double startelapsedtime= get_float_time(CLOCK_MONOTONIC);
  std::vector<const char*> progargvec;
  std::vector<const char*> cflagvec;
  std::vector<const char*> linkflagvec;
  if (cxxflags)
    {
      char*space=nullptr;
      cflagvec.reserve(strlen(cxxflags)/3);
      for (auto pc = cxxflags; pc; pc = space?space+1:nullptr)
        {
          std::string curarg;
          space = strchr(pc, ' ');
          if (space)
            curarg = std::string(pc, space-pc-1);
          else
            curarg = std::string(pc);
          cflagvec.push_back(curarg.c_str());
        }
    };
  if (linkflags)
    {
      char*space=nullptr;
      linkflagvec.reserve(strlen(cxxflags)/3);
      for (auto pc = linkflags; pc; pc = space?space+1:nullptr)
        {
          std::string curarg;
          space = strchr((char*)pc, ' ');
          if (space)
            curarg = std::string(pc, space-pc-1);
          else
            curarg = std::string(pc);
          linkflagvec.push_back(curarg.c_str());
        }
    }
  progargvec.reserve(cflagvec.size()
                     + linkflagvec.size() + argvec.size() + 2);
  assert (mygcc != nullptr && mygcc[0] != (char)0);
  progargvec.push_back(mygcc);
  for (auto itcfla : cflagvec)
    progargvec.push_back(itcfla);
  for (int ix=1; ix<(int)argvec.size(); ix++)
    progargvec.push_back(argvec[ix]);
  for (auto itlfla : linkflagvec)
    progargvec.push_back(itlfla);
  std::string progcmd;
  for (auto itarg: progargvec)
    progcmd += *itarg;
  progargvec.push_back(nullptr);
  syslog (LOG_INFO, "%s running C++ compilation %s - %s", progargvec[0], progcmd.c_str(),
          cmdstr.c_str());
  fork_log_child_process(mygcc, progcmd, startelapsedtime, progargvec);
} // end do_cxx_compilation

int main(int argc, char**argv)
{
  if (argc <= 1)
    {
      std::clog << argv[0] << " requires at least one argument. Try "
                << argv[0] << " --help" << std::endl;
      exit(EXIT_FAILURE);
    }
  openlog(argv[0], LOG_PERROR|LOG_PID, LOG_USER);
  std::string argstr;
  if (!mygcc)
    mygcc = getenv("GCC");
  if (!mygcc)
    mygcc = getenv("LOGGED_GCC");
  if (!mygcc)
    mygcc = GCC_EXEC;
  mygxx = getenv("GXX");
  if (!mygxx)
    mygxx= getenv("LOGGED_GXX");
  if (!mygxx)
    mygxx = GXX_EXEC;
  bool for_cxx = strstr(argv[0], "++") != nullptr;
  if (argc==2 && !strcmp(argv[1], "--version"))
    {
      openlog(argv[0], LOG_PID, LOG_USER);
      syslog(LOG_INFO, "%s running version query: %s --version", __FILE__, argv[0]);
      if (mygcc && !for_cxx)
        {
          syslog(LOG_INFO, "running version for gcc: %s", mygcc);
          argv[0] = (char*)mygcc;
          execv(mygcc, argv);
          perror(mygcc);
          exit(EX_OSERR);
        }
      else if (mygxx && for_cxx)
        {
          syslog(LOG_INFO, "running version for g++: %s", mygxx);
          argv[0] = (char*)mygxx;
          execv(mygxx, argv);
          perror(mygxx);
          exit(EX_OSERR);
        }
      else
        syslog(LOG_INFO, "running native version for %s", argv[0]);
    };
  std::vector<const char*> argvec
    = parse_logged_program_options(argc, argv, argstr);
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
