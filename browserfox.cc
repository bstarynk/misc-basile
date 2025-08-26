// file misc-basile/browserfox.cc
// SPDX-License-Identifier: GPL-3.0-or-later
// © 2022 - 2025 copyright CEA & Basile Starynkevitch

/****
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
****/

#include <unistd.h>
#include <execinfo.h>
#include <iostream>
#include <sstream>
#include <gnu/libc-version.h>

#include <cxxabi.h>
#include "fx.h"
#include "fxkeys.h"
#include "fxver.h"

/// jsoncpp C++ library for JSON
#include "json/json.h"
#include "json/version.h"

extern "C" bool bf_debug;
extern "C" const char*bf_progname;
extern "C" const char bf_gitid[];
extern "C" const char bf_buildtime[];

extern "C" char bf_hostname[];
#ifndef GIT_ID
#error GIT_ID should be provided in compilation command
#endif

const char bf_gitid[]=GIT_ID;
const char bf_buildtime[]=__DATE__ "@" __TIME__;

const char*bf_progname;
bool bf_debug;

class BfWindow: public FXMainWindow
{
  int win_rank;
  static int win_count;
  FXDECLARE(BfWindow)
private:
protected:
  BfWindow();
public:
  static constexpr int bf_width=800;
  static constexpr int bf_height=650;
  BfWindow(FXApp*app);
  virtual ~BfWindow();
  virtual void create();
  int rank() const
  {
    return win_rank;
  };
};				// end class BfWindow
FXDEFMAP(BfWindow) BfWindowMap[]=
{
};

int BfWindow::win_count;

#define BF_BACKTRACE_PRINT(Skip) do {if (my_debug_flag) \
      bf_backtrace_print_at(__FILE__,__LINE__, (Skip)); } while (0)

extern "C" void bf_backtrace_print_at(const char*fil, int line, int skip);
extern "C" void bf_abort(void) __attribute__((noreturn));

/// fatal error macro, printf-like
#define BF_FATALPRINTF_AT_BIS(Fil,Lin,Fmt,...) do {	\
    printf("\n@@°@@FATAL ERROR (%s pid %d) %s:%d:\n",	\
	   bf_progname, (int)getpid(),			\
	   Fil,Lin);					\
    printf((Fmt), ##__VA_ARGS__);			\
    putchar('\n');					\
    bf_backtrace_print_at(Fil,Lin, (1));		\
    fflush(nullptr);					\
    bf_abort(); } while(0)
