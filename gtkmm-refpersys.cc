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

#include <gtkmm.h>
#include <vector>
#include <cstddef>
#include <cstring>
#include <iostream>

#ifndef GITID
#error GITID should be compile time defined
#endif

extern "C" const char grp_git_id[];
const char grp_git_id[] = GITID;

int
main(int argc, char* argv[])
{
  auto app = Gtk::Application::create();

  return app->make_window_and_run<Gtk::Window>(argc, argv);
} // end main
