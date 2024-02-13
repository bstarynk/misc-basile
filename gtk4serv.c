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

#define DBGEPRINTF_AT(Fil,Lin,Fmt,...) do {                     \
    if (my_debug_wanted) {					\
      fprintf(stderr, "@¤%s:%d:", (Fil), (Lin));                \
      fprintf(stderr, Fmt, ##__VA_ARGS__);                      \
      fputc('\n', stderr); fflush(stderr); }} while(false)

#define DBGEPRINTF(Fmt,...) DBGEPRINTF_AT(__FILE__,__LINE__,\
  (Fmt), ##__VA_ARGS__)

static void
my_activate_app (GApplication *app)
{
  DBGEPRINTF ("my_activate app%p", app);
#warning incomplete my_activate_app
}				/* end my_activate */

static int
my_command_line (GApplication *app, GApplicationCommandLine *cmdline)
{
  DBGEPRINTF ("my_command_line app%p", app);
#warning incomplete my_command_line
  return 0;
}				/* end my_command_line */

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
  g_application_add_main_option (G_APPLICATION (my_app),
				 /*long_name: */ "version",
				 /*short_name: */ (char) 0,
				 /*flag: */ G_OPTION_FLAG_NONE,
				 /*arg: */ G_OPTION_ARG_NONE,
				 /*description: */
				 "Gives version information",
				 /*arg_description: */ NULL);
  g_application_add_main_option (G_APPLICATION (my_app),
				 /*long_name: */ "debug",
				 /*short_name: */ (char) 'D',
				 /*flag: */ G_OPTION_FLAG_NONE,
				 /*arg: */ G_OPTION_ARG_NONE,
				 /*description: */
				 "Show debugging messages",
				 /*arg_description: */ NULL);
  g_signal_connect (my_app, "activate", G_CALLBACK (my_activate_app), NULL);
  g_signal_connect (my_app, "command-line", G_CALLBACK (my_command_line),
		    NULL);
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
