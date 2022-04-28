// // file fltk-mini-edit.cc
// SPDX-License-Identifier: GPL-3.0-or-later

// A simple text editor program for the Fast Light Tool Kit (FLTK).
//
// This program is described in Chapter 4 of the FLTK Programmer's Guide.
//
// Copyright 1998-2022 by Bill Spitzak and Basile Starynkevitch and CEA

/// heavily inspired by texteditor-with-dynamic-colors.cxx from FLTK.

// Include necessary headers...
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

/// GNU libunistring https://www.gnu.org/software/libunistring/
#include <unistd.h>
#include <unicase.h>
#include <unictype.h>
#include <uniconv.h>
#include <unistr.h>

#include <string>


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

// Custom class to demonstrate a specialized text editor
class MyEditor : public Fl_Text_Editor
{

  Fl_Text_Buffer *txtbuff;      // text buffer
  Fl_Text_Buffer *stybuff;	// style buffer
public:
  void initialize(void);
  void ModifyCallback(int pos,        // position of update
                      int nInserted,  // number of inserted chars
                      int nDeleted,   // number of deleted chars
                      int,            // number of restyled chars (unused here)
                      const char*deltxt // deleted text
                     );
  static void static_modify_callback(int pos,                 // position of update
                                     int nInserted,           // number of inserted chars
                                     int nDeleted,            // number of deleted chars
                                     int nRestyled,           // number of restyled chars
                                     const char *deletedText, // text deleted
                                     void *cbarg)             // callback data
  {
    assert (cbarg != nullptr);
    MyEditor* med = reinterpret_cast<MyEditor*>(cbarg);
    med->ModifyCallback(pos, nInserted, nDeleted, nRestyled, deletedText);
  };
  MyEditor(int X,int Y,int W,int H)
    : Fl_Text_Editor(X,Y,W,H), txtbuff(nullptr), stybuff(nullptr)
  {
    txtbuff = new Fl_Text_Buffer();    // text buffer
    stybuff = new Fl_Text_Buffer();    // style buffer
    buffer(txtbuff);
    initialize();
  };
  ~MyEditor()
  {
    delete txtbuff;
    delete stybuff;
    txtbuff = nullptr;
    stybuff = nullptr;
  };
  void text(const char* val)
  {
    txtbuff->text(val);
  };
  enum my_style_en
  {
    Style_Plain,
    Style_Voyel,
    Style_Letter,
    Style_Digit,
    Style_Unicode,
    Style__LAST
  };
  static inline constexpr Fl_Text_Editor::Style_Table_Entry style_table[(unsigned)Style__LAST] =
  {
    // FONT COLOR      FONT FACE           SIZE  ATTRIBUTE      BACKGROUND COLOR
    // --------------- --------------      ----  ---------      -----------------
    [Style_Plain] = //
    {  FL_BLACK,       FL_COURIER,         17,   0,             FL_WHITE }, //:Style_Plain,
    [Style_Voyel] = //
    {  FL_DARK_GREEN,  FL_COURIER_BOLD,    17,   0,             FL_WHITE }, //:Style_Voyel,
    [Style_Letter] = //
    {  FL_DARK_BLUE,   FL_COURIER,         17,   0,             FL_WHITE }, //:Style_Letter,
    [Style_Digit] =  //
    {  FL_CYAN,        FL_COURIER,         17,   0,             FL_WHITE }, //:Style_Digit,
    [Style_Unicode] = //
    {  FL_DARK_RED,    FL_HELVETICA_BOLD,  17,   ATTR_BGCOLOR,  FL_GRAY0 }, //:Style_Unicode,
  };
  static inline constexpr const char*stylename_table[(unsigned)Style__LAST+1] =
  {
#define NAME_STYLE(N) [(int)N] = #N
    NAME_STYLE(Style_Plain),
    NAME_STYLE(Style_Voyel),
    NAME_STYLE(Style_Letter),
    NAME_STYLE(Style_Digit),
    NAME_STYLE(Style_Unicode),
    nullptr
  };
  void decorate(void);
};				// end MyEditor



