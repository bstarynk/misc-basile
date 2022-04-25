// file misc-basile/fox-tinyed.cc
// SPDX-License-Identifier: GPL-3.0-or-later
//  © Copyright 2022 Basile Starynkevitch (&Jeroen van der Zijp)
//  some code taken from fox-toolkit.org Adie
#include <fstream>
/// fox-toolkit.org
#include <fx.h>

// Editor main window
class TinyTextWindow : public FXMainWindow
{
  FXDECLARE(TinyTextWindow);
  ///
  FXHorizontalFrame   *editorframe;             // Editor frame
  FXText              *editor;                  // Editor text widget
protected:
  TinyTextWindow(): FXMainWindow(), editorframe(nullptr), editor(nullptr) {};
public:
  TinyTextWindow(FXApp *theapp);
  virtual ~TinyTextWindow();
  virtual void create();
};				// end TinyTextWindow

FXIMPLEMENT(TinyTextWindow,FXMainWindow,nullptr, 0);

void
TinyTextWindow::create()
{
  FXMainWindow::create();
  show(PLACEMENT_SCREEN);
} // end TinyTextWindow::create()

TinyTextWindow::TinyTextWindow(FXApp* theapp)
  : FXMainWindow(theapp,"tiny-text-fox",nullptr, nullptr,
                 0, 0, 450, 333),
    editorframe(nullptr), editor(nullptr)
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
  delete editorframe;
  delete editor;
}; // end TinyTextWindow::~TinyTextWindow

int
main(int argc, char*argv[])
{
  FXApp the_app("fox-tinyed","FOX tinyed (Basile)");
  the_app.init(argc, argv);
  new TinyTextWindow(&the_app);
  the_app.create();
  return the_app.run();
} // end main



/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./tinyed-build.sh" ;;
 ** End: ;;
 ****************/
