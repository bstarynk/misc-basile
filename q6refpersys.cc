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
extern "C" const char myqr_git_id[];
extern "C" char myqr_host_name[64];

#ifndef GITID
#error GITID should be defined in compilation command
#endif



#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QMainWindow>
#include <QtCore/qglobal.h>
#include <iostream>
#include <sstream>
#include <unistd.h>

extern "C" char myqr_host_name[];
extern "C" bool myqr_debug;

#define MYQR_FATALOUT_AT_BIS(Fil,Lin,Out) do {	\
    std::ostringstream outs##Lin;		\
    outs##Lin << Out << std::flush;		\
    qFatal("%s:%d: %s\n[git %s@%s] on %s",	\
	   Fil, Lin, outs##Lin.str().c_str(),   \
	   myqr_git_id, __DATE__" " __TIME__,	\
	   myqr_host_name);			\
    abort();					\
  } while(0)

#define MYQR_FATALOUT_AT(Fil,Lin,Out) \
  MYQR_FATALOUT_AT_BIS(Fil,Lin,Out)

#define MYQR_FATALOUT(Out) MYQR_FATALOUT_AT(__FILE__,__LINE__,Out)

#define MYQR_DEBUGOUT_AT_BIS(Fil,Lin,Out) do {	\
    if (myqr_debug)				\
      std::clog << Fil << ":" << Lin << " "	\
		<< Out << std::endl;		\
  } while(0)

#define MYQR_DEBUGOUT_AT(Fil,Lin,Out) \
  MYQR_DEBUGOUT_AT_BIS(Fil,Lin,Out)

#define MYQR_DEBUGOUT(Out) MYQR_DEBUGOUT_AT(__FILE__,__LINE__,Out)

extern "C" QApplication *myqr_app;

QT_BEGIN_NAMESPACE
namespace Ui
{
class MyqrMainWindow;
class MyqrDisplayWindow;
}
QT_END_NAMESPACE

class MyqrMainWindow : public QMainWindow
{
  Q_OBJECT;
private slots:
  void about();
  void aboutQt();
public:
  static MyqrMainWindow*the_instance;
  explicit MyqrMainWindow(QWidget*parent = nullptr);
  virtual ~MyqrMainWindow();
  static constexpr int minimal_width = 512;
  static constexpr int minimal_height = 128;
  static constexpr int maximal_width = 2048;
  static constexpr int maximal_height = 1024;
};				// end MyqrMainWindow
MyqrMainWindow*MyqrMainWindow::the_instance;

class MyqrDisplayWindow : public QMainWindow
{
  Q_OBJECT;
private slots:
  void about();
public:
  explicit MyqrDisplayWindow(QWidget*parent = nullptr);
  virtual ~MyqrDisplayWindow();
};				// end MyqrDisplayWindow

MyqrMainWindow::MyqrMainWindow(QWidget*parent)
  : QMainWindow(parent)
{
  if (the_instance != nullptr)
    MYQR_FATALOUT("duplicate MyqrMainWndow @" << (void*)the_instance
                  << " and this@" << (void*)this);
  the_instance = this;
  MYQR_DEBUGOUT("MyqrMainWndow the_instance@" << (void*)the_instance
		<< " parent@" << (void*)parent);
  setMinimumWidth(minimal_width);
  setMinimumHeight(minimal_height);
  setMaximumWidth(maximal_width);
  setMaximumHeight(maximal_height);
  qDebug() << "incomplete MyqrMainWindow constructor "
           << __FILE__  ":" << (__LINE__-1);
#warning incomplete MyqrMainWindow constructor
} // end MyqrMainWindow constructor

MyqrDisplayWindow::MyqrDisplayWindow(QWidget*parent)
  : QMainWindow(parent)
{
} // end MyqrDisplayWindow constructor

void
MyqrDisplayWindow::about()
{
  qDebug() << " unimplemented MyqrDisplayWindow::about";
} // end MyqrDisplayWindow::about

MyqrMainWindow::~MyqrMainWindow()
{
  if (the_instance != this)
    MYQR_FATALOUT("corruption in MyqrMainWndow the_instance@" << (void*)the_instance
                  << " this@" << (void*)this);
  the_instance = nullptr;
} // end MyqrMainWindow destructor

MyqrDisplayWindow::~MyqrDisplayWindow()
{
} // end MyqrDisplayWindow destructor

