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
extern char my_jsonrpc_prefix[128];
extern gboolean my_debug_wanted;
extern GtkApplication *my_app;
extern int my_fifo_cmd_wfd; /// fifo file descriptor, commands written by gtk4serv
extern int my_fifo_out_rfd; /// fifo file descriptor, outputs from refpersys read by gtk4serv

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
  gchar **argv = NULL;
  gint argc = 0;
  DBGEPRINTF ("my_command_line %s: app%p", my_prog_name, app);
  argv = g_application_command_line_get_arguments (cmdline, &argc);
  DBGEPRINTF ("my_command_line argc=%d", argc);
  g_application_command_line_print (cmdline,
				    "%s: with %d arguments",
				    my_prog_name, argc);
  for (int i = 0; i < argc; i++)
    {
      g_print ("argument %d: %s\n", i, argv[i]);
      DBGEPRINTF ("my_command_line argv[%d]=%s", i, argv[i]);
    };
  g_strfreev (argv);
#warning incomplete my_command_line
  return TRUE;
}				/* end my_command_line */

static void
my_print_version (void)
{
  printf ("%s version git %s built %s\n",
	  my_prog_name, my_git_id, __DATE__ "@" __TIME__);
}				/* end my_print_version */

static int
my_local_options (GApplication *app, GVariantDict *options,
		  gpointer data UNUSED)
{
  gboolean version = FALSE;
  DBGEPRINTF ("%s: my_local_options start app@%p", my_prog_name, app);
  g_variant_dict_lookup (options, "version", "b", &version);

  if (version)
    {
      my_print_version ();
      return 0;
    }

  char *jsonrpc = NULL;
  if (g_variant_dict_lookup (options, "jsonrpc", "s", &jsonrpc))
    {
      char jrbuf[sizeof(my_jsonrpc_prefix)+16];
      memset (jrbuf, 0, sizeof(jrbuf));
      DBGEPRINTF ("%s: my_local_options jsonrpc %s", my_prog_name, jsonrpc);
      /// create the $JSONRPC.cmd fifo for commands
      strncpy(my_jsonrpc_prefix, jsonrpc, sizeof(my_jsonrpc_prefix)-1);
      snprintf (jrbuf, sizeof(jrbuf), "%s.cmd", jsonrpc);
      /// create the $JSONRPC.out fifo for outputs
      snprintf (jrbuf, sizeof(jrbuf), "%s.out", jsonrpc);
#warning my_local_options needs code to create the JSONRPC fifos
      return 0;
    }
  return -1;
}				// end of my_local_options

int
main (int argc, char *argv[])
{
  int status = 0;
  my_prog_name = basename (argv[0]);
  gethostname (my_host_name, sizeof (my_host_name));
  /// special handing of debug flag when passed first!
  if (argc > 1 && (!strcmp (argv[1], "-D") || !strcmp (argv[1], "--debug")))
    my_debug_wanted = true;
  DBGEPRINTF ("%s on %s git %s pid %d argc %d", my_prog_name, my_host_name,
	      my_git_id, (int) getpid (), argc);
  my_app = gtk_application_new ("org.refpersys.gtk4serv",
				G_APPLICATION_DEFAULT_FLAGS);
  g_application_add_main_option	//
    (G_APPLICATION (my_app),
     /*long_name: */ "version",
     /*short_name: */ (char) 0,
     /*flag: */ G_OPTION_FLAG_NONE,
     /*arg: */ G_OPTION_ARG_NONE,
     /*description: */
     "Gives version information",
     /*arg_description: */ NULL);
  g_application_add_main_option	//
    (G_APPLICATION (my_app),
     /*long_name: */ "jsonrpc",
     /*short_name: */ (char) 'J',
     /*flag: */ G_OPTION_FLAG_NONE,
     /*arg: */ G_OPTION_ARG_STRING,
     /*description: */
     "Use the given $FIFONAME.cmd & $FIFONAME.out for JsonRpc fifos\n"
     "\tThey are created if needed, and useful for refpersys.org\n"
     "\t$FIFONAME.cmd is read by refpersys, written by this gtk4serv program.\n"
     "\t$FIFONAME.out is written by refpersys and read by this gtk4serv program\n"
     "\t... see file utilities_rps.cc of RefPerSys",
     /*arg_description: */ "FIFONAME");
  g_application_add_main_option	//
    (G_APPLICATION (my_app),
     /*long_name: */ "debug",
     /*short_name: */ (char) 'D',
     /*flag: */ G_OPTION_FLAG_NONE,
     /*arg: */ G_OPTION_ARG_NONE,
     /*description: */
     "Show debugging messages",
     /*arg_description: */ NULL);
  g_signal_connect (my_app, "activate", G_CALLBACK (my_activate_app), NULL);
  g_signal_connect (my_app, "command-line",
		    G_CALLBACK (my_command_line), NULL);
  g_signal_connect (my_app, "handle-local-options",
		    G_CALLBACK (my_local_options), NULL);
  status = g_application_run (G_APPLICATION (my_app), argc, argv);
  DBGEPRINTF ("ending main with app@%p git %s", my_app, my_git_id);
  g_object_unref (my_app);
  return status;
}				/// end main

//// define global variables, those declared in the beginning of that file
char *my_prog_name;
char my_host_name[64];
char my_jsonrpc_prefix[128];
const char my_git_id[] = GITID;
gboolean my_debug_wanted;
GtkApplication *my_app;
int my_fifo_cmd_wfd= -1; /// fifo file descriptor, commands written by gtk4serv
int my_fifo_out_rfd= -1; /// fifo file descriptor, outputs from refpersys read by gtk4serv

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make gtk4serv" ;;
 ** End: ;;
 ****************/

/// end of file gtk4serv.c
