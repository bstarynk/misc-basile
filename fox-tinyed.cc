// file misc-basile/fox-tinyed.cc
// SPDX-License-Identifier: GPL-3.0-or-later
//  © Copyright 2022 - 2024 Basile Starynkevitch (&Jeroen van der Zijp)
//    and the RefPerSys team on http://refpersys.org/
//  some code taken from fox-toolkit.org Adie

/**
 * This program shows two kind of windows. The main window (a single one,
 * which enable textual input from the users, see TinyMainWindow), and
 * several display windows (output only, the user can copy the
 * output), see TinyDisplayWindow.
 **/

#include <fstream>
#include <iostream>
#include <vector>
#include <cassert>
#include <functional>

/// fox-toolkit.org
#include <fx.h>
#include <unistd.h>
/// jsoncpp
#include <json/json.h>
#include <json/version.h>

extern "C" bool tiny_debug;
extern "C" char tiny_hostname[80];
extern "C" char*tiny_progname;

extern "C" const char tiny_buildtimestamp[];

const char tiny_buildtimestamp[]=__DATE__ "@" __TIME__;

#define TINY_DGBOUT_AT(Fil,Lin,Out) do {	\
  if (tiny_debug)				\
    std::clog << Fil << ":" << Lin << ": "	\
              << Out << std::endl;		\
} while(0)

#define TINY_DBGOUT(Out)  TINY_DGBOUT_AT(__FILE__, __LINE__, Out)

extern "C" void tiny_fatal_at(const char*fil, int lin) __attribute__((noreturn));
#define TINY_FATALOUT_AT(Fil,Lin,Out) do {		\
    std::cerr << Fil << ":" << Lin << ": FATAL:"	\
              << Out << std::endl;			\
    tiny_fatal_at(Fil,Lin);				\
} while(0)

#define TINY_FATALOUT(Out)  TINY_FATALOUT_AT(__FILE__, __LINE__, Out)

extern "C" void tiny_usage(void);
extern "C" void tiny_version(void);
class TinyMainWindow;
class TinyDisplayWindow;
class TinyHorizontalFrame;
class TinyVerticalFrame;
class TinyApp;
extern "C" TinyApp* tiny_app;

std::ostream& operator << (std::ostream&out,
                           const std::function<void(std::ostream&)>&f)
{
  f(out);
  return out;
}

// display output only window, there are several of them
class TinyDisplayWindow : public FXMainWindow
{
  FXDECLARE(TinyDisplayWindow);
private:
  TinyDisplayWindow();
  long _disp_win_rank; // the rank into TinyApp vector
  TinyVerticalFrame* _disp_vert_frame; // the topmost vertical frame
  ///
  FXMenuBar* _disp_menubar;
  FXMenuPane   *_disp_filemenu;       // File menu
  TinyHorizontalFrame* _disp_first_hframe; // the first horizontal subframe
  static long _disp_win_count;
#warning missing widget fields in TinyDisplayWindow
public:
  TinyDisplayWindow(FXApp *theapp);
  virtual ~TinyDisplayWindow();
  virtual void create(void);
  virtual void layout();
  void output(std::ostream&out) const;
};				// end TinyDisplayWindow



// Editor main window
class TinyMainWindow : public FXMainWindow
{
  FXDECLARE(TinyMainWindow);
  TinyVerticalFrame* _main_vertframe;
  FXMenuBar* _main_menubar;
  FXMenuPane *_main_filemenu;
  FXMenuCommand *_main_quitcmd;
protected:
  TinyMainWindow(): FXMainWindow(),
    _main_vertframe(nullptr), _main_menubar(nullptr), _main_filemenu(nullptr), _main_quitcmd(nullptr)
  {
    TINY_DBGOUT("TinyMainWindow @" << (void*)this);
  };
public:
  TinyMainWindow(FXApp *theapp);
  virtual ~TinyMainWindow();
  virtual void create(void);
  virtual void layout(void);
  virtual void show(void);
  void output (std::ostream&out) const;
};				// end TinyMainWindow

