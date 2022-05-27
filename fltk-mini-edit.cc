// // file fltk-mini-edit.cc
// SPDX-License-Identifier: GPL-3.0-or-later

// A simple text editor program for the Fast Light Tool Kit (FLTK). It
// should interact with another Linux process using some protocol to
// be defined...
//
// This program is inspired by Chapter 4 of the FLTK Programmer's Guide.
/****
   © Copyright  1998-2022 by Bill Spitzak and Basile Starynkevitch and CEA
   program released under GNU general public license

   this is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   this is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.
****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gnu/libc-version.h>

/// GNU libunistring https://www.gnu.org/software/libunistring/
#include <unistd.h>
#include <unicase.h>
#include <unictype.h>
#include <uniconv.h>
#include <unistr.h>

#include <string>
#include <iostream>
#include <cassert>

/// https://github.com/ianlancetaylor/libbacktrace/
#include <backtrace.h>

/// JSONCPP from https://github.com/open-source-parsers/jsoncpp
#include <json/json.h>
#include <json/version.h>

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

extern "C" struct backtrace_state *my_backtrace_state;
extern "C" const char*my_prog_name;
extern "C" char my_host_name[];
extern "C" bool my_help_flag;
extern "C" bool my_version_flag;
extern "C" bool my_debug_flag;
extern "C" bool my_styledemo_flag;
extern "C" Fl_Window *my_top_window;
extern "C" Fl_Menu_Bar*my_menubar;
extern "C" const int my_font_delta;
extern "C" const char* my_fifo_name;

/// a small integer to increase font size at compile time, e.g. compiling with -DMY_FONT_DELTA=2

#ifndef MY_FONT_DELTA
#define MY_FONT_DELTA 0
#endif

#define MY_FIFO_NAME_MAXLEN 250
#define MY_FONT_SIZE(Fsiz) ((int)(MY_FONT_DELTA)+(Fsiz))

/// from www.december.com/html/spec/colorsvghex.html
#ifndef FL_ORANGE
#define FL_ORANGE 0xffA500
#endif

#ifndef FL_PURPLE
#define FL_PURPLE 0x800080
#endif

#ifndef FL_GOLDENROD
#define FL_GOLDENROD 0xDAA520
#endif

#ifndef FL_LIGHTCYAN
#define FL_LIGHTCYAN 0xE0FFFF
#endif

#ifndef FL_SLATEBLUE
#define FL_SLATEBLUE 0x6A5ACD
#endif


const Fl_Font MYFL_FREEMONO_FONT = FL_FREE_FONT;
const Fl_Font MYFL_FREEMONO_BOLD_FONT = FL_FREE_FONT+1;
const Fl_Font MYFL_GOMONO_FONT = FL_FREE_FONT+2;
const Fl_Font MYFL_DEJAVUSANSMONO_FONT = FL_FREE_FONT+2;
const Fl_Font MYFL_XTRAFONT = FL_FREE_FONT+3;
const Fl_Font MYFL_OTHERFONT = FL_FREE_FONT+4;

#define MY_BACKTRACE_PRINT(Skip) do {if (my_debug_flag) \
      my_backtrace_print_at(__FILE__,__LINE__, (Skip)); } while (0)

extern "C" void my_backtrace_print_at(const char*fil, int line, int skip);

extern "C" int miniedit_prog_arg_handler(int argc, char **argv, int &i);

#define FATALPRINTF(Fmt,...) do {		\
    printf("\n@@@FATAL ERROR (%s pid %d):",	\
	   my_prog_name, (int)getpid());	\
    printf((Fmt), ##__VA_ARGS__);		\
    putchar('\n');				\
    my_backtrace_print_at(__FILE__,__LINE__,	\
			  (1));			\
    fflush(nullptr);				\
    abort(); } while(0)

#define DBGPRINTF(Fmt,...) do {if (my_debug_flag) { \
    printf("@@%s:%d:", __FILE__, __LINE__); \
    printf((Fmt), ##__VA_ARGS__); \
    putchar('\n'); fflush(stdout); \
    }} while(0)

#define DBGOUT(Out) do { if (my_debug_flag) { \
      std::cout << "@@" __FILE__ << ":" << __LINE__ << ':' \
		<< Out << std::endl; \
    }} while(0)


class MyEditor;

extern "C" void do_style_demo(MyEditor*);
extern "C" void my_initialize_fifo(void);
extern "C" int fifo_cmd_fd, fifo_out_fd;
extern "C" void my_cmd_fd_handler(FL_SOCKET, void*);
extern "C" void my_out_fd_handler(FL_SOCKET, void*);

// Custom class to demonstrate a specialized text editor
class MyEditor : public Fl_Text_Editor
{
  friend void do_style_demo(MyEditor*);
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
                                     void *cbarg);             // callback data

  MyEditor(int X,int Y,int W,int H);
  ~MyEditor();
  inline void text(const char* val)
  {
    txtbuff->text(val);
  };
  enum my_style_en
  {
    Style_Plain,
    Style_Literal,
    Style_Number,
    Style_Name,
    Style_Word,
    Style_Keyword,
    Style_Oid,
    Style_Comment,
    Style_Bold,
    Style_Italic,
    Style_CodeChunk,
    Style_Unicode,
    Style_Errored,
    Style__LAST
  };
  static inline constexpr Fl_Text_Editor::Style_Table_Entry style_table[(unsigned)Style__LAST] =
  {
    // FONT COLOR      FONT FACE           SIZE  ATTRIBUTE      BACKGROUND COLOR
    // --------------- --------------      ----  ---------      -----------------
    [Style_Plain] = //
    {  FL_BLACK,       FL_COURIER,         	MY_FONT_SIZE(17),   0,             FL_WHITE }, //:Style_Plain,
    [Style_Literal] = //
    {  FL_DARK_GREEN,  FL_COURIER_BOLD,    	MY_FONT_SIZE(17),   0,             FL_WHITE }, //:Style_Literal,
    [Style_Number] = //
    {  FL_DARK_BLUE,   FL_COURIER,         	MY_FONT_SIZE(17),   0,             FL_WHITE }, //:Style_Number,
    [Style_Name] =  //
    {  FL_CYAN,        FL_COURIER,         	MY_FONT_SIZE(17),   0,             FL_WHITE }, //:Style_Name,
    [Style_Word] =  //
    {  FL_BLUE,        FL_COURIER,         	MY_FONT_SIZE(17),   0,             FL_WHITE }, //:Style_Word,
    [Style_Keyword] =  //
    {  FL_DARK_MAGENTA, FL_COURIER,        	MY_FONT_SIZE(17),   0,             FL_WHITE }, //:Style_Keyword,
    [Style_Oid] =  //
    {  FL_ORANGE,        FL_COURIER,            MY_FONT_SIZE(17),   0,             FL_WHITE }, //:Style_Oid,
    [Style_Comment] =  //
    {  FL_PURPLE,        FL_COURIER,            MY_FONT_SIZE(17),   0,             FL_WHITE }, //:Style_Comment,
    [Style_Bold] =  //
    {  FL_BLACK,        FL_COURIER_BOLD,        MY_FONT_SIZE(17),   0,             FL_WHITE }, //:Style_Bold,
    [Style_Italic] =  //
    {  FL_BLACK,        FL_COURIER_ITALIC,      MY_FONT_SIZE(17),   0,             FL_WHITE }, //:Style_Italic,
    [Style_CodeChunk] =  //
    {  FL_SLATEBLUE,        FL_COURIER,         MY_FONT_SIZE(17),   0,             FL_WHITE }, //:Style_CodeChunk,
    [Style_Unicode] = //
    {  FL_DARK_RED,    MYFL_FREEMONO_BOLD_FONT, MY_FONT_SIZE(17),   0,  FL_GRAY0 }, //:Style_Unicode,
    [Style_Errored] = //
    {  FL_RED,        FL_COURIER_BOLD,          MY_FONT_SIZE(17),   ATTR_BGCOLOR,  FL_GRAY0 }, //:Style_Errored,
  };
  static inline constexpr const char*stylename_table[(unsigned)Style__LAST+1] =
  {
#define NAME_STYLE(N) [(int)N] = #N
    NAME_STYLE(Style_Plain),
    NAME_STYLE(Style_Literal),
    NAME_STYLE(Style_Number),
    NAME_STYLE(Style_Name),
    NAME_STYLE(Style_Word),
    NAME_STYLE(Style_Keyword),
    NAME_STYLE(Style_Oid),
    NAME_STYLE(Style_Comment),
    NAME_STYLE(Style_Bold),
    NAME_STYLE(Style_Italic),
    NAME_STYLE(Style_CodeChunk),
    NAME_STYLE(Style_Unicode),
    NAME_STYLE(Style_Errored),
    nullptr
  };
  void decorate(void);
};				// end MyEditor


extern "C" const int last_shared_line = __LINE__;

Json::CharReaderBuilder my_json_cmd_builder;
Json::StreamWriterBuilder my_json_out_builder;
char* my_shell_command;
char* my_xtrafont_name;
char* my_otherfont_name;

// we could compile and dlopen extra C++ plugins, sharing all the code above....

MyEditor::MyEditor(int X,int Y,int W,int H)
  : Fl_Text_Editor(X,Y,W,H), txtbuff(nullptr), stybuff(nullptr)
{
  txtbuff = new Fl_Text_Buffer();    // text buffer
  stybuff = new Fl_Text_Buffer();    // style buffer
  buffer(txtbuff);
  initialize();
} // end MyEditor::MyEditor

MyEditor::~MyEditor()
{
  delete txtbuff;
  delete stybuff;
  txtbuff = nullptr;
  stybuff = nullptr;
} // end MyEditor::~MyEditor

void
MyEditor::static_modify_callback(int pos,                 // position of update
                                 int nInserted,           // number of inserted chars
                                 int nDeleted,            // number of deleted chars
                                 int nRestyled,           // number of restyled chars
                                 const char *deletedText, // text deleted
                                 void *cbarg)
{
  assert (cbarg != nullptr);
  MyEditor* med = reinterpret_cast<MyEditor*>(cbarg);
  MY_BACKTRACE_PRINT(1);
  med->ModifyCallback(pos, nInserted, nDeleted, nRestyled, deletedText);
};

int
MyEditor::tab_key_binding(int key, Fl_Text_Editor*editor)
{
  MyEditor* myed = dynamic_cast<MyEditor*>(editor);
  assert (key == FL_Tab);
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
      int insmode = myed->insert_mode();
      DBGPRINTF("MyEditor TAB key=%d inspos=%d L%dC%d curutf#%d wstart=%d"
                " wend=%d linend=%d insmode=%d",
                key,
                inspos, lin, col, curutf, wstart, wend, linend, insmode);
      if (wend > linend)
        wend = linend;
      const char*startchr = mybuf->address(wstart);
      const char*endchr = mybuf->address(wend);
      assert(startchr != nullptr);
      assert(endchr != nullptr);
      DBGPRINTF("MyEditor TAB inspos=%d startchr:%p endchr:%p",
                inspos, startchr, endchr);
      if (startchr > endchr)
        {
          // this happens when TAB is pressed with the cursor at end
          // of line...
          DBGPRINTF("MyEditor TAB inspos=%d endofline#%d",
                    inspos, lin);
          return 0; // don't handle
        }

      assert (endchr >= startchr);
      char*curword = mybuf->text_range(wstart, wend);
      char*prefix = mybuf->text_range(wstart, inspos);
      DBGPRINTF("MyEditor TAB inspos=%d word '%s' prefix='%s'",
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
          int ok = asprintf(&prefixrepl, "%d", atoi(prefix)+1);
          assert (ok > 0);
        }
      /// If the prefix is only letters replace it by a x2 duplicate
      else if (prefixletters)
        {
          int ok = asprintf(&prefixrepl, "%s/%s", prefix, prefix);
          assert (ok > 0);
        }
      if (prefixrepl)
        {
          DBGPRINTF("MyEditor TAB inspos=%d prefixrepl:%s",
                    inspos, prefixrepl);
          mybuf->remove(wstart, inspos);
          mybuf->insert(wstart, prefixrepl);
#warning missing style insertion
          free (prefixrepl), prefixrepl = nullptr;
        }
      free (curword), curword = nullptr;
      free (prefix), prefix = nullptr;
    }
  else
    DBGPRINTF("MyEditor TAB inspos=%d not shown",   inspos);
  fflush(stdout);
  return 0; /// this means don't handle
} // end MyEditor::tab_key_binding

int
MyEditor::escape_key_binding(int key, Fl_Text_Editor*editor)
{
  MyEditor* myed = dynamic_cast<MyEditor*>(editor);
  MY_BACKTRACE_PRINT(1);
  assert (myed != nullptr);
  assert (key ==  FL_Escape);
  DBGPRINTF("MyEditor::escape_key_binding myed@%p", myed);
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
  DBGPRINTF("MyEditor::ModifyCallback pos=%d ninserted=%d ndeleted=%d"
            " nrestyled=%d deltxt=%s",
            pos, nInserted, nDeleted, nRestyled, deltxt);
  MY_BACKTRACE_PRINT(1);
  decorate();
} // end MyEditor::ModifyCallback

void
MyEditor::decorate(void)
{
  if (my_styledemo_flag)
    return;
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
            cursty = Style_Name;
          else if (isalpha(curch))
            cursty = Style_Literal;
          else if (isdigit(curch))
            cursty = Style_Number;
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
          DBGPRINTF("MyEditor::decorate Unicode %s at %d=%#x style#%d/%s",
                    utfbuf, curix, curix, (int)cursty,
                    stylename_table[(int)cursty]);
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
              DBGPRINTF("MyEditor::decorate Ctrlchar \\x%02x%s "
                        "at %d=%#x plain_style",
                        (int)curch, escstr, curix, curix);
            }
          else
            DBGPRINTF("MyEditor::decorate Unicode %s at %d=%#x plain_style",
                      utfbuf, curix, curix);
        }
      previx= curix;
    };
#warning incomplete MyEditor::decorate
} // end MyEditor::decorate

void
do_style_demo(MyEditor*med)
{
  assert (med != nullptr);
  assert (med->txtbuff != nullptr);
  assert (med ->stybuff != nullptr);
  \
#define DEMO_STYLE(S) do {			\
    int sl = strlen(#S);			\
    char sbuf[sizeof(#S)+1];			\
    memset(sbuf, 0, sizeof(sbuf));		\
    med->txtbuff->append(#S);			\
    for (int i=0; i<sl; i++)			\
      sbuf[i] = 'A'+(int)MyEditor::S;		\
  med->txtbuff->append("\n");			\
  med->stybuff->append(sbuf);			\
  med->stybuff->append("A");			\
  DBGPRINTF("DEMO_STYLE " # S			\
	    " txtbuff.len %d stybuff.len %d",	\
	    med->txtbuff->length(),		\
	    med->stybuff->length());		\
  } while(0);
  /* starting demo styles */
  DEMO_STYLE(Style_Plain);
  DEMO_STYLE(Style_Literal);
  DEMO_STYLE(Style_Number);
  DBGPRINTF("after Style_Number txtbuff.len %d stybuff.len %d", med->txtbuff->length(), med->stybuff->length());
  DEMO_STYLE(Style_Name);
  DEMO_STYLE(Style_Word);
  DBGPRINTF("after Style_Word txtbuff.len %d stybuff.len %d", med->txtbuff->length(), med->stybuff->length());
  DEMO_STYLE(Style_Keyword);
  DEMO_STYLE(Style_Oid);
  DEMO_STYLE(Style_Comment);
  DBGPRINTF("after Style_Comment txtbuff.len %d stybuff.len %d", med->txtbuff->length(), med->stybuff->length());
  DEMO_STYLE(Style_Bold);
  DEMO_STYLE(Style_Italic);
  DEMO_STYLE(Style_CodeChunk);
  DBGPRINTF("after Style_CodeChunk txtbuff.len %d stybuff.len %d", med->txtbuff->length(), med->stybuff->length());
