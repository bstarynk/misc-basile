// file misc-basile/q6refpersys.cc
// SPDX-License-Identifier: GPL-3.0-or-later

/***
    © Copyright 2024 by Basile Starynkevitch
   program released under GNU General Public License v3+

   This is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   This is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   This gtk4serv is a GTK4 application. It is the interface to the
   RefPerSys inference engine on http://refpersys.org/ and
   communicates with the refpersys process using some JSONRPC protocol
   on named fifos. In contrast to refpersys itself, the q6refpersys process is
   short lived.
   
****/
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#define UNUSED __attribute__((unused))
extern const char myqr_git_id[];
extern char myqr_host_name[64];

#ifndef GITID
#error GITID should be defined in compilation command
#endif

extern "C" char myqr_gitid[];

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

extern "C" QApplication *myqr_app;
int main(int argc, char **argv)
{
  QApplication the_app(argc, argv);
  QCoreApplication::setApplicationName("q6refpersys");
  QCoreApplication::setApplicationVersion(QString("version ") + myqr_gitid
					  + " " __DATE__ "@" __TIME__);
  QCommandLineParser cli_parser;
  cli_parser.addVersionOption();
  cli_parser.process(the_app);
  const QStringList args = cli_parser.positionalArguments();
  
  myqr_app = &the_app;
  myqr_app->exec();
  myqr_app = nullptr;
  return 0;
} // end main



char myqr_gitid[] = GITID;
QApplication *myqr_app;

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make q6refpersys" ;;
 ** End: ;;
 **
 ****************/