std::ostream&operator << (std::ostream&out, const TinyMainWindow&mw)
{
  mw.output(out);
  return out;
}

std::ostream&operator << (std::ostream&out, const TinyMainWindow*ptw)
{
  if (!ptw)
    out << "nullmainwinptr";
  else
    ptw->output(out);
  return out;
}


// our application
class TinyApp : public FXApp
{
  friend class TinyMainWindow;
  friend class TinyDisplayWindow;
  FXDECLARE(TinyApp);
public:
  TinyApp(const char*name, const char*vendor);
  virtual ~TinyApp();
private:
  TinyApp& operator=(const TinyApp&);
  std::vector<TinyDisplayWindow*> _vec_disp_win;
  TinyMainWindow* _main_win;
  TinyApp(): FXApp(), _vec_disp_win(), _main_win(nullptr) {};
  TinyApp(const TinyApp&);
}; // end of TinyApp

class TinyHorizontalFrame : public FXHorizontalFrame
{
  static int _counter;
  int _hf_num;
  FXDECLARE(TinyHorizontalFrame);
protected:
  TinyHorizontalFrame() : FXHorizontalFrame(), _hf_num(++_counter)
  {
  };
public:
  int num() const
  {
    return _hf_num;
  };
  TinyHorizontalFrame (FXComposite *p, FXuint opts=0, //
                       FXint x=0, FXint y=0, //
                       FXint w=0, FXint h=0, //
                       FXint pl=DEFAULT_SPACING, //
                       FXint pr=DEFAULT_SPACING, //
                       FXint pt=DEFAULT_SPACING, //
                       FXint pb=DEFAULT_SPACING, //
                       FXint hs=DEFAULT_SPACING, //
                       FXint vs=DEFAULT_SPACING) : //
    FXHorizontalFrame(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs), _hf_num(++_counter)
  {
    TINY_DBGOUT("TinyHorizontalFrame#" << _hf_num << "@" << (void*)this
                <<" p@" << (void*)p << " x=" << x << " y=" << y
                << " w=" << w << " h=" << h);
  };
  virtual ~TinyHorizontalFrame()
  {
    TINY_DBGOUT("destroy TinyHorizontalFrame#" << _hf_num);
  }
  void output(std::ostream&out) const;
};				// end TinyHorizontalFrame

class TinyVerticalFrame : public FXVerticalFrame
{
  static int _counter;
  int _vf_num;
  FXDECLARE(TinyVerticalFrame);
protected:
  TinyVerticalFrame() : FXVerticalFrame(), _vf_num(++_counter)
  {
  };
public:
  int num() const
  {
    return _vf_num;
  };
  TinyVerticalFrame (FXComposite *p, FXuint opts=0, //
                     FXint x=0, FXint y=0, //
                     FXint w=0, FXint h=0, //
                     FXint pl=DEFAULT_SPACING, //
                     FXint pr=DEFAULT_SPACING, //
                     FXint pt=DEFAULT_SPACING, //
                     FXint pb=DEFAULT_SPACING, //
                     FXint hs=DEFAULT_SPACING, //
                     FXint vs=DEFAULT_SPACING) : //
    FXVerticalFrame(p,opts,x,y,w,h,pl,pr,pt,pb,hs,vs), _vf_num(++_counter)
  {
    TINY_DBGOUT("TinyVerticalFrame#" << _vf_num << "@" << (void*)this
                <<" p@" << (void*)p << " x=" << x << " y=" << y
                << " w=" << w << " h=" << h);
  };
  virtual ~TinyVerticalFrame()
  {
    TINY_DBGOUT("destroy TinyVerticalFrame#" << _vf_num);
  }
  void output(std::ostream&out) const;
};				// end TinyVerticalFrame

void
TinyVerticalFrame::output(std::ostream&out) const
{
  out << "TinyVerticalFrame#" << _vf_num
      << "(X="<< getX() << ",Y="<< getY()
      << ",W=" << getWidth() << ",H=" << getHeight()
      << ";"
      << (isEnabled()?"enabled":"disabled")
      << ";"
      << (isActive()?"active":"inactive")
      << ";"
      << (isShell()?"shell":"nonshell")
      << ";"
      << (shown()?"shown":"hidden")
      << ")";
} // end TinyVerticalFrame::output

