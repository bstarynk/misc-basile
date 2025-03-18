// file sync-periodically.c in github.com/bstarynk/misc-basile/

/* Copyright 2020 - 2025 Basile Starynkevitch
   <basile@starynkevitch.net>

This sync-periodically program is free software for Linux; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2, or (at your option) any later version.

The sync-periodically program is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

The purpose of sync-periodically is to regularily call the sync(2)
system call to avoid losing too much data on a Linux desktop with
64Gbytes of RAM.

See of course https://man7.org/linux/man-pages/man2/sync.2.html
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

#ifndef SYNPER_GITID
#error compilation command should define SYNPER_GITID
/*** for example in our Makefile:
   GIT_ID=$(shell git log --format=oneline -q -1 | cut -c1-10)
***/
#endif

char *synper_progname;
char *synper_pidfile;
char *synper_name;
char synper_selfexe[256];
char synper_host[128];

#define SYNPER_MIN_PERIOD 2	/*minimal sync period, in seconds */
#define SYNPER_MAX_PERIOD 30	/*maximal sync period, in seconds */

/// see http://man7.org/linux/man-pages/man2/sync.2.html
// period for sync(2) in seconds
int synper_period =
  (SYNPER_MAX_PERIOD - SYNPER_MIN_PERIOD) / 4 + SYNPER_MIN_PERIOD;
int synper_logperiod = 2000;	// period for syslog(3) in seconds;,
volatile atomic_bool synper_stop;
bool synper_daemonized;

#define SYNPER_MIN_LOGPERIOD 100	/*minimal log period, in seconds */
#define SYNPER_MAX_LOGPERIOD 7200	/*maximal log period, in seconds */


void synper_set_signal_handlers (void);

void synper_fatal_at (const char *file, int lin) __attribute__((noreturn));

