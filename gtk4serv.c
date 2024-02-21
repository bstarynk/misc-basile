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
#include <gdk/gdk.h>
/// the gtk/gtk.h includes every header from GTK4
#include <gtk/gtk.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*** Food for thought (Feb 14, 2024):
 *
 * A possible approach could be that in some cases, the gtk4serv
 * executable generates a temporary plugin as C code whose first
 * lines are copied up to the sentinel line ///@@@ below and compile
 * then dlopens that temporary plugin.
 ***/
#define UNUSED __attribute__((unused))
extern char *my_prog_name;
extern const char my_git_id[];
extern char my_host_name[64];
extern char my_jsonrpc_prefix[128];
#define MY_FIFO_LEN  (sizeof(my_jsonrpc_prefix)+16)
extern gboolean my_debug_wanted;
extern GtkApplication *my_app;
extern GtkWidget *my_main_window;
extern int my_fifo_cmd_wfd;	/// fifo file descriptor, commands written by gtk4serv
extern int my_fifo_out_rfd;	/// fifo file descriptor, outputs from refpersys read by gtk4serv
extern GIOChannel *my_fifo_cmd_wchan;	/* channel to write JSONRPC commands to refpersys */
extern GIOChannel *my_fifo_out_rchan;	/* channel to read JSONRPC outputs from refpersys */
extern int my_fifo_cmd_watchid;	/// watcher id for JSONRPC commands to refpersys
extern int my_fifo_out_watchid;	/// watcher id for JSONRPC outputs from refpersys
extern GtkBuilder *my_builder;


#define DBGEPRINTF_AT(Fil,Lin,Fmt,...) do {                     \
    if (my_debug_wanted) {					\
      fprintf(stderr, "@¤%s:%d: ", (Fil), (Lin));               \
      fprintf(stderr, Fmt, ##__VA_ARGS__);                      \
      fputc('\n', stderr); fflush(stderr); }} while(false)

#define DBGEPRINTF(Fmt,...) DBGEPRINTF_AT(__FILE__,__LINE__,\
  (Fmt), ##__VA_ARGS__)


#define MY_FATAL_AT(Fil,Lin,Fmt,...) do {	\
    char wbuf[64];				\
    snprintf(wbuf, sizeof(wbuf), "%s:%d",	\
	     (Fil), (Lin));			\
    g_error("%s: " # Fmt, wbuf, ##__VA_ARGS__);	\
    abort(); } while(false)

#define MY_FATAL(Fmt,...) MY_FATAL_AT(__FILE__,__LINE__,\
  (Fmt), ##__VA_ARGS__)




//////@@@@@@@@@@
// the above line is a sentinel, see Food for thought comment above.

static int
my_fifo_cmd_writer_cb (GIOChannel *src, GIOCondition cond,
		       gpointer data UNUSED)
{
  DBGEPRINTF ("%s: my_fifo_cmd_writer_cb start src@%p", my_prog_name, src);
  g_assert (cond == G_IO_OUT);
  g_assert (my_fifo_cmd_wfd > 0);
#warning unimplemented my_fifo_cmd_writer_cb
  MY_FATAL ("%s unimplemented my_fifo_cmd_writer_cb for fd#%d",
	    my_prog_name, my_fifo_cmd_wfd);
  /// The function should return FALSE if the event source should be removed.
}				/* end of my_fifo_cmd_writer_cb */

static int
my_fifo_out_reader_cb (GIOChannel *src, GIOCondition cond,
		       gpointer data UNUSED)
{
  DBGEPRINTF ("%s: my_fifo_out_reader_cb start src@%p", my_prog_name, src);
  g_assert (cond == G_IO_IN);
  g_assert (my_fifo_out_rfd > 0);
#warning unimplemented my_fifo_out_reader_cb
  MY_FATAL ("%s unimplemented my_fifo_out_reader_cb for fd#%d",
	    my_prog_name, my_fifo_out_rfd);
  /// The function should return FALSE if the event source should be removed.
}				/* end of my_fifo_out_reader_cb */