int TinyVerticalFrame::_counter;

std::ostream&operator << (std::ostream&out, const TinyVerticalFrame&tf)
{
  tf.output(out);
  return out;
}

std::ostream&operator << (std::ostream&out, const TinyVerticalFrame*ptw)
{
  if (!ptw)
    out << "nulltinyverticalframeptr";
  else
    out << *ptw ;
  return out;
}

int TinyHorizontalFrame::_counter;

void
TinyHorizontalFrame::output(std::ostream&out) const
{
  out << "TinyHorizontalFrame#" << num()
      << "/(x=" << getX() << ",y=" << getY()
      << ",w=" << getWidth() << ",h=" << getHeight()
      << ";"
      << (isEnabled()?"enabled":"disabled")
      << ";"
      << (isActive()?"active":"inactive")
      << ";"
      << (isShell()?"shell":"nonshell")
      << ";"
      << (shown()?"shown":"hidden")
      << ")";
} // end TinyHorizontalFrame

std::ostream&operator << (std::ostream&out, const TinyHorizontalFrame&tf)
{
  tf.output(out);
  return out;
}

std::ostream&operator << (std::ostream&out, const TinyHorizontalFrame*ptw)
{
  if (!ptw)
    out << "nulltinyhorizontalframeptr";
  else
    out << *ptw ;
  return out;
}

class TinyText : public FXText
{
  FXDECLARE(TinyText);
  static int _tt_textcount;
  int _tt_num;
public:
  TinyText();
  TinyText (FXComposite *p, FXObject *tgt=nullptr, //
            FXSelector sel=0, FXuint opts=0,//
            FXint x=0, FXint y=0, FXint w=0, FXint h=0, //
            FXint pl=3, FXint pr=3, FXint pt=2, FXint pb=2) //
    : FXText(p,tgt,sel,opts,x,y,w,h,pl,pr,pt,pb), _tt_num(++_tt_textcount)
  {
    TINY_DBGOUT("TinyText#" << _tt_num << " @" << (void*)this
                << "(x=" << ",y=" << y << ",w=" << w << ",h=" << h << ")");
  };
  virtual ~TinyText()
  {
  };
  int num() const
  {
    return _tt_num;
  };
  void output(std::ostream&out) const;
};				// end TinyText


TinyText::TinyText() : FXText(), _tt_num(++_tt_textcount)
{
  TINY_DBGOUT("TinyText#" << _tt_num << " @" << (void*)this);
};

void
TinyText::output(std::ostream&out) const
{
  out << "TinyText#" << _tt_num
      << "(X="<< getX() << ",Y="<< getY()
      << ",W=" << getWidth() << ",H=" << getHeight()
      << ")";
} // end TinyText::output

int TinyText::_tt_textcount;



////////////////

FXDEFMAP(TinyHorizontalFrame) TinyHorizontalFrameMap[]=
{
};

FXIMPLEMENT(TinyHorizontalFrame,FXHorizontalFrame,
            TinyHorizontalFrameMap, ARRAYNUMBER(TinyHorizontalFrameMap));


////////////////
FXDEFMAP(TinyVerticalFrame) TinyVerticalFrameMap[]=
{
};
////////////////


FXIMPLEMENT(TinyVerticalFrame,FXVerticalFrame,
            TinyVerticalFrameMap, ARRAYNUMBER(TinyVerticalFrameMap));

long TinyDisplayWindow::_disp_win_count = 0;

FXDEFMAP(TinyText) TinyTextMap[]=
{
};
FXIMPLEMENT(TinyText,FXText,TinyTextMap,ARRAYNUMBER(TinyTextMap));


std::ostream&operator << (std::ostream&out, const TinyDisplayWindow&tw)
{
  tw.output(out);
  return out;
}

std::ostream&operator << (std::ostream&out, const TinyDisplayWindow*ptw)
{
  if (!ptw)
    out << "nulldispwinptr";
  else
    ptw->output(out);
  return out;
}

