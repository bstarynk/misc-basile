// file execicar.c
// SPDX-License-Identifier: GPL-3.0-or-later
// in https://github.com/bstarynk/misc-basile/

//   Copyright © 2005-2025 Basile STARYNKEVITCH
//
// This Software is licensed under the GPL licensed Version 2, 
// please read http://www.gnu.org/copyleft/gpl.html

/* you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software
 * Foundation; version 3 of the License.
 * see https://www.gnu.org/licenses/gpl-3.0.html
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <getopt.h>
#include <syslog.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


const char execicar_ident[] = __FILE__ ":" __DATE__ "@" __TIME__;

#ifndef EXECICAR_GITID
#error compilation command should define EXECICAR_GITID
#endif

const char execicar_gitid[] = EXECICAR_GITID;

static const struct option options[] = {
  {"help", no_argument, (int *) 0, 'h'},
  {"version", no_argument, (int *) 0, 'V'},
  {"quiet", no_argument, (int *) 0, 'q'},
  {"syslog", no_argument, (int *) 0, 'l'},
  {"incmd", required_argument, (int *) 0, 'i'},
  {"nice", required_argument, (int *) 0, 'n'},
  {"outrep", required_argument, (int *) 0, 'o'},
  {"shell", required_argument, (int *) 0, 's'},
  {0, 0, (int *) 0, 0}
};

clock_t startclock;

double secondpertick;
int quiet = 0;
int dolog = 0;
int fdcmd = -1;
FILE *frep;
char *shell = "/bin/sh";

int devnull = -1;

#define PROCNAMELEN 48
#define PROCNAMEFMT "%40[A-Za-z0-9_.]"
struct proctab_st
{
  char procname[PROCNAMELEN];
  pid_t procpid;
  struct timeval procstart;
} *proctab;
int proctablen, proctabcnt;

int procnice = 1;

// the commands are lines, no more than 4k
#define MAXLINELEN 4096

static void
usage (char *progname)
{
  if (!progname)
    progname = "**execicar**";
  fprintf (stderr, "usage: %s\n"
	   "   [--help|-h] ## to get this help\n"
	   "   [--incmd|-i channel] ## input command channel (if only digits, fd)\n"
	   "   [--outrep|-o channel] ## output report channel (if only digits, fd)\n"
	   "   [--nice|-n level] ## nice level (default 1)\n"
	   "   [--shell|s progpath] ## used shell - default to $SHELL or else /bin/sh\n"
	   "   [--syslog|l] ## syslog\n"
	   "   [--quiet|q] ## quiet\n", progname);
  fprintf (stderr, "version %s git %s\n",
	   execicar_ident, execicar_gitid);
  fflush (stderr);
}


static void main_loop ();


static volatile sig_atomic_t got_sigchild;

static void
sigchld_handler (int signnum)
{
  assert(signnum > 0);
  got_sigchild = 1;
}

int
main (int argc, char **argv)
{
  struct tms initial_tms;
  int longoptindex = 0;
  int getoptix = 0;
  long fd;
  char *end;
  char *str;
  char *progname = argv[0];
  if (!progname)
    progname = "**execicar**";
  fdcmd = -1;
  devnull = open ("/dev/null", O_RDONLY);
  if (devnull < 0)
    {
      perror ("/dev/null");
      return 1;
    };
  startclock = times (&initial_tms);
  secondpertick = 1.0 / (double) sysconf (_SC_CLK_TCK);
  str = getenv ("SHELL");
  if (str)
    shell = str;
  if (access(shell, X_OK))
    {
      fprintf (stderr, "%s has bad shell %s - %s\n",
	       progname, shell, strerror(errno));
      exit (EXIT_FAILURE);
    };
      
  while ((getoptix = getopt_long (argc, argv, "i:o:s:n:hlq",
				  options, &longoptindex)) > 0)
    {
      end = 0;
      switch (getoptix)
	{
	case 'i':
	  if (fdcmd >= 0)
	    {
	      fprintf (stderr, "%s duplicate input command [-i %s]\n",
		       progname, optarg);
	      return 1;
	    };
	  if ((fd = strtol (optarg, &end, 10)) >= 0 && end && !end[0])
	    fdcmd = fd;
	  else
	    fdcmd = open (optarg, O_RDONLY);
	  if (fdcmd < 0)
	    {
	      fprintf (stderr, "%s cannot open command input %s - %s\n",
		       progname, optarg, strerror (errno));
	      return 1;
	    };
	  break;
	case 'o':
	  if (frep)
	    {
	      fprintf (stderr, "%s duplicate output reply [-o %s]\n",
		       progname, optarg);
	      return 1;
	    };
	  if ((fd = strtol (optarg, &end, 10)) >= 0 && end && !end[0])
	    frep = fdopen (fd, "w");
	  else
	    frep = fopen (optarg, "w");
	  if (!frep)
	    {
	      fprintf (stderr, "%s cannot open reply output %s - %s\n",
		       progname, optarg, strerror (errno));
	      return 1;
	    };
	  break;
	case 's':
	  if (optarg)
	    shell = optarg;
	  break;
	case 'n':
	  if (optarg)
	    procnice = atoi (optarg);
	  if (procnice < -1)
	    procnice = -1;
	  else if (procnice > 100)
	    procnice = 100;
	  break;
	case 'h':
	  usage (progname);
	  return 0;
	case 'l':
	  dolog = 1;
	  break;
	case 'q':
	  quiet = 1;
	  break;
	default:
	  usage (progname);
	  return 1;
	}
    };
  if (fdcmd < 0)
    {
      fprintf (stderr, "%s no command input \n", progname);
      return 1;
    };
  if (!frep)
    {
      fprintf (stderr, "%s no reply output \n", progname);
      return 1;
    };
  if (access (shell, X_OK))
    {
      fprintf (stderr, "invalid shell %s - %s\n", shell, strerror (errno));
      return 1;
    };
  if (dolog)
    openlog ("execicar", LOG_PID, LOG_USER);
  proctab = calloc (10, sizeof (struct proctab_st));
  if (!proctab)
    {
      perror ("proctab");
      return 1;
    };
  proctablen = 10;
  proctabcnt = 0;
  signal (SIGCHLD, sigchld_handler);
  main_loop ();
  return 0;
}				// end of main


int
find_proc_by_name (const char *pnam)
{
  int i;
  for (i = 0; i < proctablen; i++)
    if (!strncmp (pnam, proctab[i].procname, PROCNAMELEN))
      return i;
  return -1;
}

static void
wait_any_child (void)
{
  struct rusage rus;
  int status = 0, ix = 0;
  pid_t wpid = 0;
  double usertime, systemtime, realtime;
  struct timeval tv;
  while (status = 0, memset (&rus, 0, sizeof (rus)),
	 (wpid = wait3 (&status, WNOHANG, &rus)) > 0)
    {
      memset (&tv, 0, sizeof (tv));
      gettimeofday (&tv, 0);
      usertime = rus.ru_utime.tv_sec + 1.0e-6 * rus.ru_utime.tv_usec;
      systemtime = rus.ru_stime.tv_sec + 1.0e-6 * rus.ru_stime.tv_usec;
      for (ix = 0; ix < proctablen; ix++)
	if (proctab[ix].procpid == wpid)
	  {
	    realtime = (tv.tv_sec + 1.0e-6 * tv.tv_usec)
	      - (proctab[ix].procstart.tv_sec
		 + 1.0e-6 * proctab[ix].procstart.tv_usec);
	    if (WIFEXITED (status))
	      {
		int cod = WEXITSTATUS (status);
		if (!cod)
		  fprintf (frep, "ENDOK '%s' %.3f %.3f %.3f\n",
			   proctab[ix].procname, usertime, systemtime,
			   realtime);
		else
		  fprintf (frep, "ENDFAIL '%s' %d %.3f %.3f %.3f\n",
			   proctab[ix].procname, cod,
			   usertime, systemtime, realtime);
		memset (proctab + ix, 0, sizeof (struct proctab_st));
		proctabcnt--;
	      }
	    else if (WIFSIGNALED (status))
	      {
		int sig = WTERMSIG (status);
		fprintf (frep, "ENDKILL '%s' %s %.3f %.3f %.3f\n",
			 proctab[ix].procname, strsignal (sig),
			 usertime, systemtime, realtime);
		memset (proctab + ix, 0, sizeof (struct proctab_st));
		proctabcnt--;
	      };
	    fflush (frep);
	  }
    }				// end while wait3
}				/* end wait_any_child */


