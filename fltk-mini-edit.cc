// // file fltk-mini-edit.cc
// SPDX-License-Identifier: GPL-3.0-or-later

// A simple text editor program for the Fast Light Tool Kit (FLTK).
//
// This program is described in Chapter 4 of the FLTK Programmer's Guide.
//
// Copyright 1998-2022 by Bill Spitzak and Basile Starynkevitch and CEA



// Include necessary headers...
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <FL/Fl.H>
#include <FL/platform.H> // for fl_open_callback
#include <FL/Fl_Group.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/filename.H>

int main(int argc, char **argv)
{
  auto textbuf = new Fl_Text_Buffer;
  return Fl::run();
}

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./mini-edit-build.sh" ;;
 ** End: ;;
 ****************/