void
MyEditor::initialize()
{
  assert (txtbuff);
  assert (stybuff);
  highlight_data(stybuff, style_table,
                 (unsigned)Style__LAST-1,
                 'A', 0, 0);
  txtbuff->add_modify_callback(MyEditor::static_modify_callback, (void*)this);
} // end MyEditor::initialize


void
MyEditor::ModifyCallback(int pos,        // position of update
                         int nInserted,  // number of inserted chars
                         int nDeleted,   // number of deleted chars
                         int nRestyled,            // number of restyled chars (unused here)
                         const char*deltxt // deleted text
                        )
{
  printf("MyEditor::ModifyCallback[L%d] pos=%d ninserted=%d ndeleted=%d nrestyled=%d deltxt=%s\n", __LINE__,
         pos, nInserted, nDeleted, nRestyled, deltxt);
  decorate();
} // end MyEditor::ModifyCallback

void
MyEditor::decorate(void)
{
  Fl_Text_Buffer*buf = buffer();
  Fl_Text_Buffer*stybuf = style_buffer();
  assert (buf != nullptr);
  assert (stybuf != nullptr);
  int curix= -1, previx= -1;
  int buflen = buf->length();
  for (curix=0;
       curix>=0 && curix<buflen;
       curix=buf->next_char(curix))
    {
      if (previx == curix)
        break;
      unsigned int curch = buf->char_at(curix);
      assert (curch>0);
      char utfbuf[8];
      memset (utfbuf, 0, sizeof(utfbuf));
      u8_uctomb((uint8_t*)utfbuf, (ucs4_t)curch, sizeof(utfbuf));
      enum my_style_en cursty = Style_Plain;
      if (curch < 0x7F)
        {
          if (strchr("aeiouyAEIOUY", (int)curch))
            cursty = Style_Voyel;
          else if (isalpha(curch))
            cursty = Style_Letter;
          else if (isdigit(curch))
            cursty = Style_Digit;
        }
      else
        cursty = Style_Unicode;
      //Fl_Text_Editor::Style_Table_Entry curstyent = style_table[cursty];
      if (cursty != Style_Plain)
        {
          char stychar[4];
          memset (stychar, 0, sizeof(stychar));
          stychar[0] = 'A' + cursty;
          stybuf->replace(curix, curix+1, stychar);
          printf("MyEditor::decorate Unicode %s at %d=%#x style#%d/%s (%s:%d)\n",
                 utfbuf, curix, curix, (int)cursty,
                 stylename_table[(int)cursty],
                 __FILE__,__LINE__);
        }
      else
        {
          if (curch < ' ')
            printf("MyEditor::decorate Ctrlchar \\x%02x at %d=%#x plain_style (%s:%d)\n",
                   (int)curch, curix, curix, __FILE__,__LINE__);
          else
            printf("MyEditor::decorate Unicode %s at %d=%#x plain_style (%s:%d)\n",
                   utfbuf, curix, curix,
                   __FILE__,__LINE__);
        }
      previx= curix;
    };
#warning incomplete MyEditor::decorate
} // end MyEditor::decorate

int
main(int argc, char **argv)
{
  std::string tistr = __FILE__;
  tistr.erase(sizeof(__FILE__)-4,3);
#ifdef GITID
  tistr += "/" GITID;
#endif
  Fl_Window *win = new Fl_Window(720, 480, tistr.c_str());
  MyEditor  *med = new MyEditor(10,10,win->w()-20,win->h()-20);
  med->text("Test\n"
            "Other\n"
            "0123456789\n"
            "°§ +\n");
  med->initialize();
  med->decorate();
  win->resizable(med);
  win->show();
  return Fl::run();
}  // end main

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./mini-edit-build.sh" ;;
 ** End: ;;
 ****************/