static void
killall (int signum)
{
  int ix;
  pid_t pid;
  for (ix = 0; ix < proctablen; ix++)
    if ((pid = proctab[ix].procpid) > 0)
      if (kill (pid, signum) < 0)
	{
	  fprintf (stderr,
		   "kill of process '%s' #%d signal %d:%s failed: %s\n",
		   proctab[ix].procname, (int) pid, signum,
		   strsignal (signum), strerror (errno));
	  fflush (stderr);
	};
  wait_any_child ();
}				/* end of killall */

void
process_line (const char *linebuf)
{
  const char *arg = NULL;
  int ix;
  int endpos = 0;
  char pnam[PROCNAMELEN];
  memset (pnam, 0, sizeof (pnam));
  if (linebuf[0] == 0 || linebuf[0] == '#')
    (void) 0;
  ////////////////// cd builtin
  else if (!strncmp (linebuf, "cd ", 3))
    {
      arg = linebuf + 3;
      while (*arg && isspace (*arg))
	arg++;
      if (*arg && isgraph (*arg))
	{
	  if (chdir (arg))
	    {
	      if (dolog)
		syslog (LOG_ERR, "failed chdir %s : %m", arg);
	      else if (!quiet)
		fprintf (stderr, "failed chdir %s: %s\n", arg,
			 strerror (errno));
	    }
	  else if (dolog)
	    {
	      syslog (LOG_INFO, "done chdir %s", arg);
	    };
	};
    }
  ////////////////// do builtin
  else if (sscanf (linebuf, "do " PROCNAMEFMT " : %n",
		   pnam, &endpos) > 0 && endpos > 0)
    {
      ix = find_proc_by_name (pnam);
      if (ix >= 0)
	{
	  if (!quiet)
	    fprintf (stderr, "duplicate procname %s", pnam);
	  if (dolog)
	    syslog (LOG_ERR, "duplicate procname %s", pnam);
	  fprintf (frep, "ERROR 'do: duplicate procname %s'\n", pnam);
	}
      else
	{
	  pid_t pid;
	  fflush (frep);
	  if (proctabcnt >= proctablen - 1)
	    {
	      int newlen = ((3 * proctabcnt / 2) | 0xf) + 0x11;
	      int newix;
	      struct proctab_st *newtab = calloc ((size_t) newlen,
						  sizeof (struct proctab_st));
	      if (!newtab)
		{
		  fprintf (stderr,
			   "fail to allocate proctable of %d entries - %s\n",
			   newlen, strerror (errno));
		  if (dolog)
		    syslog (LOG_CRIT,
			    "failed to allocate proctable of %d entries - %m",
			    newlen);
		  exit (1);
		};
	      proctabcnt = 0;
	      for (newix = 0; newix < proctablen; newix++)
		if (proctab[newix].procname[0])
		  newtab[proctabcnt++] = proctab[newix];
	      free (proctab);
	      proctab = newtab;
	      proctablen = newlen;
	    }
	  pid = fork ();
	  arg = linebuf + endpos;
	  if (pid == 0)
	    {
	      /* child process */
	      const char *shellarg[4];
	      int i;
	      signal (SIGCHLD, SIG_IGN);
	      (void) close (fdcmd);
	      fclose (frep);
	      (void) dup2 (devnull, STDIN_FILENO);
	      (void) close (devnull);
	      for (i = 3; i < 64; i++)
		(void) close (i);
	      shellarg[0] = shell;
	      shellarg[1] = "-c";
	      shellarg[2] = arg;
	      shellarg[3] = 0;
	      nice (procnice);
	      usleep (100);
	      execv (shell, (char *const *) shellarg);
	      perror (shell);
	      exit (127);
	    }
	  else if (pid > 0)
	    {
	      /* parent process */
	      for (ix = 0; ix < proctablen; ix++)
		if (!proctab[ix].procname[0])
		  break;
	      assert (ix < proctablen);
	      proctabcnt++;
	      strncpy (proctab[ix].procname, pnam, PROCNAMELEN-1);
	      proctab[ix].procpid = pid;
	      gettimeofday (&proctab[ix].procstart, 0);
	      fflush (frep);
	      wait_any_child ();
	    }
	  else
	    {
	      fprintf (stderr, "cannot fork for %s\n", pnam);
	      if (dolog)
		syslog (LOG_ERR, "failed to fork for %s - %m", pnam);
	    };
	};
    }
  ////////////////// say builtin
  else if (!strncmp (linebuf, "say ", 4))
    {
      arg = linebuf + 4;
      while (*arg && isspace (*arg))
	arg++;
      if (*arg && isgraph (*arg))
	{
	  fprintf (frep, "%s\n", arg);
	};
    }
  ///////////////// term builtin
  else if (sscanf (linebuf, "term " PROCNAMEFMT " %n",
		   pnam, &endpos) > 0 && endpos > 0)
    {
      ix = find_proc_by_name (pnam);
      if (ix >= 0 && proctab[ix].procpid > 0)
	{
	  kill (proctab[ix].procpid, SIGTERM);
	}
    }
  ///////////////// kill builtin
  else if (sscanf (linebuf, "kill " PROCNAMEFMT " %n",
		   pnam, &endpos) > 0 && endpos > 0)
    {
      ix = find_proc_by_name (pnam);
      if (ix >= 0 && proctab[ix].procpid > 0)
	{
	  kill (proctab[ix].procpid, SIGKILL);
	}
    }
  //////////////// termall
  else if (!strcmp (linebuf, "termall"))
    {
      killall (SIGTERM);
    }
  //////////////// killall
  else if (!strcmp (linebuf, "killall"))
    {
      killall (SIGKILL);
    }
  //////////////// version
  else if (!strcmp (linebuf, "version"))
    {
      fprintf (frep, "VERSION '%s'\n", execicar_ident);
    }
  //////////////// exit
  else if (!strcmp (linebuf, "exit"))
    {
      if (dolog)
	syslog (LOG_INFO, "execicar got exit command");
      if (!quiet)
	fprintf (stderr, "execicar got exit command\n");
      killall (SIGTERM);
      fflush (frep);
      fflush (stderr);
      usleep (10000);
      exit (0);
    }
  //////////////// quit
  else if (!strcmp (linebuf, "quit"))
    {
      killall (SIGTERM);
      usleep (10000);
      killall (SIGQUIT);
      usleep (10000);
      fflush (frep);
      fflush (stderr);
      exit (0);
    }
  //// ----------------  otherwise unrecognised command
  else if (arg && arg[0])
    {
      if (!quiet)
	fprintf (stderr, "invalid command %.50s\n", arg);
      if (dolog)
	syslog (LOG_ERR, "invalid command %.50s", arg);
    };
}				/* end of process_line */