#define SYNPER_FATAL_AT_BIS(Fil,Lin,Fmt,...) do {                       \
    int err##Lin = errno;                                               \
    char msgbuf##Lin[384];                                              \
    memset(msgbuf##Lin, 0, sizeof(msgbuf##Lin));                        \
    snprintf(msgbuf##Lin, sizeof(msgbuf##Lin)-1, Fmt, ##__VA_ARGS__);   \
    if (isatty(STDERR_FILENO))                                          \
      fprintf(stderr,  "SYNPER FATAL:%s:%d: <%s> %s - %s\n\n",		\
              Fil, Lin, __func__, msgbuf##Lin, strerror(err##Lin));     \
    fflush(NULL);                                                       \
    syslog(LOG_CRIT|LOG_CONS, "SYNPER FATAL:%s:%d: <%s> %s- %s\n\n",    \
           Fil, Lin, __func__, msgbuf##Lin, strerror(err##Lin));        \
    synper_fatal_at(Fil,Lin); } while(0)

#define SYNPER_FATAL_AT(Fil,Lin,Fmt,...)  SYNPER_FATAL_AT_BIS(Fil,Lin,Fmt,##__VA_ARGS__)

#define SYNPER_FATAL(Fmt,...) SYNPER_FATAL_AT(__FILE__,__LINE__,Fmt,##__VA_ARGS__)

#define SYNPER_STRINGIFY_BIS(Arg) #Arg
#define SYNPER_STRINGIFY(Arg) SYNPER_STRINGIFY_BIS(Arg)

struct argp_option synper_options[] = {
  /*name, key, arg, flag, doc, group */
  {"pid-file", 'P', "FILE", 0,
   "write pid to file, ensuring no other process is running", 0},
  {"name", 'N', "NAME", 0, "set NAME for logging", 0},
  {"sync-period", 'Y', "SYNC-PERIOD", 0,
   "call sync(2) every SYNC-PERIOD seconds", 0},
  {"log-period", 'L', "LOG-PERIOD", 0,
   "do a syslog(3) every LOG-PERIOD seconds", 0},
  {"daemon", 'd', NULL, 0, "run as a daemon(3)", 0},
  {"chdir", 'C', "NEWDIR", 0, "change directory to NEWDIR", 0},
  {"version", 'V', NULL, 0, "show version info", 0},
  {NULL, 0, NULL, 0, NULL, 0}
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
  syslog (LOG_CRIT | LOG_CONS, "SYNPER FATAL %s:%d", file, lin);
  fflush (NULL);
  exit (EXIT_FAILURE);
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
	if (daemon (0, /*no-close: */ 1))
	  {
	    int olderr = errno;
	    SYNPER_FATAL ("failed to daemon(3) pid %ld : %s",
			  (long) getpid (), strerror (olderr));
	  }
	synper_daemonized = true;
	synper_set_signal_handlers ();
      };
      return 0;
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
    case 'N':			// --name
      {
	synper_name = arg;
      }
      return 0;
    case 'L':			// --log-period
      {
	synper_logperiod = atoi (arg ? : "");
      }
      return 0;
    case 'C':
      {
	char newpath[384];
	memset (newpath, 0, sizeof (newpath));
	if (chdir (arg))
	  {
	    SYNPER_FATAL ("failed to chdir(2) to %s", arg);
	  }
	char *newdir = getcwd (newpath, sizeof (newpath) - 1);
	if (newdir)
	  {
	    if (isatty (STDOUT_FILENO))
	      printf ("%s changed directory to %s pid %d git %s\n",
		      synper_progname, newdir, (int) getpid (),
		      SYNPER_STRINGIFY (SYNPER_GITID));
	    syslog (LOG_INFO | LOG_CONS, "%s changed directory to %s pid %d",
		    synper_progname, newdir, (int) getpid ());
	    fflush (NULL);
	  }
      }
      return 0;
    case 'V':			// --version
      {
	printf ("%s gitid %s built on %s (executable %s)\n", synper_progname,
		SYNPER_STRINGIFY (SYNPER_GITID), __DATE__, synper_selfexe);
	printf ("\t run as: '%s --help' to get help.\n", synper_progname);
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
  char parentexe[128];
  memset (parentexe, 0, sizeof (parentexe));
  if (strftime (nowbuf, sizeof (nowbuf), "%Y/%b/%d %T %Z", &nowtm) <= 0)
    SYNPER_FATAL ("failed to strftime");
  pid_t parentpid = getppid ();
  if (parentpid > 0)
    {
      char parentprocx[64];
      memset (parentprocx, 0, sizeof (parentprocx));
      snprintf (parentprocx, sizeof (parentprocx), "/proc/%d/exe",
		(int) parentpid);
      if (readlink (parentprocx, parentexe, sizeof (parentexe) - 1) < 0)
	{
	  syslog (LOG_WARNING, "%s failed to readlink from %s (%s)",
		  synper_progname, parentprocx, strerror (errno));
	};
    }
  else				/// according to man page getppid(2) never fails
    SYNPER_FATAL ("failed to getppid");
  if (synper_name)
    syslog (LOG_INFO,
	    "start of %s named %s git %s build %s\n"
	    "... with sync period %d seconds"
	    " and log period %d seconds\n"
	    "... at %s, pid %d, parentpid %d running %s\n",
	    synper_progname, synper_name, SYNPER_STRINGIFY (SYNPER_GITID),
	    __DATE__, synper_period, synper_logperiod, nowbuf,
	    (int) getpid (), (int) parentpid, parentexe);
  else
    syslog (LOG_INFO,
	    "start of %s git %s build %s\n"
	    "... with sync period %d seconds"
	    " and log period %d seconds\n"
	    "... at %s, pid %d, parentpid %d running %s\n",
	    synper_progname, SYNPER_STRINGIFY (SYNPER_GITID), __DATE__,
	    synper_period, synper_logperiod, nowbuf,
	    (int) getpid (), (int) parentpid, parentexe);
  if (synper_daemonized)
    syslog (LOG_NOTICE, "%s git %s daemonized as pid %ld",
	    synper_progname, SYNPER_STRINGIFY (SYNPER_GITID),
	    (long) getpid ());
}				/* end synper_syslog_begin */


void
synper_syslog_final (void)
{
  syslog (LOG_INFO, "%s git %s ending pid %ld",
	  synper_progname, SYNPER_STRINGIFY (SYNPER_GITID), (long) getpid ());
}				/* end synper_syslog_final */

int
main (int argc, char **argv)
{
  synper_progname = argv[0];
  if (gethostname (synper_host, sizeof (synper_host)))
    {
      SYNPER_FATAL ("%s git %s failed to gethostname (%m)",
		    synper_progname, SYNPER_STRINGIFY (SYNPER_GITID));
    }
  char myselfexe[sizeof (synper_selfexe)];
  memset (myselfexe, 0, sizeof (myselfexe));
  {
    if (readlink ("/proc/self/exe", myselfexe, sizeof (myselfexe)) > 0
	&& myselfexe[sizeof (myselfexe) - 1] == (char) 0
	&& myselfexe[0] != (char) 0)
      strcpy (synper_selfexe, myselfexe);
    else
      {
	SYNPER_FATAL
	  ("%s failed to readlink /proc/self/exe git %s pid %ld on %s",
	   synper_progname, SYNPER_STRINGIFY (SYNPER_GITID), (long) getpid (),
	   synper_host);
      }
  }
  /// sleep a small amount of time to let other processes run....
  usleep (2000 + ((int) getpid ()) % 1024);
  struct argp argp = { synper_options, synper_parse_opt, "",
    "Utility to call sync(2) periodically. GPLv3+ licensed.\n"
      "See www.gnu.org/licenses/ for details.\n"
      "Source " __FILE__ " on github.com/bstarynk/misc-basile/\n"
      "Build on " __DATE__ " git " SYNPER_GITID "\n" "Program options:\n",
    /*children: */ NULL,
    /*help_filter: */ NULL,
    /*argp_domain: */ NULL
  };
  argp_parse (&argp, argc, argv, 0, 0, NULL);	// could run daemon(3)
  openlog ("synper", LOG_PID | LOG_NDELAY | LOG_CONS, LOG_DAEMON);
  synper_syslog_begin ();
  atexit (synper_syslog_final);
  time_t nowt = 0;
  char timbuf[48];
  memset (timbuf, 0, sizeof (timbuf));
  if (time (&nowt))
    {
      ctime_r (&nowt, timbuf);
    }
  else
    SYNPER_FATAL ("%s git %s failed to get current time: %m", synper_progname,
		  SYNPER_STRINGIFY (SYNPER_GITID));
  if (synper_daemonized)
    syslog (LOG_INFO,
	    "%s is daemonized as pid %ld on %s git %s executable %s on %s. See daemon(3)",
	    synper_progname, (long) getpid, synper_host,
	    SYNPER_STRINGIFY (SYNPER_GITID), synper_selfexe, timbuf);
  else
    syslog (LOG_INFO,
	    "%s is started as pid %ld on %s git %s executable %s on %s.",
	    synper_progname, (long) getpid, synper_host,
	    SYNPER_STRINGIFY (SYNPER_GITID), synper_selfexe, timbuf);

  if (synper_name)
    syslog (LOG_INFO, "%s named %s", synper_progname, synper_name);

  if (synper_pidfile)
    {
      {
	char oldexebuf[256];
	memset (oldexebuf, 0, sizeof (oldexebuf));
	FILE *oldpidfil = fopen (synper_pidfile, "r");
	if (oldpidfil)
	  {
	    char oldexe[sizeof (synper_selfexe)];
	    long oldp = 0;
	    if (fscanf (oldpidfil, " %ld", &oldp) > 0 && oldp > 0
		&& kill (oldp, 0))
	      {
		char oldprexebuf[64];
		memset (oldprexebuf, 0, sizeof (oldprexebuf));
		snprintf (oldprexebuf, sizeof (oldprexebuf),
			  "/proc/%ld/exe", oldp);
		if (readlink (oldprexebuf, oldexe, sizeof (oldexe)) > 0
		    && oldexe[sizeof (oldexe) - 1] == (char) 0
		    && oldexe[0] != (char) 0 && !strcmp (oldexe, myselfexe))
		  {
		    SYNPER_FATAL
		      ("%s (executable %s) is already running as old pid %ld"
		       " on %s (git %s) - from pid %ld", synper_progname,
		       myselfexe, oldp, synper_host,
		       SYNPER_STRINGIFY (SYNPER_GITID), (long) getpid ());
		  }
	      }
	    fclose (oldpidfil);
	  }
      }
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
    }
  synper_set_signal_handlers ();
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
  if (synper_pidfile)
    {
      syslog (LOG_INFO, "%s wrote pid-file %s as uid#%d on %s",
	      synper_progname, synper_pidfile, (int) getuid (), synper_host);
    };
  sleep (synper_period / 3);
  time_t lastlogtime = 0;
  long loopcnt = 0;
  while (!atomic_load (&synper_stop))
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
	  if (strftime (nowbuf, sizeof (nowbuf), "%Y/%b/%d %T %Z", &nowtm)
	      <= 0)
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
    if (strftime (nowbuf, sizeof (nowbuf), "%Y/%b/%d %T %Z", &nowtm) <= 0)
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
 ** compile-command: "make sync-periodically" ;;
 ** End: ;;
 ****************/
