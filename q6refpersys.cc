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
#include <QProcess>
#include <QCommandLineParser>
#include <QDebug>
#include <QMainWindow>
#include <QMenuBar>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QSizePolicy>
#include <QLabel>
#include <QSocketNotifier>
#include <QtCore/qglobal.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern "C" char myqr_host_name[];
extern "C" bool myqr_debug;
extern "C" std::string myqr_jsonrpc;
extern "C" int myqr_jsonrpc_cmd_fd; /// written by RefPerSys, read by q6refpersys
extern "C" int myqr_jsonrpc_out_fd; /// read by RefPerSys, written by q6refpersys
extern "C" QSocketNotifier* myqr_notifier_jsonrpc_cmd;
extern "C" QSocketNotifier* myqr_notifier_jsonrpc_out;
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


////////////////////////////////////////////////////////////////
class MyqrMainWindow : public QMainWindow
{
  Q_OBJECT;
  //// the menubar
  QMenuBar* _mainwin_menubar;
  QMenu* _mainwin_appmenu;
  QAction* _mainwin_aboutact;
  QAction* _mainwin_aboutqtact;
  QMenu* _mainwin_editmenu;
  QAction* _mainwin_copyact;
  QAction* _mainwin_pasteact;
  /// the central widget is a vertical group box
  QGroupBox* _mainwin_centralgroup;
  QLabel*_mainwin_toplabel;
  QLineEdit*_mainwin_cmdline;
  QTextEdit*_mainwin_textoutput;
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





////////////////////////////////////////////////////////////////
class MyqrDisplayWindow : public QMainWindow
{
  Q_OBJECT;
private slots:
  void about();
public:
  explicit MyqrDisplayWindow(QWidget*parent = nullptr);
  virtual ~MyqrDisplayWindow();
};				// end MyqrDisplayWindow



////////////////////////////////////////////////////////////////
extern "C" QProcess*myqr_refpersys_process;
//=============================================================

std::ostream& operator << (std::ostream&out, const QList<QString>&qslist)
{
  int nbl=0;
  for (const QString&qs: qslist)
    {
      if (nbl++ > 0)
        out << ' ';
      std::string s = qs.toStdString();
      bool needquotes=false;
      for (char c: s)
        {
          if (!isalnum(c)&& c!='_') needquotes=true;
        }
      if (needquotes)
        out << "'";
      for (QChar qc : qs)
        {
          switch (qc.unicode())
            {
            case '\\':
              out << "\\\\";
              break;
            case '\'':
              out << "\\'";
              break;
            case '\"':
              out << "\\\"";
              break;
            case '\r':
              out << "\\r";
              break;
            case '\n':
              out << "\\n";
              break;
            case '\t':
              out << "\\t";
              break;
            case '\v':
              out << "\\v";
              break;
            case '\f':
              out << "\\f";
              break;
            case ' ':
              out << " ";
              break;
            default:
              out << QString(qc).toStdString();
              break;
            }
        }
      if (needquotes)
        out << "'";
    }
  return out;
}// end operator << (std::ostream&out, const QList<QString>&qslist)

////////////////////////////////////////////////////////////////


MyqrMainWindow::MyqrMainWindow(QWidget*parent)
  : QMainWindow(parent),
    _mainwin_menubar(nullptr),
    _mainwin_appmenu(nullptr),
    _mainwin_editmenu(nullptr),
    _mainwin_aboutact(nullptr),
    _mainwin_aboutqtact(nullptr),
    _mainwin_copyact(nullptr),
    _mainwin_pasteact(nullptr),
    _mainwin_centralgroup(nullptr),
    _mainwin_toplabel(nullptr)
{
  if (the_instance != nullptr)
    MYQR_FATALOUT("duplicate MyqrMainWndow @" << (void*)the_instance
                  << " and this@" << (void*)this);
  _mainwin_menubar = menuBar();
  _mainwin_appmenu =_mainwin_menubar-> addMenu("App");
  _mainwin_aboutact = _mainwin_appmenu->addAction("About");
  QObject::connect(_mainwin_aboutact,&QAction::triggered,this,&MyqrMainWindow::about);
  _mainwin_aboutqtact = _mainwin_appmenu->addAction("About Qt");
  QObject::connect(_mainwin_aboutqtact,&QAction::triggered,this,&MyqrMainWindow::aboutQt);
  _mainwin_editmenu =_mainwin_menubar-> addMenu("Edit");
  _mainwin_copyact =  _mainwin_editmenu->addAction("Copy");
  _mainwin_pasteact = _mainwin_editmenu->addAction("Paste");
  _mainwin_centralgroup = new QGroupBox(this);
  {
    QVBoxLayout *vbox = new QVBoxLayout;
    _mainwin_centralgroup->setLayout(vbox);
    _mainwin_toplabel = new QLabel("q6refpersys");
    vbox->addWidget(_mainwin_toplabel);
    _mainwin_cmdline = new QLineEdit(_mainwin_centralgroup);
    _mainwin_cmdline->setFixedWidth(this->width()-16);
    vbox->addWidget(_mainwin_cmdline);
    _mainwin_textoutput = new QTextEdit();
    _mainwin_textoutput->setReadOnly(true);
    vbox->addWidget(_mainwin_textoutput);
  }
  setCentralWidget(_mainwin_centralgroup);
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
  MYQR_DEBUGOUT("unimplemented MyqrMainWindow::aboutQt");
#warning unimplemented MyqrMainWindow::aboutQt
} // end MyqrDisplayWindow::aboutQt

void
MyqrMainWindow::about()
{
  MYQR_DEBUGOUT("unimplemented MyqrMainWindow::about");
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
  MYQR_DEBUGOUT("myqr_create_windows w=" << w << ", h=" << h);
  auto mainwin = new MyqrMainWindow(nullptr);
  mainwin->resize(w,h);
  mainwin->show();
  MYQR_DEBUGOUT("myqr_create_windows incomplete mainwin@" << (void*)mainwin);
#warning incomplete myqr_create_windows
} // end myqr_create_windows

void
myqr_readable_jsonrpc_cmd(void)
{
  MYQR_DEBUGOUT("myqr_readable_jsonrpc_cmd unimplemented myqr_jsonrpc_cmd_fd=" << myqr_jsonrpc_cmd_fd);
#warning unimplemented myqr_readable_jsonrpc_cmd
} // end myqr_readable_jsonrpc_cmd

void
myqr_writable_jsonrpc_out(void)
{
  MYQR_DEBUGOUT("myqr_writable_jsonrpc_out unimplemented myqr_jsonrpc_out_fd=" << myqr_jsonrpc_out_fd);
#warning unimplemented myqr_writable_jsonrpc_out
} // end myqr_writable_jsonrpc_out

void
myqr_have_jsonrpc(const std::string&jsonrpc)
{
  MYQR_DEBUGOUT("myqr_have_jsonrpc incomplete " << jsonrpc);
  std::string jsonrpc_cmd = jsonrpc+".cmd"; /// written by RefPerSys, read by q6refpersys
  std::string jsonrpc_out = jsonrpc+".out"; /// read by RefPerSys, written by q6refpersys
  if (access(jsonrpc_cmd.c_str(), F_OK))
    {
      if (mkfifo(jsonrpc_cmd.c_str(), 0660)<0)
        MYQR_FATALOUT("failed to create command JSONRPC fifo " << jsonrpc_cmd << ":" << strerror(errno));
    };
  if (access(jsonrpc_out.c_str(), F_OK))
    {
      if (mkfifo(jsonrpc_out.c_str(), 0660)<0)
        MYQR_FATALOUT("failed to create output JSONRPC fifo " << jsonrpc_out << ":" << strerror(errno));
    };
  myqr_jsonrpc_cmd_fd = open(jsonrpc_cmd.c_str(), 0440 | O_CLOEXEC);
  if (myqr_jsonrpc_cmd_fd<0)
    MYQR_FATALOUT("failed to open command JSONRPC " << jsonrpc_cmd << " for reading:" << strerror(errno));
  myqr_notifier_jsonrpc_cmd = new QSocketNotifier(myqr_jsonrpc_cmd_fd, QSocketNotifier::Read);
  QObject::connect(myqr_notifier_jsonrpc_cmd,&QSocketNotifier::activated,myqr_readable_jsonrpc_cmd);
  myqr_jsonrpc_out_fd = open(jsonrpc_out.c_str(), 0660 | O_CLOEXEC);
  if (myqr_jsonrpc_out_fd<0)
    MYQR_FATALOUT("failed to open output JSONRPC " << jsonrpc_out << " for writing:" << strerror(errno));
  myqr_notifier_jsonrpc_out = new QSocketNotifier(myqr_jsonrpc_out_fd, QSocketNotifier::Write);
  QObject::connect(myqr_notifier_jsonrpc_out,&QSocketNotifier::activated,myqr_writable_jsonrpc_out);
} // end myqr_have_jsonrpc

void
myqr_start_refpersys(const std::string& refpersysprog,
                     QStringList&arglist)
{
  std::string prog= refpersysprog;
  qint64 pid= 0;
  MYQR_DEBUGOUT("starting myqr_start_refpersys " << refpersysprog
                << " " << arglist);
  if (refpersysprog.empty())
    {
      prog = "refpersys";
    }
  else if (refpersysprog[0] == '-')
    {
      prog = "refpersys";
      arglist.prepend(QString(refpersysprog.c_str()));
    };
  myqr_refpersys_process = new QProcess();
  myqr_refpersys_process->setProgram(QString(prog.c_str()));
  myqr_refpersys_process->setArguments(arglist);
  if (!myqr_refpersys_process->startDetached(&pid))
    MYQR_FATALOUT("failed to start refpersys " << prog);
  MYQR_DEBUGOUT("myqr_start_refpersys started " << refpersysprog
                << " with arguments " << arglist
                << " as pid " << pid);
} // end myqr_start_refpersys

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
  QStringList args = cli_parser.positionalArguments();
  myqr_app = &the_app;
  QString geomstr = cli_parser.value(geometry_opt);
  MYQR_DEBUGOUT("geomstr:" << geomstr.toStdString());
  MYQR_DEBUGOUT("debug:" << cli_parser.value(debug_opt).toStdString());
  MYQR_DEBUGOUT("startrefpersys:" << cli_parser.value(refpersys_opt).toStdString()
                << (cli_parser.isSet(refpersys_opt)?" is set":" is not set"));
  myqr_create_windows(geomstr);
  if (cli_parser.isSet(jsonrpc_opt))
    myqr_have_jsonrpc(cli_parser.value(jsonrpc_opt).toStdString());
  if (cli_parser.isSet(refpersys_opt))
    {
      if (cli_parser.isSet(jsonrpc_opt))
        args += cli_parser.value(jsonrpc_opt);
      myqr_start_refpersys(cli_parser.value(refpersys_opt).toStdString(), args);
    };
  myqr_app->exec();
  myqr_app = nullptr;
  return 0;
} // end main



const char myqr_git_id[] = GITID;
char myqr_host_name[sizeof(myqr_host_name)];
QApplication *myqr_app;
bool myqr_debug;
std::string myqr_jsonrpc;
int myqr_jsonrpc_cmd_fd = -1;
int myqr_jsonrpc_out_fd = -1;
QProcess*myqr_refpersys_process;
QSocketNotifier* myqr_notifier_jsonrpc_cmd;
QSocketNotifier* myqr_notifier_jsonrpc_out;
#include "_q6refpersys-moc.cc"

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make q6refpersys" ;;
 ** End: ;;
 **
 ****************/