TinyApp::TinyApp(const char*name, const char*vendor)
  : FXApp(name, vendor), _vec_disp_win(), _main_win(nullptr)
{
  TINY_DBGOUT("TinyApp constructor name:" << name << " vendor:" << vendor
              << "@" << (void*)this);
  _vec_disp_win.reserve(10);
} // end constructor TinyApp::TinyApp

TinyApp::~TinyApp()
{
  TINY_DBGOUT("TinyApp destructor @" << (void*)this);
} // end destructor TinyApp::~TinyApp

FXDEFMAP(TinyApp) TinyAppMap[]=
{
};
FXIMPLEMENT(TinyApp,FXApp,TinyAppMap,ARRAYNUMBER(TinyAppMap))

void
TinyDisplayWindow::output(std::ostream&out) const
{
  out << "TinyTextWindow#" << _disp_win_rank
      << "(X="<< getX() << ",Y="<< getY()
      << ",W=" << getWidth() << ",H=" << getHeight()
      << ";"
      << (isEnabled()?"enabled":"disabled")
      << ";"
      << (isActive()?"active":"inactive")
      << ";"
      << (isShell()?"shell":"nonshell")
      << ";"
      << (shown()?"shown":"hidden")
      << ")";
} // end TinyDisplayWindow::output

FXDEFMAP(TinyDisplayWindow) TinyDisplayWindowMap[]=
{
};

FXIMPLEMENT(TinyDisplayWindow,FXMainWindow,
            TinyDisplayWindowMap, ARRAYNUMBER(TinyDisplayWindowMap));

void
TinyDisplayWindow::create(void)
{
  TINY_DBGOUT("TinyDisplayWindow::create #" << _disp_win_rank);
  FXMainWindow::create();
#warning incomplete TinyDisplayWindow::create
  show(PLACEMENT_SCREEN);
  TINY_DBGOUT("end TinyDisplayWindow::create " << *this);
} // end TinyDisplayWindow::create

void
TinyDisplayWindow::layout()
{
  TINY_DBGOUT("TinyDisplayWindow::layout start " << *this);
  FXMainWindow::layout();
  TINY_DBGOUT("TinyDisplayWindow here " << *this);
  //if (editorframe)
  //  {
  //    editorframe->layout();
  //    TINY_DBGOUT("TinyDisplayWindow#" << winrank << ":layout editorframe: "
  //                << "(X="<< editorframe->getX()
  //                << ",Y="<< editorframe->getY()
  //                << ",W=" << editorframe->getWidth()
  //                << ",H=" << editorframe->getHeight()
  //                << ")");
  //  }
  //if (editor)
  //  {
  //    editor->layout();
  //    TINY_DBGOUT("TinyDisplayWindow#" << winrank << ":layout editor: "
  //                << "(X="<< editor->getX()
  //                << ",Y="<< editor->getY()
  //                << ",W=" << editor->getWidth()
  //                << ",H=" << editor->getHeight()
  //                << ")");
  //  };
  TINY_DBGOUT("end TinyDisplayWindow::layout " << *this);
} // end TinyDisplayWindow::layout