#define BF_FATALPRINTF_AT(Fil,Lin,Fmt,...) \
  BF_FATALPRINTF_AT_BIS(Fil,Lin,Fmt,##__VA_ARGS__)
//// example is BF_FATALPRINTF   ("x=%d",x)
#define BF_FATALPRINTF(Fmt,...) BF_FATALPRINTF_AT(__FILE__,__LINE__,\
					    Fmt,##__VA_ARGS__)

#define BF_FATALOUT_AT_BIS(Fil,Lin,Out) do {	\
    std::ostringstream outs##Lin;		\
    outs##Lin << Out << std::fflush;		\
    BF_FATALPRINTF_AT(Fil,Lin,"%s",		\
		   outs##Lin.str().c_str());	\
} while(0)
#define BF_FATALOUT_AT(Fil,Lin,Out) BF_FATALOUT_AT_BIS(Fil,Lin,Out)
//// example is BF_FATALOUT("bad x=" << x)
#define BF_FATALOUT(Out) BF_FATALOUT_AT(__FILE__,__LINE__,Out)


/// debug printing macro à la printf
#define BF_DBGPRINTF_AT_BIS(Fil,Lin,Fmt,...) do {	\
    if (bf_debug) {					\
      fprintf(stderr,"@@%s:%d:",Fil,Lin);		\
      fprintf(stderr, (Fmt), ##__VA_ARGS__);		\
      fputc('\n', stderr); fflush(stderr);		\
    }} while(0)
#define  BF_DBGPRINTF_AT(Fil,Lin,Fmt,...) \
  BF_DBGPRINTF_AT_BIS(Fil,Lin,Fmt,##__VA_ARGS__)
//// example is BF_DBGPRINTF("x=%d", x)
#define BF_DBGPRINTF(Fmt,...) BF_DBGPRINTF_AT(__FILE__,__LINE__,Fmt,\
					##__VA_ARGS__)

/// debug printing macro à la C++
#define BF_DBGOUT_AT_BIS(Fil,Lin,Out) do {			\
    if (bf_debug) {					\
      std::clog << "@@" Fil << ":" << Lin << ':'	\
		<< Out << std::endl;			\
    }} while(0)
#define BF_DBGOUT_AT(Fil,Lin,Out) BF_DBGOUT_AT_BIS(Fil,Lin,Out)
#define BF_DBGOUT(Out) BF_DBGOUT_AT(__FILE__,__LINE__,Out)




////////////////////////////////////////////////////////////////
BfWindow::BfWindow(FXApp*app)
  : FXMainWindow(app, "browserfox",nullptr,nullptr,DECOR_ALL,0,0,
                 bf_width,bf_height),
    win_rank(++win_count)
{
  BF_DBGOUT("BfWindow#" << win_rank << " in app @"<<(void*)this);
}; // end BfWindow::BfWindow

BfWindow::BfWindow(): FXMainWindow(), win_rank(++win_count)
{
  BF_DBGOUT("BfWindow#" << win_rank<<" @"<<(void*)this);
};
BfWindow::~BfWindow()
{
  BF_DBGOUT("~BfWindow#" << win_rank);
}; // end BfWindow::~BfWindow

void
BfWindow::create()
{
  BF_DBGOUT("BfWindow:create#" << win_rank);
}; // end BfWindow::create

FXIMPLEMENT(BfWindow,FXMainWindow,BfWindowMap,ARRAYNUMBER(BfWindowMap));

char bf_hostname[80];

void
bf_abort(void)
{
  abort();
} // end bf_abort

void
bf_backtrace_print_at(const char*fil, int line, int skip)
{
  constexpr int maxdepth = 256;
  void *array[maxdepth];
  int size=0, i=0;
  char **strings=nullptr;
  memset(array, 0, sizeof(array));
  size = backtrace (array,maxdepth);
  strings = backtrace_symbols (array, size);
  if (!strings)
    {
      fprintf(stderr, "Failed to backtrace from %s:%d (skip:%d)\n", fil, line, skip);
      bf_abort();
      return;
    };
  fprintf(stderr, "backtrace from %s:%d (skip:%d)\n", fil, line, skip);
  for (i=0; i<size; i++)
    {
      if (i<skip)
        continue;
      fprintf(stderr, "%d: %s", i, strings[i]);
      if (strings[i][0]=='_')
        {
          int status = -4;
          char*demangledname= abi::__cxa_demangle(strings[i],
                                                  NULL, NULL, &status);
          if (demangledname && demangledname[0] && status ==0)
            fprintf(stderr, " = %s", demangledname);
          free(demangledname);
        };
      fprintf(stderr, " @%p\n", array[i]);
    }
  fflush(stderr);
} // end bf_backtrace_print_at



void
bf_help(void)
{
  printf("%s usage:\n", bf_progname);
  printf("\t -D | --debug                    # debug messages\n");
  printf("\t -V | --version                  # version information\n");
  printf("\t -H | --help                     # this help\n");
  printf("\t --dont-run                      # dry run, crashes\n");
  printf("# and FOX toolkit options\n");
  fflush(stdout);
} // end of bf_help

int
main(int argc, char**argv)
{
  bool dontrun = false;
  bool showversion = false;
  bool showhelp = false;
  bf_progname = argv[0];
  gethostname(bf_hostname, sizeof(bf_hostname)-1);
  for (int i=1; i<argc; i++)
    {
      if (!strcmp(argv[i], "-D") || !strcmp(argv[i], "--debug"))
        bf_debug = true;
      if (!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version"))
        showversion = true;
      if (!strcmp(argv[i], "-H") || !strcmp(argv[i], "--help"))
        showhelp = true;
      if (!strcmp(argv[i], "--dont-run"))
        dontrun = true;
    }
  BF_DBGOUT("start of " << argv[0] << " pid " << (int)getpid()
            << " on " << bf_hostname
            << " git " << bf_gitid << " build " << bf_buildtime
            << " for FOX "
            << FOX_MAJOR << '.' << FOX_MINOR << '.' << FOX_LEVEL);
  if (showversion)
    {
      printf("%s version git %s built on %s\n", bf_progname,
             bf_gitid, bf_buildtime);
      printf("GNU glibc %s\n", gnu_get_libc_version());
      printf("compiled for FOX %d.%d.%d\n", FOX_MAJOR, FOX_MINOR, FOX_LEVEL);
      printf("compiled for JSONCPP %s\n", JSONCPP_VERSION_STRING);
      return 0;
    };
  FXApp application("browserfox");
  application.init(argc, argv);
  if (showhelp)
    {
      bf_help();
      return 0;
    };
  BfWindow win(&application);
  win.create();
  BF_DBGOUT("show win#" << win.rank() << " X=" << win.getX() << ",Y=" << win.getY()
            << ",W=" << win.getWidth() << ",H=" << win.getHeight());
  /// the following call is creating X11 windows.
  application.create();
  win.show(PLACEMENT_SCREEN);
  BF_DBGOUT("win#" << win.rank() << " is " << (win.shown()?"shown":"hidden"));
  if (dontrun)
    {
      BF_DBGOUT("dont run app " << (void*)&application);
      BF_FATALOUT("wont run in pid " << (int)getpid() << " on " << bf_hostname
                  << " git " << bf_gitid << " build " << bf_buildtime << " since --dont-run given");
      return EXIT_FAILURE;
    };
  int runcode = application.run();
  BF_DBGOUT("after app " << (void*)&application << " in pid " << (int)getpid() << " on " << bf_hostname
            << " git " << bf_gitid << " runcode " << runcode);
  return runcode;
} // end main

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./build-browserfox.sh" ;;
 ** End: ;;
 ****************/
