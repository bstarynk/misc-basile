// fichier sync-periodically.c

/* Copyright 2020 Basile Starynkevitch
   <basile@starynkevitch.net>

This sync-periodically program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2, or (at
your option) any later version.

sync-periodically is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

--

*/
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdatomic.h>
#include <argp.h>

char *synper_progname;
char *synper_pidfile;

/// see http://man7.org/linux/man-pages/man2/sync.2.html
int synper_period;		// period for sync(2) in seconds
int synper_logperiod = 2000;	// period for syslog(3) in seconds;,
volatile atomic_bool synper_stop;
bool synper_daemonized;

#define SYNPER_MIN_PERIOD 2	/*minimal sync period, in seconds */
#define SYNPER_MAX_PERIOD 30	/*maximal sync period, in seconds */

#define SYNPER_MIN_LOGPERIOD 100	/*minimal log period, in seconds */
#define SYNPER_MAX_LOGPERIOD 7200	/*maximal log period, in seconds */


void synper_fatal_at (const char *file, int lin) __attribute__((noreturn));

#define SYNPER_FATAL_AT_BIS(Fil,Lin,Fmt,...) do {			\
    syslog(LOG_CRIT, "SYNPER FATAL:%s:%d: <%s>\n " Fmt "- %m\n\n",	\
            Fil, Lin, __func__, ##__VA_ARGS__);				\
    synper_fatal_at(Fil,Lin); } while(0)

#define SYNPER_FATAL_AT(Fil,Lin,Fmt,...)  SYNPER_FATAL_AT_BIS(Fil,Lin,Fmt,##__VA_ARGS__)

#define SYNPER_FATAL(Fmt,...) SYNPER_FATAL_AT(__FILE__,__LINE__,Fmt,##__VA_ARGS__)

#define SYNPER_STRINGIFY_BIS(Arg) #Arg
#define SYNPER_STRINGIFY(Arg) SYNPER_STRINGIFY_BIS(Arg)

struct argp_option synper_options[] = {
  {"pid-file", 'P', "FILE", 0, "write pid to file"},
  {"sync-period", 'Y', "SYNC-PERIOD", 0,
   "call sync(2) every SYNC-PERIOD seconds"},
  {"log-period", 'L', "LOG-PERIOD", 0,
   "do a syslog(3) every LOG-PERIOD seconds"},
  {"daemon", 'd', NULL, 0, "run as a daemon(3)"},
  {"version", 'V', NULL, 0, "show version info"},
  {NULL, 0, NULL, 0, NULL}
};

error_t synper_parse_opt (int key, char *arg, struct argp_state *state);

void synper_signal_handler (int signum __attribute__((unused)));
void synper_syslog_begin (void);

void
synper_signal_handler (int signum __attribute__((unused)))
{
  atomic_store (&synper_stop, true);
}				/* end synper_signal_handler */


void
synper_fatal_at (const char *file, int lin)
{
  syslog (LOG_CRIT, "SYNPER FATAL %s:%d", file, lin);
  abort ();
}				/* end synper_fatal_at */

error_t
synper_parse_opt (int key, char *arg, struct argp_state *state)
{
  if (state == NULL)
    SYNPER_FATAL ("null arpg_state key:%c=%d arg=%s", (char) key, key,
		  arg ? arg : "??");
  switch (key)
    {
    case 'd':			// --daemon
      {
	if (daemon (0, 0))
	  SYNPER_FATAL ("failed to daemon(3) : %m");
	synper_daemonized = true;
      };
      break;
    case 'P':			// --pid-file
      {
	synper_pidfile = arg;
      }
      return 0;
    case 'Y':			// --sync-period
      {
	synper_period = atoi (arg ? : "");
      }
      return 0;
    case 'L':			// --log-period
      {
	synper_logperiod = atoi (arg ? : "");
      }
      return 0;
    case 'V':			// --version
      {
#ifdef SYNPER_GITID
	printf ("%s gitid %s built on %s\n", synper_progname,
		SYNPER_STRINGIFY (SYNPER_GITID), __DATE__);
#else
	printf ("%s built on %s\n", synper_progname, __DATE__);
#endif
	printf ("\t run as: '%s --help' to get help.\n", synper_progname);
	printf ("\t see also https://github.com/bstarynk/misc-basile/\n");
	fflush (NULL);
	exit (EXIT_SUCCESS);
      }
      return -1;
    }
  return ARGP_ERR_UNKNOWN;
}				/* end synper_parse_opt */


void
synper_set_signal_handlers (void)
{
  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = synper_signal_handler;
  if (sigaction (SIGTERM, &sa, NULL))
    SYNPER_FATAL ("failed to install SIGTERM action");
  if (sigaction (SIGQUIT, &sa, NULL))
    SYNPER_FATAL ("failed to install SIGQUIT action");
}				/* end synper_set_signal_handlers */