TinyDisplayWindow::TinyDisplayWindow(FXApp* theapp)
  : FXMainWindow(theapp, /*name:*/"tiny-displaywin-fox",
                 /*closedicon:*/nullptr, /*mainicon:*/nullptr,
                 /*opt:*/DECOR_ALL,
                 /*x,y,w,h:*/0, 0, 450, 333),
    _disp_win_rank (++_disp_win_count),
    _disp_vert_frame(nullptr),
    _disp_menubar(nullptr),
    _disp_filemenu(nullptr),
    _disp_first_hframe(nullptr)
{
  TINY_DBGOUT("TinyDisplayWindow#" << _disp_win_rank << " @" << (void*)this);
  assert(tiny_app != nullptr);
  tiny_app->_vec_disp_win.reserve(_disp_win_rank+1);
  if ((long)tiny_app->_vec_disp_win.size() == _disp_win_rank-1)
    tiny_app->_vec_disp_win.push_back(this);
  assert((long)tiny_app->_vec_disp_win.size() == _disp_win_rank);
  assert(tiny_app->_vec_disp_win[_disp_win_rank] == this);
  _disp_vert_frame = //
    new TinyVerticalFrame(this,LAYOUT_SIDE_TOP|FRAME_NONE //
                          |LAYOUT_FILL_X|LAYOUT_FILL_Y|PACK_UNIFORM_WIDTH,
                          2, 2, // x,yo
                          448, 330 //w,h
                         );
  _disp_menubar=new FXMenuBar(_disp_vert_frame,LAYOUT_SIDE_TOP|LAYOUT_FILL_X);

  // File menu
  _disp_filemenu=new FXMenuPane(_disp_menubar);
  new FXMenuCommand(_disp_filemenu,"&Quit",nullptr,getApp(),FXApp::ID_QUIT);
  _disp_first_hframe = //
    new TinyHorizontalFrame (_disp_vert_frame, 0,
                             TEXT_READONLY|TEXT_WORDWRAP|LAYOUT_FILL_X|LAYOUT_FILL_Y, //
                             6, 6, //x,y
                             444, 320 //w,h
                            );
  //editor->setText("Text in Tiny ");
  //editor->insertStyledText(editor->lineEnd(0), "XX", FXText::STYLE_BOLD);
  TINY_DBGOUT("end TinyDisplayWindow#" << _disp_win_rank << " @" << (void*)this);
}; // end TinyDisplayWindow::TinyDisplayWindow

TinyDisplayWindow::TinyDisplayWindow():
  FXMainWindow(),
  _disp_win_rank (++_disp_win_count),
  _disp_vert_frame(nullptr),
  _disp_menubar(nullptr),
  _disp_filemenu(nullptr),
  _disp_first_hframe(nullptr)
{
  TINY_DBGOUT("TinyDisplayWindow#" << _disp_win_rank << " @" << (void*)this);
  assert(tiny_app != nullptr);
  tiny_app->_vec_disp_win.reserve(_disp_win_rank+1);
  if ((long)tiny_app->_vec_disp_win.size() == _disp_win_rank-1)
    tiny_app->_vec_disp_win.push_back(this);
  assert((long)tiny_app->_vec_disp_win.size() == _disp_win_rank);
} // end TinyDisplayWindow::TinyDisplayWindow

TinyDisplayWindow::~TinyDisplayWindow()
{
  TINY_DBGOUT("destroying TinyDisplayWindow#" << _disp_win_rank << " @" << (void*)this);
  assert(tiny_app != nullptr);
  assert ((long)tiny_app->_vec_disp_win.size() > _disp_win_rank);
  assert(tiny_app->_vec_disp_win[_disp_win_rank] == this);
  tiny_app->_vec_disp_win[_disp_win_rank] = nullptr;
  // don't delete these subwindows, FX will do it....
  delete _disp_filemenu;
}; // end TinyDisplayWindow::~TinyDisplayWindow

////////////////////////////////////////////////////////////////

void
TinyMainWindow::show(void)
{
  TINY_DBGOUT("TinyMainWindow::show start " << *this);
  FXMainWindow::show();
  if (_main_vertframe)
    {
      _main_vertframe->show();
      TINY_DBGOUT("TinyMainWindow::show vertframe " << *_main_vertframe);
    }
  if (_main_menubar)
    {
      _main_menubar->show();
      TINY_DBGOUT("TinyMainWindow::show menubar @" << (void*)_main_menubar);
    };
  if (_main_filemenu)
    {
      _main_filemenu->show();
      TINY_DBGOUT("TinyMainWindow::show filemenu @" << (void*)_main_filemenu);
    };
  if (_main_quitcmd)
    {
      _main_quitcmd->show();
      TINY_DBGOUT("TinyMainWindow::show quitcmd @" << (void*)_main_quitcmd);
    };
  TINY_DBGOUT("TinyMainWindow::show end " << *this);
} // end TinyMainWindow::show

