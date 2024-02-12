// file misc-basile/browserfox.cc
// SPDX-License-Identifier: GPL-3.0-or-later
// © 2022 - 2023 copyright CEA & Basile Starynkevitch

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

extern "C" const char*progname;

extern "C" const char my_gitid[];
extern "C" const char my_buildtime[];

#ifndef GIT_ID
#error GIT_ID should be provided in compilation command
#endif

const char my_gitid[]=GIT_ID;
const char my_buildtime[]=__DATE__ "@" __TIME__;

const char*progname;


class BfWindow: public FXMainWindow
{
  FXDECLARE(BfWindow)
private:
protected:
  BfWindow() {};
public:
  static constexpr int bf_width=800;
  static constexpr int bf_height=650;
  BfWindow(FXApp*app);
  virtual ~BfWindow();
  virtual void create();
};				// end class BfWindow
FXDEFMAP(BfWindow) BfWindowMap[]=
{
};



////////////////////////////////////////////////////////////////
BfWindow::BfWindow(FXApp*app)
  : FXMainWindow(app, "browserfox",nullptr,nullptr,DECOR_ALL,0,0,
                 bf_width,bf_height)
{
}; // end BfWindow::BfWindow

BfWindow::~BfWindow()
{
}; // end BfWindow::~BfWindow

void
BfWindow::create()
{
}; // end BfWindow::create

FXIMPLEMENT(BfWindow,FXMainWindow,BfWindowMap,ARRAYNUMBER(BfWindowMap));

int main(int argc, char**argv)
{
  progname = argv[0];
  FXApp application("browserfox");
  application.init(argc, argv);
  BfWindow win(&application);
  return application.run();
} // end main

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./build-browserfox.sh" ;;
 ** End: ;;
 ****************/