void
synper_syslog_begin (void)
{
  time_t nowt = 0;
  time (&nowt);
  struct tm nowtm;
  memset (&nowtm, 0, sizeof (nowtm));
  if (localtime_r (&nowt, &nowtm) == NULL)
    SYNPER_FATAL ("failed to get localtime");
  char nowbuf[80];
  memset (nowbuf, 0, sizeof (nowbuf));
  if (strftime (nowbuf, sizeof (nowbuf), "%Y/%b/%d %T %Z", &nowtm) < 0)
    SYNPER_FATAL ("failed to strftime");
#ifdef SYNPER_GITID
  syslog (LOG_INFO,
	  "start of %s git %s build %s with sync period %d seconds and log period %d seconds at %s\n",
	  synper_progname, SYNPER_STRINGIFY (SYNPER_GITID), __DATE__,
	  synper_period, synper_logperiod, nowbuf);
  if (synper_daemonized)
    syslog (LOG_NOTICE, "%s git %s daemonized as pid %ld",
	    synper_progname, SYNPER_STRINGIFY (SYNPER_GITID),
	    (long) getpid ());
#else
  syslog (LOG_INFO,
	  "start of %s built %s with sync period %d seconds and log period %d seconds at %s\n",
	  synper_progname, __DATE__, synper_period, synper_logperiod, nowbuf);
  if (synper_daemonized)
    syslog (LOG_NOTICE, "%s daemonized as pid %ld", synper_progname,
	    (long) getpid ());
#endif /*SYNPER_GITID */
}				/* end synper_syslog_begin */


int
main (int argc, char **argv)
{
  synper_progname = argv[0];
  struct argp argp = { synper_options, synper_parse_opt, "",
    "Utility to call sync(2) periodically. GPLv3+ licensed.\n"
      "See www.gnu.org/licenses/ for details.\n"
      "Source " __FILE__ " on https://github.com/bstarynk/misc-basile/\n"
  };
  argp_parse (&argp, argc, argv, 0, 0, NULL);	// could run daemon(3)
  openlog ("synper", LOG_PID | LOG_NDELAY | LOG_CONS, LOG_DAEMON);
  if (synper_pidfile)
    {
      {
	char backup[256];
	memset (backup, 0, sizeof (backup));
	snprintf (backup, sizeof (backup) - 1, "%s~", synper_pidfile);
	if (strcmp (backup, synper_pidfile) && !access (synper_pidfile, F_OK))
	  (void) rename (synper_pidfile, backup);
      };
      FILE *pidfil = fopen (synper_pidfile, "w");
      if (!pidfil)
	SYNPER_FATAL ("%s failed to open pid file %s : %m",
		      synper_progname, synper_pidfile);
      fprintf (pidfil, "%ld\n", (long) getpid ());
      if (fclose (pidfil))
	SYNPER_FATAL ("%s failed to close pid file %s : %m",
		      synper_progname, synper_pidfile);
      syslog (LOG_INFO, "%s wrote pid-file %s as uid#%d", synper_progname,
	      synper_pidfile, (int) getuid ());
    }
  usleep (10 * 1024);
  if (synper_period < SYNPER_MIN_PERIOD)
    synper_period = SYNPER_MIN_PERIOD;
  if (synper_period > SYNPER_MAX_PERIOD)
    synper_period = SYNPER_MAX_PERIOD;
  if (synper_logperiod < SYNPER_MIN_LOGPERIOD)
    synper_logperiod = SYNPER_MIN_LOGPERIOD;
  if (synper_logperiod < 3 * synper_period)
    synper_logperiod = 3 * synper_period;
  if (synper_logperiod > SYNPER_MAX_LOGPERIOD)
    synper_logperiod = SYNPER_MAX_LOGPERIOD;
  synper_syslog_begin ();
  sleep (synper_period / 3);
  time_t lastlogtime = 0;
  long loopcnt = 0;
  while (true)
    {
      time_t nowt = 0;
      usleep (2000);
      sync ();
      (void) sleep (synper_period);
      if (atomic_load (&synper_stop))
	break;
      time (&nowt);
      loopcnt++;
      if (nowt <= lastlogtime)
	{
	  time_t nextlogtime = nowt + synper_logperiod;
	  char nowbuf[80];
	  memset (nowbuf, 0, sizeof (nowbuf));
	  struct tm nowtm;
	  memset (&nowtm, 0, sizeof (nowtm));
	  if (localtime_r (&nowt, &nowtm) == NULL)
	    SYNPER_FATAL ("failed to get localtime");
	  if (strftime (nowbuf, sizeof (nowbuf), "%Y/%b/%d %T %Z", &nowtm) <
	      0)
	    SYNPER_FATAL ("failed to strftime");
	  syslog (LOG_INFO,
		  "%s with sync period %d seconds done #%ld at %s\n", argv[0],
		  synper_period, loopcnt, nowbuf);
	  lastlogtime = nextlogtime;
	}
    };				// end while
  /// final syslog
  {
    time_t nowt = 0;
    time (&nowt);
    char nowbuf[80];
    memset (nowbuf, 0, sizeof (nowbuf));
    struct tm nowtm;
    memset (&nowtm, 0, sizeof (nowtm));
    if (localtime_r (&nowt, &nowtm) == NULL)
      SYNPER_FATAL ("failed to get localtime");
    if (strftime (nowbuf, sizeof (nowbuf), "%Y/%b/%d %T %Z", &nowtm) < 0)
      SYNPER_FATAL ("failed to strftime");
    syslog (LOG_NOTICE,
	    "%s with sync period %d seconds terminating loop #%ld at %s\n",
	    argv[0], synper_period, loopcnt, nowbuf);
  };
  exit (EXIT_SUCCESS);
  return 0;
}				/* end of main */


/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "gcc -v -o sync-periodically -Wall -Bstatic -O -g -DSYNPER_GITID=\"$(git log --format=oneline -q -1 | awk '{print $1}')\" sync-periodically.c" ;;
 ** End: ;;
 ****************/
