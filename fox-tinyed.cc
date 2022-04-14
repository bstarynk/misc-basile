// file misc-basile/fox-tinyed.cc
// SPDX-License-Identifier: GPL-3.0-or-later
//  © Copyright 2022 Basile Starynkevitch (&Jeroen van der Zijp)
//  some code taken from fox-toolkit.org Adie
#include <ofstream>
/// fox-toolkit.org
#include <FX.h>

// Editor main window
class TinyTextWindow : public FXMainWindow {
  FXDECLARE(TextWindow);
  FXHorizontalFrame   *editorframe;             // Editor frame
  FXText              *editor;                  // Editor text widget
public:
  TinyTextWindow();
  virtual ~TinyTextWindow();
};				// end TinyTextWindow

TinyTextWindow::TinyTextWindow()
  : FXMainWindow(), editorframe(nullptr), editor(null)
{
} // end TinyTextWindow::TinyTextWindow

TinyTextWindow::~TinyTextWindow()
{
} // end TinyTextWindow::~TinyTextWindow