void
main_loop (void)
{
  char *eol = NULL;
  char *curp = NULL;
  char *endp = NULL;
  int eof = 0;
  int pollres = 0;
  int rdcnt = 0;
  int rdlen = 0;
  static char buffer[2 * MAXLINELEN + 10];
  struct pollfd pfd[1];
  curp = buffer;
  memset (buffer, 0, sizeof (buffer));
  memset (pfd, 0, sizeof (pfd));
  do
    {
      eof = 0;
      rdcnt = 0;
      wait_any_child ();
      pfd[0].fd = fdcmd;
      pfd[0].events = POLLIN;
      pfd[0].revents = 0;
      pollres = poll (pfd, 1, 600);
      if (pollres > 0 && (pfd[0].revents & POLLIN))
	{
	  rdlen = (buffer + sizeof (buffer) - 4) - curp;
	  assert (rdlen > 0);
	  *curp = 0;
	  rdcnt = read (fdcmd, curp, rdlen);
	  if (rdcnt == 0)
	    eof = 1;
	  else if (rdcnt < 0 && errno == EINTR)
	    wait_any_child ();
	  else if (rdcnt > 0)
	    {
	      endp = curp + rdcnt;
	      *endp = (char) 0;
	      do
		{
		  eol = strchr (curp, '\n');
		  if (!eol)
		    break;
		  *eol = 0;
		  process_line (curp);
		  curp = eol + 1;
		}
	      while (eol);
	      if (curp > buffer)
		{
		  int curlen = endp - curp;
		  assert (curlen >= 0 && curlen < (int) sizeof (buffer));
		  memmove (buffer, curp, curlen);
		  curp = buffer + curlen;
		  *curp = 0;
		};
	    }
	}
      else if (pollres == 0 || (pollres < 0 && errno == EINTR))
	wait_any_child ();
      else if (pollres > 0 && (pfd[0].revents & POLLHUP))
	{
	  if (!quiet)
	    fprintf (stderr, "execicar command input hanged up\n");
	  if (dolog)
	    syslog (LOG_ERR, "execicar command input hanged up\n");
	  killall (SIGTERM);
	  usleep (200000);
	  killall (SIGQUIT);
	  usleep (200000);
	  killall (SIGKILL);
	  eof = 1;
	  break;
	}
      fflush (frep);
    }
  while (!eof);			// end while !eof
}				// end of main_loop


/* eof execicar.c */


/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make execicar" ;;
 ** End: ;;
 ****************/
