// // file fltk-mini-edit.cc
// SPDX-License-Identifier: GPL-3.0-or-later

// A simple text editor program for the Fast Light Tool Kit (FLTK).
//
// This program is inspired by Chapter 4 of the FLTK Programmer's Guide.
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

/// https://github.com/ianlancetaylor/libbacktrace/
#include <backtrace.h>

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
struct backtrace_state *my_backtrace_state;

#define MY_BACKTRACE_PRINT(Skip) my_backtrace_print_at(__LINE__, (Skip))
void my_backtrace_print_at(int line, int skip);

// Custom class to demonstrate a specialized text editor
class MyEditor : public Fl_Text_Editor
{

  Fl_Text_Buffer *txtbuff;      // text buffer
  Fl_Text_Buffer *stybuff;	// style buffer
  static int tab_key_binding(int key, Fl_Text_Editor*editor);
  static int escape_key_binding(int key, Fl_Text_Editor*editor);
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
    MY_BACKTRACE_PRINT(1);
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


int
MyEditor::tab_key_binding(int key, Fl_Text_Editor*editor)
{
  MyEditor* myed = dynamic_cast<MyEditor*>(editor);
  MY_BACKTRACE_PRINT(1);
  assert (myed != nullptr);
  int inspos = myed->insert_position();
  int lin= -1, col= -1;
  if (myed->position_to_linecol(inspos, &lin, &col))
    {
      Fl_Text_Buffer* mybuf = myed->buffer();
      assert (mybuf != nullptr);
      unsigned int curutf = mybuf->char_at(inspos);
      int wstart = mybuf->word_start(inspos);
      int wend = mybuf->word_end(inspos);
      int linend = mybuf->line_end(inspos);
      printf("MyEditor TAB [%s:%d] inspos=%d L%dC%d curutf#%d wstart=%d wend=%d linend=%d\n",
             __FILE__, __LINE__, //
             inspos, lin, col, curutf, wstart, wend, linend);
      if (wend > linend)
        wend = linend;
      const char*startchr = mybuf->address(wstart);
      const char*endchr = mybuf->address(wend);
      assert(startchr != nullptr);
      assert(endchr != nullptr);
      assert (endchr >= startchr);
      char*curword = mybuf->text_range(wstart, wend);
      char*prefix = mybuf->text_range(wstart, inspos);
      printf("MyEditor TAB [%s:%d] inspos=%d word '%s' prefix='%s'\n",
             __FILE__, __LINE__,
             inspos, curword, prefix);
      bool prefixdigits = true;
      bool prefixletters = true;
      for (const char*pc = prefix; *pc; pc++)
        {
          if (!isalpha(*pc)) prefixletters = false;
          if (!isdigit(*pc)) prefixdigits = false;
        };
      char* prefixrepl = nullptr; // possible replacement of prefix
      /// If the prefix is only made of digits replace it (as a
      /// number) by its numerical successor.
      if (prefixdigits)
        {
          asprintf(&prefixrepl, "%d", atoi(prefix)+1);
        }
      /// If the prefix is only letters replace it by a x2 duplicate
      else if (prefixletters)
        asprintf(&prefixrepl, "%s/%s", prefix, prefix);
      if (prefixrepl)
        {
          printf("MyEditor TAB [%s:%d] inspos=%d prefixrepl:%s\n",
                 __FILE__, __LINE__,
                 inspos, prefixrepl);
#warning missing actual replacement of prefix in MyEditor::tab_key_binding
          free (prefixrepl), prefixrepl = nullptr;
        }
      free (curword), curword = nullptr;
      free (prefix), prefix = nullptr;
    }
  else
    printf("MyEditor TAB [%s:%d] inspos=%d not shown\n", __FILE__, __LINE__,
           inspos);
  fflush(stdout);
  return 0; /// this means don't handle
} // end MyEditor::tab_key_binding

int
MyEditor::escape_key_binding(int key, Fl_Text_Editor*editor)
{
  MyEditor* myed = dynamic_cast<MyEditor*>(editor);
  MY_BACKTRACE_PRINT(1);
  assert (myed != nullptr);
  return 1; // this means do handle the binding
} // end MyEditor::escape_key_binding

void
MyEditor::initialize()
{
  assert (txtbuff);
  assert (stybuff);
  highlight_data(stybuff, style_table,
                 (unsigned)Style__LAST,
                 'A', 0, 0);
  txtbuff->add_modify_callback(MyEditor::static_modify_callback, (void*)this);
  // the 0 is a state, could be FL_SHIFT etc... See Fl/Enumerations.h
  add_key_binding(FL_Tab, 0, tab_key_binding);
  add_key_binding(FL_Escape, 0, escape_key_binding);
  MY_BACKTRACE_PRINT(1);
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
  MY_BACKTRACE_PRINT(1);
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
            {
              const char*escstr = "";
              switch (curch)
                {
                case '\t':
                  escstr = "=\\t";
                  break;
                case '\r':
                  escstr = "=\\r";
                  break;
                case '\n':
                  escstr = "=\\n";
                  break;
                case '\v':
                  escstr = "=\\v";
                  break;
                case '\b':
                  escstr = "=\\b";
                  break;
                case '\f':
                  escstr = "=\\f";
                  break;
                case '\e':
                  escstr = "=\\e";
                  break;
                default:
                  break;
                };
              printf("MyEditor::decorate Ctrlchar \\x%02x%s at %d=%#x plain_style (%s:%d)\n",
                     (int)curch, escstr, curix, curix, __FILE__,__LINE__);
            }
          else
            printf("MyEditor::decorate Unicode %s at %d=%#x plain_style (%s:%d)\n",
                   utfbuf, curix, curix,
                   __FILE__,__LINE__);
        }
      previx= curix;
    };
#warning incomplete MyEditor::decorate
} // end MyEditor::decorate


void my_backtrace_error(void*data, const char*msg, int errnum)
{
  fprintf(stderr, "backtrace error %s (errnum=%d:%s)", msg,
          errnum, (errnum>=0)?strerror(errnum):"BUG");
  fflush(nullptr);
  exit(EXIT_FAILURE);
} // end my_backtrace_error

void my_backtrace_print_at(int line, int skip)
{
  printf("%s:%d backtrace\n", __FILE__, line);
  backtrace_print(my_backtrace_state, skip, stdout);
  fflush(NULL);
} // end my_backtrace_print_at

int
main(int argc, char **argv)
{
  std::string tistr = __FILE__;
  tistr.erase(sizeof(__FILE__)-4,3);
#ifdef GITID
  tistr += "/" GITID;
#endif
  my_backtrace_state = //
    backtrace_create_state
    (
      "fltk-mini-edit", /*threaded:*/0,
      my_backtrace_error, nullptr);
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
