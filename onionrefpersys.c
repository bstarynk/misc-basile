
/**
  onionrefpersys.c:
  Copyright (C) 2023 - 2024 Basile Starynkevitch

  This onionrefpersys is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2.0 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GPL licence, if not see
  <http://www.gnu.org/licenses/>
*/


#include <features.h>
#include <getopt.h>
#include <onion/onion.h>
#include <onion/log.h>
#include <onion/version.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>
#include <stdio.h>
#include <syslog.h>
#include <gnu/libc-version.h>
#include <sqlite3.h>

onion *my_onion;

const char *progname;

#ifndef GITID
#error compilation command should define the GITID macro
#endif

const char gitid[] = GITID;

bool debug = false;

const char *web_port = "8080";
const char *web_host = "localhost";
const char *my_sqlite_path = "/var/tmp/onionrefpersys.sqlite";
sqlite3*my_sqlite_db;

const struct option options_arr[] = {
  {.name = (const char *) "debug",	//
   .has_arg = no_argument,	//
   .flag = (int *) NULL,	//
   .val = 'D'},			//
  {.name = (const char *) "help",	//
   .has_arg = no_argument,	//
   .flag = (int *) NULL,	//
   .val = 'h'},			//
  {.name = (const char *) "version",	//
   .has_arg = no_argument,	//
   .flag = (int *) NULL,	//
   .val = 'V'},			//
  {.name = (const char *) "port",	//
   .has_arg = required_argument,	//
   .flag = (int *) NULL,	//
   .val = 'P'},			//
  {.name = (const char *) "host",	//
   .has_arg = required_argument,	//
   .flag = (int *) NULL,	//
   .val = 'H'},			//
  {.name = (const char *) "sqlite-base",	//
   .has_arg = required_argument,	//
   .flag = (int *) NULL,	//
   .val = 'B'},			//
  {.name = (const char *) NULL,	//
   .has_arg = no_argument,	//
   .flag = (int *) NULL,	//
   .val = 0}			//
};

void parse_options (int argc, char **argv);

void show_usage (void);

void fatal_at (const char *fil, int lin, const char *fmt, ...)
  __attribute__((noreturn, format (printf, 3, 4)));

#define FATAL_AT_BIS(Fil,Lin,Fmt,...) do{fatal_at(Fil,Lin,Fmt,##__VA_ARGS__);}while(0)
#define FATAL_AT(Fil,Lin,Fmt,...) FATAL_AT_BIS((Fil),(Lin),Fmt,##__VA_ARGS__)
#define FATAL(Fmt,...) FATAL_AT(__FILE__,__LINE__,Fmt,##__VA_ARGS__)

////////////////////////////////////////////////////////////////

void
fatal_at (const char *fil, int lin, const char *fmt, ...)
{
  static char buf[1024];
  va_list arg;
  va_start (arg, fmt);
  vsnprintf (buf, sizeof (buf) - 1, fmt, arg);
  va_end (arg);
  syslog (LOG_CRIT, "%s fatal at %s:%d: %s", progname, fil, lin, buf);
  exit (EXIT_FAILURE);
}				/* end fatal_at */

/// keep this function consistent with show_usage
void
parse_options (int argc, char **argv)
{
  while (1)
    {
      int option_index = 0;
      int c = 0;
      c = getopt_long (argc, argv, "DP:H:B:hV", options_arr, &option_index);
      if (c < 0)
	break;
      switch (c)
	{
	case 'D':		// --debug
	  debug = true;
	  break;
	case 'P':
	  web_port = optarg;
	  break;
	case 'h':
	  show_usage ();
	  break;
	case 'V':
	  printf ("%s version git %s built %s\n",
		  progname, gitid, __DATE__ "@" __TIME__);
	  printf ("sqlite version %s sourceid %s\n", sqlite3_libversion(), sqlite3_sourceid());
	  printf ("onion version %s\n", onion_version());
	  exit (EXIT_SUCCESS);
	  break;
	case 'H':
	  web_host = optarg;
	  break;
	case 'B':
	  my_sqlite_path = optarg;
	  break;
	default:
	  break;
	}
    }
}				/* end parse_options */

/// keep the show_usage consistent with parse_options
void
show_usage (void)
{
  printf ("%s usage (a specialized webserver)\n", progname);
  printf ("see %s file (GPLv3+ licensed, no warranty)\n", __FILE__);
  printf ("source file on https://github.com/bstarynk/%s/%s\n",
	  "blob/master", __FILE__);
  printf ("uses the libonion web service library from %s\n",
	  "https://www.coralbits.com/libonion/");
  printf ("uses the sqlite3 library from %s\n", "https://sqlite.org/");
  printf ("\t --debug | -D                 # enable debugging\n");
  printf ("\t --version | -V               # show version\n");
  printf ("\t --help | -h                  # show this help\n");
  printf ("\t --port | -P <webport>        # set HTTP port, default %s\n",
	  web_port);
  printf ("\t --host | -H <webhost>        # set HTTP host, default %s\n",
	  web_host);
  printf ("\t --sqlite-base | -B <path>    # set Sqlite file, default %s\n",
	  my_sqlite_path);
}				/* end show_usage */

int
main (int argc, char **argv)
{
  progname = argv[0];
  openlog (progname, LOG_NDELAY | LOG_CONS | LOG_PERROR | LOG_PID,
	   LOG_LOCAL0);
  parse_options (argc, argv);
  nice (5);
  my_onion = onion_new (O_THREADED);
  errno = 0;
  {
    int sqin = sqlite3_initialize ();
    if (sqin != SQLITE_OK)
      FATAL ("Failed to initialize sqlite3: sqlite error %d (errno %s)", sqin,
	     strerror(errno));
    int sqop = sqlite3_open_v2(my_sqlite_path, &my_sqlite_db,
			       SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE|SQLITE_OPEN_FULLMUTEX,
			       NULL);
    if (sqop != SQLITE_OK || !my_sqlite_db)
      FATAL("Failed to open sqlite database %s, sqlite error %d (errno %s)",
	    my_sqlite_path, sqop, strerror(errno));
  }
  onion_set_port (my_onion, web_port);
  onion_set_hostname (my_onion, web_host);
}				/* end main */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make onionrefpersys" ;;
 ** End: ;;
 ****************/

/// end of
