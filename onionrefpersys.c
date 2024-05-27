
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
#include <assert.h>
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
#include <sys/types.h>
#include <sys/resource.h>
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
sqlite3 *my_sqlite_db;
int my_cputime_limit = 7200 /* seconds, so two hours */ ;
int my_filesize_limit_mb = 1024 /* megabytes, so one gigabyte */ ;
int my_web_timeout = 10;	/* web timeout in seconds */


////////////////////////////////////////////////////////////////
/****
 *    web requests to be handled by onionrefpersys
 *
 *
 ***** POST /refpersys-build (sent from RefPerSys GNUmakefile
 * request arguments
 *        gitid from REFPERSYS_CONFIGURED_GITID
 *        gcc_version
 *        gpp_version
 *        fltk_version (from fltk-config --version)
 *        builder_person
 *        builder_email
 *
 * request response ???not sure???
 *        type JSON
 *        fields dbid
*****/
////////////////////////////////////////////////////////////////




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
  {.name = (const char *) "timeout",	//
   .has_arg = required_argument,	//
   .flag = (int *) NULL,	//
   .val = 'T'},			//
  {.name = (const char *) "cpu-limit",	//
   .has_arg = required_argument,	//
   .flag = (int *) NULL,	//
   .val = 'C'},			//
  {.name = (const char *) "file-limit",	//
   .has_arg = required_argument,	//
   .flag = (int *) NULL,	//
   .val = 'F'},			//
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
      c =
	getopt_long (argc, argv, "DP:H:B:C:F:T:hV", options_arr,
		     &option_index);
      if (c < 0)
	break;
      switch (c)
	{
	case 'D':		// --debug
	  debug = true;
	  break;
	case 'P':		// --port <webport>
	  web_port = optarg;
	  break;
	case 'h':		// --help
	  show_usage ();
	  break;
	case 'V':		// --version
	  printf ("%s version git %s built %s\n",
		  progname, gitid, __DATE__ "@" __TIME__);
	  printf ("sqlite version %s sourceid %s\n", sqlite3_libversion (),
		  sqlite3_sourceid ());
	  printf ("onion version %s\n", onion_version ());
	  exit (EXIT_SUCCESS);
	  break;
	case 'H':		// --host <webhost>
	  web_host = optarg;
	  break;
	case 'B':		// --sqlite-base <filepath>
	  my_sqlite_path = optarg;
	  break;
	case 'C':		// --cpu-limit <cpuseconds>
	  my_cputime_limit = atoi (optarg);
	  if (my_cputime_limit < 0)
	    FATAL ("invalid CPU time limit %d", my_cputime_limit);
	  break;
	case 'F':
	  my_filesize_limit_mb = atoi (optarg);
	  if (my_filesize_limit_mb < 0)
	    FATAL ("invalid file size limit %d", my_filesize_limit_mb);
	  break;
	case 'T':
	  my_web_timeout = atoi (optarg);
	  if (my_web_timeout < 0)
	    FATAL ("invalid web timeout limit %d", my_filesize_limit_mb);
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
  printf
    ("\t --cpu-limit | -C <cpusec>    # set CPU time limit, default %d sec,\n"
     "\t                              # 0 for none.\n", my_cputime_limit);
  printf
    ("\t --timeout | -T <realsec>    # set web timeout limit, default %d sec,\n"
     "\t                              # 0 for none.\n", my_web_timeout);
  printf
    ("\t --file-limit | -F <sizemb>   # set file size limit, default %d megabytes\n"
     "\t                              # 0 for none.\n", my_filesize_limit_mb);
}				/* end show_usage */

void
set_process_limits (void)
{
  struct rlimit rlim;
  memset (&rlim, 0, sizeof (rlim));
  if (my_cputime_limit > 0)
    {
      rlim.rlim_cur = my_cputime_limit;
      rlim.rlim_max = my_cputime_limit + 2;
      if (setrlimit (RLIMIT_CPU, &rlim))
	FATAL ("failed to limit CPU time to %d sec. (%s)", my_cputime_limit,
	       strerror (errno));
    };
  memset (&rlim, 0, sizeof (rlim));
  if (my_filesize_limit_mb > 0)
    {
      rlim.rlim_cur = my_filesize_limit_mb << 20;
      rlim.rlim_max = (my_filesize_limit_mb << 20) + 65536;
      if (setrlimit (RLIMIT_FSIZE, &rlim))
	FATAL ("failed to limit file size to %d megabytes (%s)",
	       my_cputime_limit, strerror (errno));
    };
}				/* end set_process_limits */


void
create_sqlite_tables (void)
{
  char *msgerr = NULL;
  assert (my_sqlite_db != NULL);
  int r1 = sqlite3_exec (my_sqlite_db,
			 "PRAGMA encoding = 'UTF-8';\n"
			 "BEGIN TRANSACTION;\n"
			 "CREATE TABLE IF NOT EXISTS tb_ (" ")",
			 NULL,
			 NULL,
			 &msgerr);
  if (r1 != SQLITE_OK)
    FATAL ("failed to create tb_ (r1:%d:%s)- error %s", r1,
	   sqlite3_errstr (r1), msgerr ? msgerr : "???");
  msgerr = NULL;
  int rend = sqlite3_exec (my_sqlite_db,
			   "END TRANSACTION;\n",
			   NULL,
			   NULL,
			   &msgerr);
  if (rend != SQLITE_OK)
    FATAL ("failed to END transaction (rend:%d:%s) - error %s", rend,
	   sqlite3_errstr (rend), msgerr ? msgerr : "???");
}				/* end create_sqlite_tables */

int
main (int argc, char **argv)
{
  progname = argv[0];
  openlog (progname, LOG_NDELAY | LOG_CONS | LOG_PERROR | LOG_PID,
	   LOG_LOCAL0);
  parse_options (argc, argv);
  nice (5);
  set_process_limits ();
  my_onion = onion_new (O_THREADED);
  if (my_web_timeout > 0)
    onion_set_timeout (my_onion, my_web_timeout);
  errno = 0;
  {
    int sqin = sqlite3_initialize ();
    if (sqin != SQLITE_OK)
      FATAL ("Failed to initialize sqlite3: sqlite error %d (errno %s)", sqin,
	     strerror (errno));
    int sqop = sqlite3_open_v2 (my_sqlite_path, &my_sqlite_db,
				SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE |
				SQLITE_OPEN_FULLMUTEX,
				NULL);
    if (sqop != SQLITE_OK || !my_sqlite_db)
      FATAL ("Failed to open sqlite database %s, sqlite error %d (errno %s)",
	     my_sqlite_path, sqop, strerror (errno));
  }
  onion_set_port (my_onion, web_port);
  onion_set_hostname (my_onion, web_host);
  {
    int sqcl = sqlite3_close_v2 (my_sqlite_db);
    if (sqcl != 0)
      FATAL ("Failed to close sqlite database %s, sqlite error %d (errno %s)",
	     my_sqlite_path, sqcl, strerror (errno));
  }
  create_sqlite_tables ();
}				/* end main */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make onionrefpersys" ;;
 ** End: ;;
 ****************/

/// end of
