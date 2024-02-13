// file misc-basile/gtk4serv.c
// SPDX-License-Identifier: GPL-3.0-or-later

/***
    © Copyright 2024 by Basile Starynkevitch
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

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <sys/types.h>
#include <sys/stat.h>

#define UNUSED __attribute__((unused))
extern char *my_prog_name;
extern const char my_git_id[];
extern char my_host_name[64];
extern gboolean my_debug_wanted;
extern GtkApplication *my_app;




int
main (int argc, char *argv[])
{
  int status = 0;
  my_prog_name = basename (argv[0]);
  gethostname (my_host_name, sizeof (my_host_name));
  if (argc > 1 && (!strcmp (argv[1], "-D") || !strcmp (argv[1], "--debug")))
    my_debug_wanted = true;
  my_app = gtk_application_new ("org.refpersys.gtk4serv",
				G_APPLICATION_DEFAULT_FLAGS);
  status = g_application_run (G_APPLICATION (my_app), argc, argv);
  g_object_unref (my_app);
  return status;
}				/// end main

//// define global variables, those declared in the beginning of that file
char *my_prog_name;
char my_host_name[64];
const char my_git_id[] = GITID;
gboolean my_debug_wanted;
GtkApplication *my_app;
/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make gtk4serv" ;;
 ** End: ;;
 ****************/

/// end of file gtk4serv.c