void
MyqrMainWindow::aboutQt()
{
  qDebug() << " unimplemented MyqrMainWindow::aboutQt";
#warning unimplemented MyqrMainWindow::aboutQt
} // end MyqrDisplayWindow::aboutQt

void
MyqrMainWindow::about()
{
  qDebug() << " unimplemented MyqrMainWindow::about";
#warning unimplemented MyqrMainWindow::about
} // end MyqrDisplayWindow::aboutQt

void myqr_create_windows(const QString& geom);

void
myqr_create_windows(const QString& geom)
{
  MYQR_DEBUGOUT("incomplete myqr_create_windows geometry "
                << geom.toStdString() << ";");
  int w=0, h=0;
  const char*geomcstr = geom.toStdString().c_str();
  if (geomcstr != nullptr)
    {
      MYQR_DEBUGOUT("myqr_create_windows geomcstr='" << geomcstr << "'");
      if (sscanf(geomcstr, "%dx%h", &w, &h) >= 2)
        {
          MYQR_DEBUGOUT("scanned w=" << w << " h=" << h);
          if (w > MyqrMainWindow::maximal_width)
            w= MyqrMainWindow::maximal_width;
          if (h > MyqrMainWindow::maximal_height)
            h=MyqrMainWindow::maximal_height;
        }
    }
  if (w< MyqrMainWindow::minimal_width)
    w=MyqrMainWindow::minimal_width;
  if (h< MyqrMainWindow::minimal_height)
    h= MyqrMainWindow::minimal_height;
  qDebug() << "myqr_create_windows w=" << w << ", h=" << h;
  auto mainwin = new MyqrMainWindow(nullptr);
  mainwin->resize(w,h);
  mainwin->show();
  qDebug() << "incomplete myqr_create_windows mainwin@" << (void*)mainwin;
#warning incomplete myqr_create_windows
} // end myqr_create_windows

int
main(int argc, char **argv)
{
  for (int i=1; i<argc; i++)
    {
      if (!strcmp(argv[i], "-D") || !strcmp(argv[i], "--debug"))
        {
          qDebug().setVerbosity(QDebug::DefaultVerbosity);
          myqr_debug = true;
        }
    }
  gethostname(myqr_host_name, sizeof(myqr_host_name)-1);
  MYQR_DEBUGOUT("starting " << argv[0] << " on " << myqr_host_name
                << " git " << myqr_git_id << " pid " << (int)getpid());
  QApplication the_app(argc, argv);
  QCoreApplication::setApplicationName("q6refpersys");
  QCoreApplication::setApplicationVersion(QString("version ") + myqr_git_id
                                          + " " __DATE__ "@" __TIME__);
  QCommandLineParser cli_parser;
  cli_parser.addVersionOption();
  cli_parser.addHelpOption();
  QCommandLineOption debug_opt(QStringList() << "D" << "debug",
                               "show debugging messages");
  cli_parser.addOption(debug_opt);
  QCommandLineOption jsonrpc_opt{{"J", "jsonrpc"},
    "Use $JSONRPC.out and $JSONRPC.cmd fifos.", "JSONRPC"};
  cli_parser.addOption(jsonrpc_opt);
  QCommandLineOption geometry_opt{{"G", "geometry"},
    "Main window geometry is W*H,\n... e.g. --geometry 400x650", "WxH"};
  cli_parser.addOption(geometry_opt);
  QCommandLineOption refpersys_opt{"start-refpersys",
                                   "Start the given $REFPERSYS, defaulted to refpersys",
                                   "REFPERSYS", QString("refpersys")};
  cli_parser.addOption(refpersys_opt);
  cli_parser.process(the_app);
  const QStringList args = cli_parser.positionalArguments();
  myqr_app = &the_app;
  QString geomstr = cli_parser.value(geometry_opt);
  MYQR_DEBUGOUT("geomstr:" << geomstr.toStdString());
  MYQR_DEBUGOUT("debug:" << cli_parser.value(debug_opt).toStdString());
  myqr_create_windows(geomstr);
  myqr_app->exec();
  myqr_app = nullptr;
  return 0;
} // end main



const char myqr_git_id[] = GITID;
char myqr_host_name[sizeof(myqr_host_name)];
QApplication *myqr_app;
bool myqr_debug;
#include "_q6refpersys-moc.cc"

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make q6refpersys" ;;
 ** End: ;;
 **
 ****************/
