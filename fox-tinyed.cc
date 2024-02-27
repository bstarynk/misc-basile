// file misc-basile/fox-tinyed.cc
// SPDX-License-Identifier: GPL-3.0-or-later
//  © Copyright 2022 - 2024 Basile Starynkevitch (&Jeroen van der Zijp)
//  some code taken from fox-toolkit.org Adie
#include <fstream>
#include <iostream>
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

extern "C" void tiny_usage(void);
extern "C" void tiny_version(void);

// our application
class TinyApp : public FXApp
{
  FXDECLARE(TinyApp);
public:
  TinyApp(const char*name, const char*vendor);
  ~TinyApp();
private:
  TinyApp() {}
  TinyApp(const TinyApp&);
  TinyApp& operator=(const TinyApp&);
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

int TinyVerticalFrame::_counter;

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






// Editor main window
class TinyTextWindow : public FXMainWindow
{
  FXDECLARE(TinyTextWindow);
  ///
  TinyHorizontalFrame   *editorframe;             // Editor frame
  TinyText              *editor;                  // Editor text widget
  static int wincount;
  int winrank;
protected:
  TinyTextWindow(): FXMainWindow(), editorframe(nullptr), editor(nullptr),
    winrank(++wincount)
  {
    TINY_DBGOUT("TinyTextWindow#" << winrank << " @" << (void*)this);
  };
public:
  TinyTextWindow(FXApp *theapp);
  virtual ~TinyTextWindow();
  virtual void create();
  virtual void layout();
  void output (std::ostream&out) const;
};				// end TinyTextWindow



////////////////

FXDEFMAP(TinyHorizontalFrame) TinyHorizontalFrameMap[]=
{
};

FXIMPLEMENT(TinyHorizontalFrame,FXHorizontalFrame,
            TinyHorizontalFrameMap, ARRAYNUMBER(TinyHorizontalFrameMap));

int TinyTextWindow::wincount = 0;

FXDEFMAP(TinyText) TinyTextMap[]=
{
};
FXIMPLEMENT(TinyText,FXText,TinyTextMap,ARRAYNUMBER(TinyTextMap));


std::ostream&operator << (std::ostream&out, const TinyTextWindow&tw)
{
  tw.output(out);
  return out;
}

std::ostream&operator << (std::ostream&out, const TinyTextWindow*ptw)
{
  if (!ptw)
    out << "nulltextwindowptr";
  else
    out << *ptw << "@" << (void*)ptw;
  return out;
}

TinyApp::TinyApp(const char*name, const char*vendor)
  : FXApp(name, vendor)
{
  TINY_DBGOUT("TinyApp constructor name:" << name << " vendor:" << vendor
              << "@" << (void*)this);
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
TinyTextWindow::output(std::ostream&out) const
{
  out << "TinyTextWindow#" << winrank
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
} // end TinyTextWindow::output

FXDEFMAP(TinyTextWindow) TinyTextWindowMap[]=
{
};

FXIMPLEMENT(TinyTextWindow,FXMainWindow,
            TinyTextWindowMap, ARRAYNUMBER(TinyTextWindowMap));

void
TinyTextWindow::create()
{
  TINY_DBGOUT("TinyTextWindow::create #" << winrank);
  FXMainWindow::create();
  editorframe->create();
  editor->create();
  show(PLACEMENT_SCREEN);
  TINY_DBGOUT("end TinyTextWindow::create #" << winrank);
} // end TinyTextWindow::create()

void
TinyTextWindow::layout()
{
  TINY_DBGOUT("TinyTextWindow::layout start " << *this);
  FXMainWindow::layout();
  TINY_DBGOUT("TinyTextWindow here " << *this);
  if (editorframe)
    {
      editorframe->layout();
      TINY_DBGOUT("TinyTextWindow#" << winrank << ":layout editorframe: "
                  << "(X="<< editorframe->getX()
                  << ",Y="<< editorframe->getY()
                  << ",W=" << editorframe->getWidth()
                  << ",H=" << editorframe->getHeight()
                  << ")");
    }
  if (editor)
    {
      editor->layout();
      TINY_DBGOUT("TinyTextWindow#" << winrank << ":layout editor: "
                  << "(X="<< editor->getX()
                  << ",Y="<< editor->getY()
                  << ",W=" << editor->getWidth()
                  << ",H=" << editor->getHeight()
                  << ")");
    };
  TINY_DBGOUT("end TinyTextWindow::layout #" << winrank);
} // end TinyTextWindow::layout

TinyTextWindow::TinyTextWindow(FXApp* theapp)
  : FXMainWindow(theapp, /*name:*/"tiny-text-fox",
                 /*closedicon:*/nullptr, /*mainicon:*/nullptr,
                 /*opt:*/DECOR_ALL,
                 /*x,y,w,h:*/0, 0, 450, 333),
    editorframe(nullptr), editor(nullptr),
    winrank (++wincount)
{
  TINY_DBGOUT("TinyTextWindow#" << winrank << " @" << (void*)this);
  editorframe = //
    new TinyHorizontalFrame(this,LAYOUT_SIDE_TOP|FRAME_NONE //
                            |LAYOUT_FILL_X|LAYOUT_FILL_Y|PACK_UNIFORM_WIDTH,
                            2, 2, // x,y
                            448, 330 //w,h
                           );
  editor = //
    new TinyText (editorframe, 0,
                  TEXT_READONLY|TEXT_WORDWRAP|LAYOUT_FILL_X|LAYOUT_FILL_Y, //
                  6, 6, //x,y
                  444, 320 //w,h
                 );
  editor->setText("Text in Tiny ");
  editor->insertStyledText(editor->lineEnd(0), "XX", FXText::STYLE_BOLD);
  TINY_DBGOUT("end TinyTextWindow#" << winrank << " @" << (void*)this);
}; // end TinyTextWindow::TinyTextWindow

TinyTextWindow::~TinyTextWindow()
{
  TINY_DBGOUT(" destroying TinyTextWindow#" << winrank << " @" << (void*)this);
  // don't delete these subwindows, FX will do it....
  //WRONG: delete editorframe;
  //WRONG: delete editor;
}; // end TinyTextWindow::~TinyTextWindow

bool tiny_debug;
char tiny_hostname[80];
char*tiny_progname;

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
  TINY_DBGOUT("start of " << argv[0] << " git " << GIT_ID
              << " pid " << (int)getpid() << " on " << tiny_hostname
              << " built " << tiny_buildtimestamp);
  TinyApp the_app("fox-tinyed","FOX tinyed (Basile Starynkevitch)");
  the_app.init(argc, argv);
  TINY_DBGOUT("application " << &the_app);
  the_app.create();
  if (showing_usage)
    {
      return 0;
    }
  auto mywin = new TinyTextWindow(&the_app);
  mywin->create();
  mywin->layout();
  mywin->show();
  mywin->enable();
  TINY_DBGOUT("mywin:" << mywin);
  if (!tiny_debug)
    {
      std::cout << argv[0] << " git " GIT_ID
                << " pid " << (int)getpid() << " on " << tiny_hostname
                << " built " << tiny_buildtimestamp << std::endl;
    };
  return the_app.run();
} // end main



/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./tinyed-build.sh" ;;
 ** End: ;;
 ****************/