#define DEMO_UNICODE_STR "§²ç°∃⁑" //∃⁑
  int ulen = (int) u8_mbsnlen((const uint8_t*)DEMO_UNICODE_STR, sizeof(DEMO_UNICODE_STR)-1);
  DBGPRINTF("do_style_demo DEMO_UNICODE_STR (%d bytes, %d utf8chars) %s"
            " txtbuff.len %d stybuff.len %d",
            (int) strlen(DEMO_UNICODE_STR), ulen,
            DEMO_UNICODE_STR,
            med->txtbuff->length(), med->stybuff->length());
  med->txtbuff->append(DEMO_UNICODE_STR);
  char sb[1+((sizeof(DEMO_UNICODE_STR)+4)|0xf)
         ]= {0};
  static_assert (sizeof(sb) > sizeof(DEMO_UNICODE_STR));
  for (int i=0; i<(int)strlen(DEMO_UNICODE_STR); i++)
    sb[i] = 'A'+(int)MyEditor::Style_Unicode;
  med->stybuff->append(sb);
  DBGPRINTF("before DEMO_STYLE Style_Unicode txtbuff.len %d stybuff.len %d sb '%s'",
            med->txtbuff->length(), med->stybuff->length(), sb);
  DEMO_STYLE(Style_Unicode);
  DBGPRINTF("after Style_Unicode txtbuff.len %d stybuff.len %d",
            med->txtbuff->length(), med->stybuff->length());
  DEMO_STYLE(Style_Errored);
  DBGPRINTF("ending do_style_demo txtbuff.len %d stybuff.len %d",
            med->txtbuff->length(), med->stybuff->length());