void
TinyMainWindow::output(std::ostream&out) const
{
  out << "TinyMainWindow"
      << "(X="<< getX() << ",Y="<< getY()
      << ",W=" << getWidth() << ",H=" << getHeight()
      << ";"
      << (isEnabled()?"enabled":"disabled")
      << ";"
      << (isActive()?"active":"inactive")
      << ";"
      << (isShell()?"shell":"nonshell")
      << ";"
      << (shown()?"shown":"hidden")
      << ")";
} // end TinyMainWindow::output

FXDEFMAP(TinyMainWindow) TinyMainWindowMap[]=
{
};

FXIMPLEMENT(TinyMainWindow,FXMainWindow,
            TinyMainWindowMap, ARRAYNUMBER(TinyMainWindowMap));

void
TinyMainWindow::create(void)
{
  static int count;
  count++;
  TINY_DBGOUT("TinyMainWindow::create #" << count << ": " << *this);
  if (count>1)
    {
      TINY_FATALOUT("TinyMainWindow multiple " << *this);
    }
  FXMainWindow::create();
  if (_main_vertframe)
    {
      _main_vertframe->create();
      TINY_DBGOUT("TinyMainWindow::create vertframe:" << *_main_vertframe);
    };
  if (_main_menubar)
    {
      _main_menubar->create();
      TINY_DBGOUT("TinyMainWindow::create menubar@" << (void*)_main_menubar);
    };
  if (_main_filemenu)
    {
      _main_filemenu->create();
      TINY_DBGOUT("TinyMainWindow::create filemenu@" << (void*)_main_filemenu);
    };
  if (_main_quitcmd)
    {
      _main_quitcmd->create();
      TINY_DBGOUT("TinyMainWindow::create quitcmd@" << (void*)_main_quitcmd);
    }
  TINY_DBGOUT("end TinyMainWindow::create " << *this);
} // end TinyMainWindow::create

void
TinyMainWindow::layout()
{
  TINY_DBGOUT("TinyMainWindow::layout start " << *this);
  FXMainWindow::layout();
  // TINY_DBGOUT("TinyMainWindow::layout here " << *this);
  //if (editorframe)
  //  {
  //    editorframe->layout();
  //    TINY_DBGOUT("TinyMainWindow#" << winrank << ":layout editorframe: "
  //                << "(X="<< editorframe->getX()
  //                << ",Y="<< editorframe->getY()
  //                << ",W=" << editorframe->getWidth()
  //                << ",H=" << editorframe->getHeight()
  //                << ")");
  //  }
  //if (editor)
  //  {
  //    editor->layout();
  //    TINY_DBGOUT("TinyMainWindow#" << winrank << ":layout editor: "
  //                << "(X="<< editor->getX()
  //                << ",Y="<< editor->getY()
  //                << ",W=" << editor->getWidth()
  //                << ",H=" << editor->getHeight()
  //                << ")");
  //  };
  TINY_DBGOUT("end TinyMainWindow::layout " << *this);
} // end TinyMainWindow::layout