static void
my_activate_app (GApplication *app)
{
  DBGEPRINTF ("my_activate app%p my_fifo_cmd_wfd=%d my_fifo_out_rfd=%d",
	      app, my_fifo_cmd_wfd, my_fifo_out_rfd);
  /// should register the fifo file descriptors for polling them
  if (my_fifo_cmd_wfd > 0)
    {
      my_fifo_cmd_wchan = g_io_channel_unix_new (my_fifo_cmd_wfd);
      DBGEPRINTF ("my_activate my_fifo_cmdw_chan@%p", my_fifo_cmd_wchan);
      if (!my_fifo_cmd_wchan)
	{
	  g_error
	    ("failed to get JSONRPC writable command channel for fd#%d",
	     my_fifo_cmd_wfd);
	  abort ();
	}
      GError *err = NULL;
      if (g_io_channel_set_encoding (my_fifo_cmd_wchan, "UTF-8", &err)
	  != G_IO_STATUS_NORMAL)
	{
	  g_error
	    ("failed to set UTF-8 encoding on JSONRPC fifo command wfd#%d : %s",
	     my_fifo_cmd_wfd, err ? err->message : "???");
	  abort ();
	};
      my_fifo_cmd_watchid =	//
	g_io_add_watch (my_fifo_cmd_wchan, G_IO_OUT, my_fifo_cmd_writer_cb,
			NULL);
    };
  if (my_fifo_out_rfd > 0)
    {
      my_fifo_out_rchan = g_io_channel_unix_new (my_fifo_out_rfd);
      DBGEPRINTF ("my_activate my_fifo_out_rchan@%p", my_fifo_out_rchan);
      if (!my_fifo_out_rchan)
	{
	  g_error
	    ("failed to get JSONRPC readable output channel for fd#%d",
	     my_fifo_out_rfd);
	  abort ();
	}
      GError *err = NULL;
      if (g_io_channel_set_encoding (my_fifo_out_rchan, "UTF-8", &err)
	  != G_IO_STATUS_NORMAL)
	{
	  g_error
	    ("failed to set UTF-8 encoding on JSONRPC fifo output rfd#%d : %s",
	     my_fifo_out_rfd, err ? err->message : "???");
	  abort ();
	};
      my_fifo_out_watchid =	//
	g_io_add_watch (my_fifo_out_rchan, G_IO_IN, my_fifo_out_reader_cb,
			NULL);
    };
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

static void
my_fifo_creation (const char *jsonrpc, char cmdjrbuf[static MY_FIFO_LEN],
		  char outjrbuf[static MY_FIFO_LEN])
{
  g_assert (jsonrpc);
  g_assert (cmdjrbuf);
  g_assert (outjrbuf);
  memset (cmdjrbuf, 0, MY_FIFO_LEN);
  memset (outjrbuf, 0, MY_FIFO_LEN);
  DBGEPRINTF ("%s: my_fifo_creation jsonrpc %s", my_prog_name,
	      my_jsonrpc_prefix);
  /// open or create the $JSONRPC.cmd fifo for refpersys commands,
  /// ... it will be written by gtk4serv
  snprintf (cmdjrbuf, MY_FIFO_LEN, "%s.cmd", my_jsonrpc_prefix);
  errno = 0;
  DBGEPRINTF ("%s: my_fifo_creation cmdfifo %s", my_prog_name, cmdjrbuf);
  if (access (cmdjrbuf, F_OK | W_OK))
    {
      DBGEPRINTF ("%s: my_fifo_creation cmdfifo %s inaccessible %m",
		  my_prog_name, cmdjrbuf);
      if (mkfifo (cmdjrbuf, O_CLOEXEC | S_IRUSR | S_IWUSR))
	MY_FATAL ("%s: my_fifo_creation failed to mkfifo command %s (%s)",
		  my_prog_name, cmdjrbuf, strerror (errno));
      else
	DBGEPRINTF ("%s: my_fifo_creation made cmdfifo %s (writable)",
		    my_prog_name, cmdjrbuf);
    }
  else
    DBGEPRINTF ("%s: existing cmdfifo writable %s", my_prog_name, cmdjrbuf);
  memset (outjrbuf, 0, MY_FIFO_LEN);
  /// open or create the $JSONRPC.out fifo for refpersys outputs,
  /// ... it will be read by gtk4serv
  snprintf (outjrbuf, MY_FIFO_LEN, "%s.out", my_jsonrpc_prefix);
  errno = 0;
  DBGEPRINTF ("%s: my_fifo_creation outfifo %s", my_prog_name, outjrbuf);
  if (access (outjrbuf, F_OK | R_OK))
    {
      DBGEPRINTF ("%s: my_fifo_creation outfifo %s inaccessible %m",
		  my_prog_name, outjrbuf);
      if (mkfifo (outjrbuf, O_CLOEXEC | S_IRUSR | S_IWUSR))
	MY_FATAL ("%s: my_fifo_creation failed to mkfifo output %s (%s)",
		  my_prog_name, outjrbuf, strerror (errno));
      else
	DBGEPRINTF ("%s: my_fifo_creation made outfifo %s readable",
		    my_prog_name, outjrbuf);
    }
  else
    DBGEPRINTF ("%s:  my_fifo_creation outfifo %s exists readable",
		my_prog_name, outjrbuf);
}				/* end my_fifo_creation */

static void
my_create_widgets_without_builder (void)
{
  my_main_window = gtk_window_new ();
  DBGEPRINTF ("my_create_widgets_without_builder mainwin@%p", my_main_window);
  /**
   * TODO: google for gtk_main_quit replacement in gtk4
   **/
  /// g_signal_connect (G_OBJECT (my_main_window),
  ///               "destroy", G_CALLBACK (gtk_main_quit), NULL);
#if 0
  gtk_container_set_border_width (GTK_CONTAINER (my_main_window), 10);
  gtk_window_set_default_size (GTK_WINDOW (my_main_window), 760, 500);
  gtk_window_set_position (GTK_WINDOW (my_main_window), GTK_WIN_POS_CENTER);
#endif
#warning my_create_widgets_without_builder incomplete
  gtk_widget_show (my_main_window);
  DBGEPRINTF ("my_create_widgets_without_builder end mainwin@%p",
	      my_main_window);
}				/* end my_create_widgets_without_builder */

static int
my_local_options (GApplication *app, GVariantDict *options,
		  gpointer data UNUSED)
{
  gboolean version = FALSE;
  gboolean builder_given = FALSE;
  DBGEPRINTF ("%s: my_local_options start app@%p", my_prog_name, app);
  g_variant_dict_lookup (options, "version", "b", &version);

  if (version)
    {
      my_print_version ();
      return 0;
    }

  char *builderpath = NULL;
  DBGEPRINTF ("%s: my_local_options testing for builder", my_prog_name);
  if (g_variant_dict_lookup (options, "builder", "&s", &builderpath))
    {
      DBGEPRINTF ("%s: my_local_options builder %s", my_prog_name,
		  builderpath);
      my_builder = gtk_builder_new_from_file (builderpath);
      builder_given = TRUE;
      g_free (builderpath);
    }
  else
    {
      builder_given = FALSE;
      DBGEPRINTF ("%s: my_local_options without builder", my_prog_name);
    };
  char *jsonrpc = NULL;
  if (g_variant_dict_lookup (options, "jsonrpc", "s", &jsonrpc))
    {
      int fd = -1;
      char cmdjrbuf[MY_FIFO_LEN];
      char outjrbuf[MY_FIFO_LEN];
      memset (cmdjrbuf, 0, sizeof (cmdjrbuf));
      memset (outjrbuf, 0, sizeof (outjrbuf));
      DBGEPRINTF ("%s: my_local_options jsonrpc %s", my_prog_name, jsonrpc);
      strncpy (my_jsonrpc_prefix, jsonrpc, sizeof (my_jsonrpc_prefix) - 1);
      my_fifo_creation (my_jsonrpc_prefix, cmdjrbuf, outjrbuf);
      DBGEPRINTF
	("%s: my_local_options jsonprefix %s created cmdjrbuf %s, outjrbuf %s",
	 my_prog_name, my_jsonrpc_prefix, cmdjrbuf, outjrbuf);
      DBGEPRINTF
	("%s: my_local_options opening cmdjrbuf %s O_CLOEXEC O_WRONLY O_NONBLOCK",
	 my_prog_name, cmdjrbuf);
      fd = open (cmdjrbuf, O_CLOEXEC | O_WRONLY | O_NONBLOCK);
      if (fd < 0)
	{
	  MY_FATAL
	    ("%s: failed to open JSONRPC command fifo %s written to refpersys (%s)",
	     my_prog_name, cmdjrbuf, strerror (errno));
	  return -1;		// not reached
	};
      my_fifo_cmd_wfd = fd;
      DBGEPRINTF ("my_local_options my_fifo_cmd_wfd=%d %s",
		  my_fifo_cmd_wfd, cmdjrbuf);
      ////////////////
      DBGEPRINTF
	("%s: my_local_options opening outjrbuf %s O_CLOEXEC O_RDONLY O_NONBLOCK",
	 my_prog_name, outjrbuf);
      fd = open (outjrbuf, O_CLOEXEC | O_RDONLY | O_NONBLOCK);
      if (fd < 0)
	{
	  MY_FATAL ("failed to open JSONRPC output fifo %s read by gtk4serv"
		    " from refpersys (%s)", outjrbuf, strerror (errno));
	  return -1;		// not reached
	};
      my_fifo_out_rfd = fd;
      DBGEPRINTF ("my_local_options my_fifo_out_rfd=%d %s", my_fifo_out_rfd,
		  outjrbuf);
      return 0;
    }
  if (!builder_given)
    {
      DBGEPRINTF ("%s: my_local_options create widgets without builder",
		  my_prog_name);
      my_create_widgets_without_builder ();
    };
  DBGEPRINTF ("%s: my_local_options end", my_prog_name);
  return 0;
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
  if (my_debug_wanted)
    {
      fprintf (stderr, "@@%s:%d: running", __FILE__, __LINE__);
      for (int i = 0; i < argc; i++)
	fprintf (stderr, " %s", argv[i]);
      fputc ('\n', stderr);
      fflush (NULL);
    };
  my_app = gtk_application_new ("org.refpersys.gtk4serv",
				G_APPLICATION_DEFAULT_FLAGS);
  DBGEPRINTF ("%s: my_app@%p", my_prog_name, my_app);
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
     /*long_name: */ "builder",
     /*short_name: */ (char) 'B',
     /*flag: */ G_OPTION_FLAG_FILENAME,
     /*arg: */ G_OPTION_ARG_FILENAME,
     /*description: */ "use the given $BUILDERFILE for GtkBuilder",
     /*arg_description: */ "BUILDERFILE");
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
  DBGEPRINTF ("before g_application_run with app@%p git %s", my_app,
	      my_git_id);
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
int my_fifo_cmd_wfd = -1;	/// fifo file descriptor, commands written by gtk4serv
int my_fifo_out_rfd = -1;	/// fifo file descriptor, outputs from refpersys read by gtk4serv
GIOChannel *my_fifo_cmd_wchan;	/* channel to write JSONRPC commands to */
GIOChannel *my_fifo_out_rchan;	/* channel to write JSONRPC commands to */
int my_fifo_cmd_watchid;	/// watcher id for JSONRPC commands to refpersys
int my_fifo_out_watchid;	/// watcher id for JSONRPC outputs from refpersys
GtkBuilder *my_builder;
GtkWidget *my_main_window;

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make gtk4serv" ;;
 ** End: ;;
 ****************/

/// end of file gtk4serv.c
