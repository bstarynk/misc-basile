/// file minrefpersys.h
// SPDX-License-Identifier: GPL-3.0-or-later
/// © Copyright (C) 2025 Basile Starynkevitch
#ifndef MINREFPERSYS_INCLUDED
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <dlfcn.h>
#include <gtk/gtk.h>
#include <backtrace.h>
#include <lightning.h>
#include <sqlite3.h>
#include "lua.h"
#include "luaconf.h"
#include "lualib.h"
#include "lauxlib.h"
extern sqlite3 *mrps_sqlite;
extern const char *mrps_progname;
extern lua_State*mrps_luastate;

#endif /*MINREFPERSYS_INCLUDED */