TinyMainWindow::TinyMainWindow(FXApp* theapp)
  : FXMainWindow(theapp, /*name:*/"tiny-mainwin-fox",
                 /*closedicon:*/nullptr, /*mainicon:*/nullptr,
                 /*opt:*/DECOR_ALL,
                 /*x,y,w,h:*/20, 20, 450, 333),
    _main_vertframe(nullptr),
    _main_menubar(nullptr),
    _main_filemenu(nullptr),
    _main_quitcmd(nullptr)
{
  TINY_DBGOUT("TinyMainWindow @" << (void*)this);
  if (tiny_app && tiny_app->_main_win && tiny_app->_main_win != this)
    {
      TINY_FATALOUT("TinyApp has already a main window " << *tiny_app->_main_win
                    << " when constructing " << *this);
    };
  _main_vertframe = new TinyVerticalFrame(this, LAYOUT_SIDE_TOP|FRAME_NONE //
                                          |LAYOUT_FILL_X|LAYOUT_FILL_Y|PACK_UNIFORM_WIDTH,
                                          2, 2, // x,y
                                          448, 330 //w,h
                                         );
  _main_menubar = new FXMenuBar(_main_vertframe,
                                LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
  _main_filemenu = new FXMenuPane(_main_menubar);
  _main_quitcmd = new FXMenuCommand(_main_filemenu,"&Quit",nullptr,getApp(),FXApp::ID_QUIT);
  tiny_app->_main_win = this;
  TINY_DBGOUT("end TinyMainWindow @" << (void*)this
              << " main_menubar@" << _main_menubar
              << " main_filemenu@" << _main_filemenu
              << " main_quitcmd@" << _main_quitcmd);
}; // end TinyMainWindow::TinyMainWindow

TinyMainWindow::~TinyMainWindow()
{
  TINY_DBGOUT(" destroying TinyMainWindow @" << (void*)this << " " << *this);
  // don't delete these subwindows, FX will do it....
}; // end TinyMainWindow::~TinyMainWindow
////////////////////////////////////////////////////////////////



bool tiny_debug;
char tiny_hostname[80];
char*tiny_progname;
TinyApp* tiny_app;

void
tiny_usage(void)
{
  std::cout << tiny_progname << " usage:" << std::endl;
  std::cout << "\t --version|-V                             #show version"
            << std::endl;
  std::cout << "\t --debug|-D                               #debug messages"
            << std::endl;
  std::cout << "\t --help|-H                                #show this help"
            << std::endl;
} // end tiny_usage

void
tiny_version(void)
{
  std::cout << tiny_progname << " version:" << std::endl;
  std::cout << "git " << GIT_ID << " built " << tiny_buildtimestamp
            << std::endl;
  std::cout << "fox-toolkit " << (int)fxversion[0] << '.'
            << (int)fxversion[1] << '.'
            << (int)fxversion[2] << std::endl;
  /// see also https://github.com/open-source-parsers/jsoncpp/issues/1531
  std::cout << "jsoncpp " << JSONCPP_VERSION_STRING << std::endl;
} // end tiny_version

void tiny_fatal_at(const char*fil, int lin)
{
  fflush(nullptr);
  std::cerr << "ABORTING from " << fil << ":" << lin << std::endl;
  abort();
} // end tiny_fatal_at

int
main(int argc, char*argv[])
{
  bool showing_usage = false;
  tiny_progname = argv[0];
  gethostname(tiny_hostname, sizeof(tiny_hostname));
  if (argc>1 && (!strcmp(argv[1], "--debug") || !strcmp(argv[1], "-D")))
    tiny_debug = true;
  if (argc>1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-H")))
    {
      tiny_usage();
      showing_usage = true;
    };
  if (argc>1 && (!strcmp(argv[1], "--version") || !strcmp(argv[1], "-V")))
    {
      tiny_version();
      return 0;
    };
  assert(argc>0 && argv[argc] == nullptr);
  TINY_DBGOUT("start of " << argv[0] << " git " << GIT_ID
              << " pid " << (int)getpid() << " on " << tiny_hostname
              << " built " << tiny_buildtimestamp);
  TinyApp the_app("fox-tinyed","FOX tinyed (Basile Starynkevitch)");
  tiny_app = &the_app;
  the_app.init(argc, argv);
  TINY_DBGOUT("application " << &the_app);
  the_app.create();
  if (showing_usage)
    {
      return 0;
    }
  auto mywin = new TinyMainWindow(&the_app);
  mywin->create();
  mywin->layout();
  mywin->enable();
  TINY_DBGOUT("showing mywin:" << mywin);
  mywin->show();
  TINY_DBGOUT("shown mywin:" << mywin);
  if (!tiny_debug)
    {
      std::cout << argv[0] << " git " GIT_ID
                << " pid " << (int)getpid() << " on " << tiny_hostname
                << " built " << tiny_buildtimestamp << std::endl;
    };
  TINY_DBGOUT("running tiny_app " << (void*)tiny_app);
  int code = the_app.run();
  tiny_app = nullptr;
  assert (argc>=0); /// it could have been changed by FX!
  return code;
} // end main



/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./tinyed-build.sh" ;;
 ** End: ;;
 ****************/
