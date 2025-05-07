/// file minrefpersys.h
// SPDX-License-Identifier: GPL-3.0-or-later
/// © Copyright 2025 Basile Starynkevitch
#ifndef MINREFPERSYS_INCLUDED
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#include <gtk/gtk.h>
#include <backtrace.h>
#include <lightning.h>
#include <sqlite3.h>

extern sqlite3 *mrps_sqlite;
extern const char *mrps_progname;

#endif /*MINREFPERSYS_INCLUDED */
