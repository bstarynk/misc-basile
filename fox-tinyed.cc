// file misc-basile/fox-tinyed.cc
// SPDX-License-Identifier: GPL-3.0-or-later
//  © Copyright 2022 Basile Starynkevitch (&Jeroen van der Zijp)
//  some code taken from fox-toolkit.org Adie
#include <fstream>
#include <iostream>
/// fox-toolkit.org
#include <fx.h>


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
                          1, 1, // x,y
                          448, 330 //w,h
                         );
  editor = //
    new FXText (editorframe, 0,
                TEXT_READONLY|TEXT_WORDWRAP|LAYOUT_FILL_X|LAYOUT_FILL_Y, //
                2, 2, //x,y
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

int
main(int argc, char*argv[])
{
  FXApp the_app("fox-tinyed","FOX tinyed (Basile)");
  the_app.init(argc, argv);
  the_app.create();
  auto mywin = new TinyTextWindow(&the_app);
  mywin->create();
  mywin->layout();
  mywin->show();
  //mywin->enable();
  std::cout << __FILE__ ":" << __LINE__ << " "
            << "mywin:" << mywin << std::endl;
  return the_app.run();
} // end main



/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./tinyed-build.sh" ;;
 ** End: ;;
 ****************/