#undef DEMO_STYLE
} // end do_style_demo

void
my_backtrace_error(void*data, const char*msg, int errnum)
{
  fprintf(stderr, "backtrace error %s (errnum=%d:%s) data:%p", msg,
          errnum, (errnum>=0)?strerror(errnum):"BUG",
          data);
  fflush(nullptr);
  exit(EXIT_FAILURE);
} // end my_backtrace_error

void
my_backtrace_print_at(const char*fil, int line, int skip)
{
  printf("%s:%d backtrace\n", fil, line);
  backtrace_print(my_backtrace_state, skip, stdout);
  fflush(NULL);
} // end my_backtrace_print_at


void
my_initialize_fifo(void)
{
  assert (fifo_cmd_fd < 0);
  assert (fifo_out_fd < 0);
  assert (my_fifo_name != nullptr);
  std::string fifo_str {my_fifo_name};
  std::string fifo_cmd_str = fifo_str + ".cmd";
  std::string fifo_out_str = fifo_str + ".out";
  struct stat fifo_cmd_stat = {};
  struct stat fifo_out_stat = {};
  DBGPRINTF("my_initialize_fifo fifo_cmd '%s'",
            fifo_cmd_str.c_str());
  if (!stat(fifo_cmd_str.c_str(), &fifo_cmd_stat))
    {
      // should check that it is a FIFO
      mode_t cmd_mod = fifo_cmd_stat.st_mode;
      if ((cmd_mod&S_IFMT) != S_IFIFO)
        FATALPRINTF("%s is not a FIFO but should be the command FIFO - %m",
                    fifo_cmd_str.c_str());
      fifo_cmd_fd = open(fifo_cmd_str.c_str(), O_RDONLY);
      if (fifo_cmd_fd < 0)
        FATALPRINTF("failed to open command FIFO %s - %m", fifo_cmd_str.c_str());
    }
  else
    {
      // should create the cmd FIFO
      fifo_cmd_fd = mkfifo(fifo_cmd_str.c_str(), S_IRUSR|S_IWUSR);
      if (fifo_cmd_fd < 0)
        FATALPRINTF("failed to make command FIFO %s - %m", fifo_cmd_str.c_str());
      printf("%s: (pid %d on %s) created command FIFO %s\n",
             my_prog_name, (int)getpid(), my_host_name, fifo_cmd_str.c_str());
      fflush(stdout);
    };
  DBGPRINTF("my_initialize_fifo fifo_out '%s'",
            fifo_out_str.c_str());
  if (!stat(fifo_out_str.c_str(), &fifo_out_stat))
    {
      // should check that it is a FIFO
      mode_t out_mod = fifo_out_stat.st_mode;
      if ((out_mod&S_IFMT) != S_IFIFO)
        FATALPRINTF("%s is not a FIFO but should be the output FIFO",
                    fifo_out_str.c_str());
      fifo_out_fd = open(fifo_out_str.c_str(), O_RDONLY);
      if (fifo_out_fd < 0)
        FATALPRINTF("failed to open output FIFO %s - %m", fifo_out_str.c_str());
    }
  else
    {
      // should create the output FIFO
      fifo_out_fd = mkfifo(fifo_out_str.c_str(), S_IRUSR|S_IWUSR);
      if (fifo_out_fd < 0)
        FATALPRINTF("failed to make output FIFO %s - %m", fifo_out_str.c_str());
      printf("%s: (pid %d on %s) created output FIFO %s\n",
             my_prog_name, (int)getpid(), my_host_name, fifo_out_str.c_str());
      fflush(stdout);
    };
  DBGPRINTF("my_initialize_fifo fifo_cmd_fd#%d fifo_out_fd#%d", fifo_cmd_fd, fifo_out_fd);
  if (my_debug_flag)
    {
      char cmdbuf[80];
      memset(cmdbuf, 0, sizeof(cmdbuf));
      snprintf(cmdbuf, sizeof(cmdbuf), "/bin/ls -l /proc/%d/fd/", (int)getpid());
      system(cmdbuf);
    }
  assert (fifo_cmd_fd > 0);
  assert (fifo_out_fd > 0);
  assert (fifo_cmd_fd != fifo_out_fd);
  Fl::add_fd(fifo_cmd_fd, FL_READ, my_cmd_fd_handler, nullptr);
  Fl::add_fd(fifo_out_fd, FL_WRITE, my_out_fd_handler, nullptr);
} // end my_initialize_fifo

