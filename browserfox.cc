// file misc-basile/browserfox.cc
// SPDX-License-Identifier: GPL-3.0-or-later
// © 2022 - 2024 copyright CEA & Basile Starynkevitch

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
#include <iostream>


#include "fx.h"
#include "fxkeys.h"

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
  int rank() const { return win_rank;};
};				// end class BfWindow
FXDEFMAP(BfWindow) BfWindowMap[]=
{
};

int BfWindow::win_count;

#define BF_BACKTRACE_PRINT(Skip) do {if (my_debug_flag) \
      my_backtrace_print_at(__FILE__,__LINE__, (Skip)); } while (0)

extern "C" void bf_backtrace_print_at(const char*fil, int line, int skip);


/// fatal error macro, printf-like
#define BF_FATALPRINTF_AT(Fil,Lin,Fmt,...) do {		\
    printf("\n@@°@@FATAL ERROR (%s pid %d) %s:%d:\n",	\
	   my_prog_name, (int)getpid(),			\
	   Fil,Lin);					\
    printf((Fmt), ##__VA_ARGS__);			\
    putchar('\n');					\
    my_backtrace_print_at(Fil,Lin, (1));		\
    fflush(nullptr);					\
    my_abort(); } while(0)

//// example is FATALPRINTF   ("x=%d",x)
#define BF_FATALPRINTF(Fmt,...) FATALPRINTF_AT(__FILE__,__LINE__,\
					    Fmt,##__VA_ARGS__)

#define BF_FATALOUT_AT(Fil,Lin,Out) do {		\
    std::ostringstream outs##Lin;		\
    outs##Lin << Out << std::fflush;		\
    BF_FATALPRINTF_AT(Fil,Lin,"%s",		\
		   outs##Lin.str().c_str());	\
} while(0)
#define BF_FATALOUT(Out) FATALOUT_AT(__FILE__,__LINE__,Out)

/// debug printing macro à la printf
#define BF_DBGPRINTF_AT(Fil,Lin,Fmt,...) do {	\
    if (bf_debug) {				\
      fprintf(stderr,"@@%s:%d:",Fil,Lin);	\
      fprintf(stderr, (Fmt), ##__VA_ARGS__);	\
      fputc('\n', stderr); fflush(stderr);	\
    }} while(0)
#define BF_DBGPRINTF(Fmt,...) DBGPRINTF_AT(__FILE__,__LINE__,Fmt,\
					##__VA_ARGS__)

/// debug printing macro à la C++
#define BF_DBGOUT_AT(Fil,Lin,Out) do {			\
    if (bf_debug) {					\
      std::clog << "@@" Fil << ":" << Lin << ':'	\
		<< Out << std::endl;			\
    }} while(0)
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
int
main(int argc, char**argv)
{
  bf_progname = argv[0];
  gethostname(bf_hostname, sizeof(bf_hostname)-1);
  for (int i=1; i<argc; i++)
    if (!strcmp(argv[i], "-D") || !strcmp(argv[i], "--debug"))
      bf_debug = true;
  BF_DBGOUT("start of " << argv[0] << " pid " << (int)getpid() << " on " << bf_hostname
            << " git " << bf_gitid << " build " << bf_buildtime);
  FXApp application("browserfox");
  application.init(argc, argv);
  BfWindow win(&application);
  win.create();
  BF_DBGOUT("show win#" << win.rank() << " X=" << win.getX() << ",Y=" << win.getY()
	    << ",W=" << win.getWidth() << ",H=" << win.getHeight());
  win.show();
  BF_DBGOUT("win#" << win.rank() << " is " << (win.shown()?"shown":"hidden"));
  return application.run();
} // end main

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./build-browserfox.sh" ;;
 ** End: ;;
 ****************/
