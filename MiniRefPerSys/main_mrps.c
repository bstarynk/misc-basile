// file main_mrps.c
//  SPDX-License-Identifier: GPL-3.0-or-later
// 
// © Copyright (C) 2025 Basile Starynkevitch
// under GNU Public License v3+

extern const char mrps_main_date[];
const char mrps_main_date[] = __DATE__;

extern const char mrps_main_shortgitid[];
const char mrps_main_shortgitid[] = MRPS_SHORTGITID;

#include "minrefpersys.h"

sqlite3 *mrps_sqlite;
const char *mrps_progname;
lua_State*mrps_luastate;

static void
activate (GtkApplication *app, gpointer user_data)
{
  GtkWidget *window;

  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "minirefpersys");
  gtk_window_set_default_size (GTK_WINDOW (window), 500, 400);
  gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char **argv)
{
  GtkApplication *app = NULL;
  int status = 0;
  mrps_progname = argv[0];
  init_jit (argv[0]);
  mrps_luastate = luaL_newstate();
  luaL_openlibs(mrps_luastate);
  if (SQLITE_OK != sqlite3_open (MRPS_SQLITEDB, &mrps_sqlite))
    {
      fprintf (stderr, "%s failed to sqlite3_open %s : %s\n",
	       mrps_progname, MRPS_SQLITEDB,
	       sqlite3_errmsg (mrps_sqlite) ? : strerror (errno));
      exit (EXIT_FAILURE);
    };
  app = gtk_application_new ("org.refpersys.minirefpers",
			     G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  sqlite3_close (mrps_sqlite);
  return status;
}


/// end of file main_mrps.c
