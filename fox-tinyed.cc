// file misc-basile/fox-tinyed.cc
// SPDX-License-Identifier: GPL-3.0-or-later
//  © Copyright 2022 - 2024 Basile Starynkevitch (&Jeroen van der Zijp)
//  some code taken from fox-toolkit.org Adie
#include <fstream>
#include <iostream>
/// fox-toolkit.org
#include <fx.h>
#include <unistd.h>

extern "C" bool tiny_debug;
extern "C" char tiny_hostname[80];
extern "C" const char tiny_buildtimestamp[];

const char tiny_buildtimestamp[]=__DATE__ "@" __TIME__;

#define TINY_DGBOUT_AT(Fil,Lin,Out) do {	\
  if (tiny_debug)				\
    std::clog << Fil << ":" << Lin << ": "	\
              << Out << std::endl;		\
} while(0)

#define TINY_DBGOUT(Out)  TINY_DGBOUT_AT(__FILE__, __LINE__, Out)

// Editor main window
class TinyTextWindow : public FXMainWindow
{
    FXDECLARE(TinyTextWindow);
    ///
    FXHorizontalFrame   *editorframe;             // Editor frame
    FXText              *editor;                  // Editor text widget
    static int wincount;
    int winrank;
protected:
    TinyTextWindow(): FXMainWindow(), editorframe(nullptr), editor(nullptr),
        winrank(++wincount) {};
public:
    TinyTextWindow(FXApp *theapp);
    virtual ~TinyTextWindow();
    virtual void create();
    virtual void layout();
    void output (std::ostream&out) const;
};				// end TinyTextWindow

int TinyTextWindow::wincount = 0;

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

FXIMPLEMENT(TinyTextWindow,FXMainWindow,nullptr, 0);

void
TinyTextWindow::create()
{
    FXMainWindow::create();
    editorframe->create();
    editor->create();
    show(PLACEMENT_SCREEN);
} // end TinyTextWindow::create()

void
TinyTextWindow::layout()
{
    FXMainWindow::layout();
} // end TinyTextWindow::layout

TinyTextWindow::TinyTextWindow(FXApp* theapp)
    : FXMainWindow(theapp, /*name:*/"tiny-text-fox",
                   /*closedicon:*/nullptr, /*mainicon:*/nullptr,
                   /*opt:*/DECOR_ALL,
                   /*x,y,w,h:*/0, 0, 450, 333),
      editorframe(nullptr), editor(nullptr),
      winrank (++wincount)
{
    editorframe = //
        new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|FRAME_NONE //
                              |LAYOUT_FILL_X|LAYOUT_FILL_Y|PACK_UNIFORM_WIDTH,
                              2, 2, // x,y
                              448, 330 //w,h
                             );
    editor = //
        new FXText (editorframe, 0,
                    TEXT_READONLY|TEXT_WORDWRAP|LAYOUT_FILL_X|LAYOUT_FILL_Y, //
                    6, 6, //x,y
                    444, 320 //w,h
                   );
    editor->setText("Text in Tiny ");
    editor->insertStyledText(editor->lineEnd(0), "XX", FXText::STYLE_BOLD);
}; // end TinyTextWindow::TinyTextWindow

TinyTextWindow::~TinyTextWindow()
{
    // don't delete these subwindows, FX will do it....
    //WRONG: delete editorframe;
    //WRONG: delete editor;
}; // end TinyTextWindow::~TinyTextWindow

bool tiny_debug;
char tiny_hostname[80];
int
main(int argc, char*argv[])
{
    gethostname(tiny_hostname, sizeof(tiny_hostname));
    if (argc>1 && (!strcmp(argv[1], "--debug") || !strcmp(argv[1], "-D")))
        tiny_debug = true;
    TINY_DBGOUT("start of " << argv[0] << " git " << GIT_ID
                << " pid " << (int)getpid() << " on " << tiny_hostname
                << " built " << tiny_buildtimestamp);
    FXApp the_app("fox-tinyed","FOX tinyed (Basile Starynkevitch)");
    the_app.init(argc, argv);
    the_app.create();
    auto mywin = new TinyTextWindow(&the_app);
    mywin->create();
    mywin->layout();
    mywin->show();
    mywin->enable();
    TINY_DBGOUT("mywin:" << mywin);
    return the_app.run();
} // end main



/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./tinyed-build.sh" ;;
 ** End: ;;
 ****************/
