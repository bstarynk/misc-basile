// SPDX-License-Identifier: GPL-3.0-or-later
// file misc-basile/gtkmm-refpersys.cc
/***
    © Copyright 2024 Basile Starynkevitch
   program released under GNU General Public License v3+

   this is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   this is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.
****/

/// naming convention: global variables (should be few) are extern "C"
/// and their name starts with "gmrps_"


#include <gtkmm.h>
#include <giomm/application.h>
#include <vector>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <functional>

#ifndef GITID
#error GITID should be compile time defined
#endif

#define GMRPS_HOSTNAME_MAX 63
extern "C" bool gmrps_debug;
extern "C" char gmrps_hostname[GMRPS_HOSTNAME_MAX];

extern "C" const char gmrps_git_id[];

extern "C" Gtk::Application*gmrps_app;


#define GMRPS_DBGEPRINTF_AT_BIS(Fil,Lin,Fmt,...) do {	\
    if (gmrps_debug) {					\
      fflush(stderr);					\
      std::cerr << std::flush;				\
      std::clog << std::flush;				\
      fprintf(stderr, "@¤%s:%d:", (Fil), (Lin));	\
      fprintf(stderr, Fmt, ##__VA_ARGS__);		\
      fputc('\n', stderr); fflush(stderr); }}		\
  while(false)

#define GMRPS_DBGEPRINTF_AT(Fil,Lin,Fmt,...) \
  GMRPS_DBGEPRINTF_AT_BIS(Fil,Lin,Fmt,##__VA_ARGS__)

#define GMRPS_DBGEPRINTF(Fmt,...) GMRPS_DBGEPRINTF_AT(__FILE__,__LINE__,\
  (Fmt), ##__VA_ARGS__)


#define GMRPS_DBGOUT_AT_BIS(Fil,Lin,Out) do {           \
    if (gmrps_debug) {                                  \
      std::ostringstream out##Lin;			\
      out##Lin << Out << std::fflush;                   \
      GMRPS_DBGEPRINTF_AT(Fil,Lin,"%s",                 \
                          out##Lin.str().c_str());      \
    }} while(false)

#define GMRPS_DBGOUT_AT(Fil,Lin,Out) \
  GMRPS_DBGOUT_AT_BIS(Fil,Lin,Out)

#define GMRPS_DBGOUT(Out) GMRPS_DBGOUT_AT(__FILE__,__LINE__,Out)

std::ostream&operator << (std::ostream&out, std::function<void(std::ostream&)> f)
{
  f(out);
  return out;
} // end output << for lambdas...


int
main(int argc, char* argv[])
{
  if (argc>1 && (!strcmp(argv[1], "--debug") || !strcmp(argv[1], "-D")))
    gmrps_debug = true;
  gethostname(gmrps_hostname, GMRPS_HOSTNAME_MAX-1);
  GMRPS_DBGOUT("start " << argv[0] << " git " << gmrps_git_id << " on "
               << gmrps_hostname << " pid " << (int)getpid());
  auto app = Gtk::Application::create("gtkmm.refpersys.org");
  GMRPS_DBGOUT("app is " << app);
  app->add_main_option_entry(Gio::Application::OptionType::BOOL, "debug", 'D',
                             "Enable debugging");
  app->add_main_option_entry(Gio::Application::OptionType::BOOL, "version", 'V',
                             "Give version information");
  gmrps_app = app.get();
  int ret= app->make_window_and_run<Gtk::Window>(argc, argv);
  gmrps_app = nullptr;
  return ret;
} // end main

char gmrps_hostname[GMRPS_HOSTNAME_MAX];
bool gmrps_debug;
Gtk::Application*gmrps_app;
const char gmrps_git_id[] = GITID;

/// end of file gtkmm-refpersys.cc