void
my_cmd_fd_handler(FL_SOCKET sock, void*)
{
#warning my_cmd_fd_handler unimplemented
  assert(sock == fifo_cmd_fd);
  FATALPRINTF("my_cmd_fd_handler unimplemented sock#%d", sock);
} // end my_cmd_fd_handler

void
my_out_fd_handler(FL_SOCKET sock, void*)
{
#warning my_out_fd_handler unimplemented
  assert(sock == fifo_out_fd);
  FATALPRINTF("my_out_fd_handler unimplemented sock#%d", sock);
} // end my_cmd_fd_handler

int
miniedit_prog_arg_handler(int argc, char **argv, int &i)
{
  if (i >= argc)
    return 0;
  if (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0)
    {
      my_help_flag = 1;
      i += 1;
      return 1;
    }
  if (strcmp("-V", argv[i]) == 0 || strcmp("--version", argv[i]) == 0)
    {
      my_version_flag = 1;
      i += 1;
      return 1;
    }
  if (strcmp("-D", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0)
    {
      my_debug_flag = 1;
      i += 1;
      return 1;
    }
  if (strcmp("-Y", argv[i]) == 0 || strcmp("--style-demo", argv[i]) == 0)
    {
      my_styledemo_flag = 1;
      i += 1;
      return 1;
    }
  if (strcmp("--do", argv[i]) == 0 && i+1<argc)
    {
      my_shell_command = argv[i+1];
      i += 2;
      return 2;
    }
  if (strcmp("--xtrafont", argv[i]) == 0 && i+1<argc)
    {
      my_xtrafont_name = argv[i+1];
      i += 2;
      return 2;
    }
  if (strcmp("--otherfont", argv[i]) == 0 && i+1<argc)
    {
      my_otherfont_name = argv[i+1];
      i += 2;
      return 2;
    }
  if (strcmp("--fifo", argv[i]) == 0 && i+1<argc)
    {
      my_fifo_name = argv[i+1];
      i += 2;
      return 2;
    }
  /* For arguments requiring a following option, increment i by 2 and return 2;
     For other arguments to be handled by FLTK, return 0 */
  return 0;
} // end miniedit_prog_arg_handler

static void my_quitmenu_handler(Fl_Widget *w, void *);
static void my_exitmenu_handler(Fl_Widget *w, void *);
static void my_dumpmenu_handler(Fl_Widget *w, void *);

static void
my_quitmenu_handler(Fl_Widget *w, void *ad)
{
  DBGPRINTF("my_quitmenu_handler w@%p ad:%p", w, ad);
  MY_BACKTRACE_PRINT(1);
  exit(EXIT_SUCCESS);
} // end my_quitmenu_handler

static void
my_exitmenu_handler(Fl_Widget *w, void *ad)
{
  DBGPRINTF("my_exitmenu_handler w@%p ad:%p", w, ad);
#warning unimplemented my_exitmenu_handler
  FATALPRINTF("my_exitmenu_handler unimplemented ad@%p", ad);
} // end my_quitmenu_handler

static void
my_dumpmenu_handler(Fl_Widget *w, void *ad)
{
  DBGPRINTF("my_dumpmenu_handler w@%p ad:%p", w, ad);
#warning unimplemented my_dumpmenu_handler
  FATALPRINTF("my_dumpmenu_handler unimplemented ad@%p", ad);
} // end my_quitmenu_handler

char my_host_name[80];

void my_expand_env(void)
{
  /* See putenv(3) man page. The buffer should be static! */
  static char hostenvbuf[128];
  static char pidenvbuf[64];
  static char fifonamebuf[MY_FIFO_NAME_MAXLEN+32];
  memset (hostenvbuf, 0, sizeof(hostenvbuf));
  snprintf (hostenvbuf, sizeof(hostenvbuf), "FLTKMINIEDIT_HOST=%s", my_host_name);
  if (putenv(hostenvbuf))
    FATALPRINTF("failed to putenv FLTKMINIEDIT_HOST=%s : %m", my_host_name);
  DBGPRINTF("my_expand_env %s", hostenvbuf);
  memset (pidenvbuf, 0, sizeof(pidenvbuf));
  snprintf (pidenvbuf, sizeof(pidenvbuf), "FLTKMINIEDIT_PID=%d", (int)getpid());
  if (putenv(pidenvbuf))
    FATALPRINTF("failed to putenv FLTKMINIEDIT_HOST=%d : %m", (int)getpid());
  DBGPRINTF("my_expand_env %s", pidenvbuf);
  if (my_fifo_name)
    {
      if (strlen(my_fifo_name) >= MY_FIFO_NAME_MAXLEN)
        FATALPRINTF("too long FIFO name %s", my_fifo_name);
      snprintf(fifonamebuf, sizeof(fifonamebuf), "FLTKMINIEDIT_FIFO=%s", my_fifo_name);
      if (putenv(fifonamebuf))
        FATALPRINTF("failed to putenv FLTKMINIEDIT_FIFO=%s : %m", my_fifo_name);
      DBGPRINTF("my_expand_env %s", fifonamebuf);
    }
} // end my_expand_env


void
do_show_usage(FILE*fil, const char*progname)
{
  fprintf(fil, "usage: %s [options...]\n"
          " -h | --help        : print extended help message\n"
          " -D | --debug       : show debugging messages\n"
          " --fifo <fifoname>  : accept JSONRPC on <fifoname>.cmd and output JSONRPC on <fifoname>.out\n"
          " --do <shellcmd>    : run a shell command\n"
          " -Y | --style-demo  : show demo of styles\n"
          " --xtrafont <fontname>\n"
          " --otherfont <fontname>\n"
          " -V | --version     : print version\n",
          progname);
  fflush(fil);
} // end do_show_usage

int
main(int argc, char **argv)
{
  int i= 1;
  my_prog_name = argv[0];
  if (Fl::args(argc, argv, i, miniedit_prog_arg_handler) < argc)
    {
      do_show_usage(stderr, my_prog_name);
      Fl::fatal("error: unknown option: %s\n",
                argv[i]);
    }
  if (my_help_flag)
    {
      do_show_usage(stdout, my_prog_name);
      exit(EXIT_SUCCESS);
    };
  if (my_version_flag)
    {
      std::cout << argv[0] << " version "
                << GITID
                << " built " << __DATE__ "@" __TIME__ << " with " <<  std::endl;
      std::cout << "\t GNU libc release " << gnu_get_libc_release() << " version " << gnu_get_libc_version() << std::endl;
      std::cout << "\t FLTK version:" << Fl::version() << " API " << Fl::api_version() << std::endl;
      std::cout << "\t JSONCPP version:" << JSONCPP_VERSION_STRING << std::endl;
      exit(EXIT_SUCCESS);
    }
  std::string tistr = __FILE__;
  tistr.erase(sizeof(__FILE__)-4,3);
  ////
#ifdef GITID
  tistr += "/" GITID;
#endif		       // GITID defined
  ////
  my_backtrace_state = //
    backtrace_create_state
    (
      "fltk-mini-edit", /*threaded:*/0,
      my_backtrace_error, nullptr);
  if (gethostname(my_host_name, sizeof(my_host_name)-2))
    FATALPRINTF("failed to get hostname %s", strerror(errno));
  if (my_fifo_name)
    my_initialize_fifo();
  my_expand_env();
  if (my_shell_command)
    {
      printf("%s runs command %s\n", my_prog_name, my_shell_command);
      fflush(nullptr);
      int errcod = system(my_shell_command);
      if (errcod)
        FATALPRINTF("shell command %s failed #%d", my_shell_command, errcod);
    };
  Fl::set_font(MYFL_FREEMONO_FONT, "FreeMono");
  Fl::set_font(MYFL_FREEMONO_BOLD_FONT, "FreeMono bold");
  Fl::set_font(MYFL_GOMONO_FONT, "Go Mono");
  Fl::set_font(MYFL_DEJAVUSANSMONO_FONT, "DejaVu Sans Mono");
  if (my_xtrafont_name)
    Fl::set_font(MYFL_XTRAFONT, my_xtrafont_name);
  if (my_otherfont_name)
    Fl::set_font(MYFL_OTHERFONT, my_xtrafont_name);
  /// perhaps should not create windows with --help ?
  {
    Fl_Window *win = new Fl_Window(720, 480, tistr.c_str());
    Fl_Menu_Bar* menub = new Fl_Menu_Bar(1, 1, win->w()-5, 17);
    menub->add("&App/&Quit", "^q", my_quitmenu_handler);
    menub->add("&App/&Exit", "^x", my_exitmenu_handler);
    menub->add("&App/&Dump", "^d", my_dumpmenu_handler);
    MyEditor  *med = new MyEditor(3,19,win->w()-8,win->h()-22);
    if (!my_fifo_name)
      {
        if (my_styledemo_flag)
          do_style_demo (med);
        else
          med->text("Test\n"
                    "Other\n"
                    "0123456789\n"
                    "°§ +\n");
      }
    win->resizable(med);
    win->end();
    win->show();
    my_top_window = win;
    my_menubar = menub;
  }
  if (my_debug_flag)
    {
      DBGPRINTF("my_top_window@%p my_menubar@%p", my_top_window, my_menubar);
      DBGPRINTF("MyEditor Style_Unicode color#%d font#%d size %d",
                MyEditor::style_table[MyEditor::Style_Unicode].color,
                MyEditor::style_table[MyEditor::Style_Unicode].font,
                MyEditor::style_table[MyEditor::Style_Unicode].size);
      DBGPRINTF("MyEditor Style_Unicode fontname %s",
                Fl::get_font(MyEditor::style_table[MyEditor::Style_Unicode].font));
    }
  int runerr = Fl::run();
  my_menubar = nullptr;
  delete my_top_window;
  return runerr;
}  // end main



struct backtrace_state *my_backtrace_state= nullptr;

const char*my_prog_name = nullptr;
const char* my_fifo_name = nullptr;
int fifo_cmd_fd = -1;
int fifo_out_fd = -1;
Fl_Window *my_top_window= nullptr;
Fl_Menu_Bar*my_menubar = nullptr;
bool my_help_flag = false;
bool my_version_flag = false;
bool my_debug_flag = false;
bool my_styledemo_flag = false;
const int my_font_delta = MY_FONT_DELTA;
/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./mini-edit-build.sh" ;;
 ** End: ;;
 ****************/
