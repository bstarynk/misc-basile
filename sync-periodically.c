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

char *synper_progname;
/// see http://man7.org/linux/man-pages/man2/sync.2.html
int synper_period;		// period for sync(2) in seconds
int synper_logperiod = 2000;	// period for syslog(3) in seconds;,
volatile atomic_bool synper_stop;

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
  syslog (LOG_INFO,
	  "start of %s with sync period %d seconds and log period %d seconds at %s\n",
	  synper_progname, synper_period, synper_logperiod, nowbuf);
}				/* end synper_syslog_begin */


int
main (int argc, char **argv)
{
  synper_progname = argv[0];
  if (argc > 1 && !strcmp (argv[1], "--help"))
    {
      fprintf (stderr, "usage: %s [sync-period [log-period]]\n"
	       "\t the sync-period is in seconds between calls to sync(2) between %d and %d\n"
	       "\t the log-period is in seconds between calls to syslog(3) between %d and %d\n"
	       "*** this utility runs sync(2) to flush disk cache buffers periodically\n"
	       "*** NO WARRANTY, since GPLv3+ licensed\n"
	       "See http://man7.org/linux/man-pages/man2/sync.2.html\n"
	       "and github.com/bstarynk/misc-basile/\n"
	       "and https://www.gnu.org/licenses/gpl-3.0.en.html\n"
	       "By <basile@starynkevitch.net>, see http://starynkevitch.net/Basile/\n",
	       argv[0],
	       SYNPER_MIN_PERIOD, SYNPER_MAX_PERIOD,
	       SYNPER_MIN_LOGPERIOD, SYNPER_MAX_LOGPERIOD);
      fflush (NULL);
      exit (EXIT_FAILURE);
    };
  openlog ("synper", LOG_PID | LOG_NDELAY | LOG_CONS, LOG_DAEMON);
  if (argc > 1)
    synper_period = atoi (argv[1]);
  if (synper_period < SYNPER_MIN_PERIOD)
    synper_period = SYNPER_MIN_PERIOD;
  if (synper_period > SYNPER_MAX_PERIOD)
    synper_period = SYNPER_MAX_PERIOD;
  if (argc > 2)
    synper_logperiod = atoi (argv[2]);
  if (synper_logperiod < SYNPER_MIN_LOGPERIOD)
    synper_logperiod = SYNPER_MIN_LOGPERIOD;
  if (synper_logperiod < 3 * synper_period)
    synper_logperiod = 3 * synper_period;
  if (synper_logperiod > SYNPER_MAX_LOGPERIOD)
    synper_logperiod = SYNPER_MAX_LOGPERIOD;
  synper_syslog_begin ();
  time_t lastlogtime = 0;
  long loopcnt = 0;
  while (true)
    {
      time_t nowt = 0;
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
 ** compile-command: "gcc -o sync-periodically -Wall -Bstatic -O -g sync-periodically.c" ;;
 ** End: ;;
 ****************/
