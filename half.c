// fichier half.c

/* Copyright 2005 - 2019 Basile Starynkevitch
   <basile@starynkevitch.net>

This HALF program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2, or (at
your option) any later version.

HALF is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

--

The half program was designed (in 2005) to run something at half load,
on some old crappy laptop (MSI 270, a Turion laptop) which overheated.
The actual hardware issue was faulty RAM modules. It took me a year to
find out that (thanks to Sebastian Pop).

With that program, I was able to build tbe Linux kernel from source
code using the "half make" command. Without half, my then laptop
overheated so much that kernel compilation failed.

Today that program might be perhaps used to simulate bugs, e.g. by
sending SIGSEGV signals to a process group. Just compile it with
-DSTOP_SIGNAL=SIGSEGV for example; see the comments near end of file
for more.

*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>


#ifndef MY_STOP_SIGNAL
#define MY_STOP_SIGNAL SIGSTOP
#endif /*MY_STOP_SIGNAL*/

pid_t child;
volatile int running;
void
time_handler (int sig)
{
  running = !running;
}

volatile int sendsig;

void
propagate_handler (int sig)
{
  sendsig = sig;
}

int
main (int argc, char **argv)
{
  int delaymillisec = 250;
  struct itimerval itv;
  char **progargv;
  int progargc;
  if (argc <= 1)
    {
    usage:
      fprintf (stderr, "usage: %s [period_millisec] command...\n"
	       "#this program run at half period the given command..\n",
	       argv[0]);
      return 1;
    };
  int delarg = atoi (argv[1]);
  if (delarg > 0)
    {
      delaymillisec = delarg;
      if (delaymillisec > 900)
	delaymillisec = 900;
      if (delaymillisec < 10)
	delaymillisec = 10;
      progargc = argc - 2;
      progargv = argv + 2;
    }
  else
    {
      progargc = argc - 1;
      progargv = argv + 1;
    };
  if (progargc < 1)
    {
      fprintf (stderr, "%s missing command\n", argv[0]);
      goto usage;
    };
  child = fork ();
  if (!child)
    {
      usleep (delaymillisec * 500);
      setpgrp ();
      execvp (progargv[0], progargv);
      perror ("cannot exec");
      exit (127);
    };
  close (0);
  itv.it_interval.tv_sec = 0;
  itv.it_interval.tv_usec = delaymillisec * 1000;
  itv.it_value.tv_sec = 0;
  itv.it_value.tv_usec = delaymillisec * 1000;
  signal (SIGALRM, time_handler);
  signal (SIGTERM, propagate_handler);
  signal (SIGQUIT, propagate_handler);
  signal (SIGINT, propagate_handler);
  setitimer (ITIMER_REAL, &itv, 0);
  sendsig = 0;
  while (1)
    {
      int status = 0;
      int sig2send = 0;
      errno = 0;
      if (waitpid (child, &status, WNOHANG) > 0)
	{
	  if (WIFEXITED (status))
	    exit (WEXITSTATUS (status));
	  else if (WIFSIGNALED (status))
	    {
	      fprintf (stderr, "command %s ", *progargv);
	      fflush (stderr);
	      psignal (WTERMSIG (status), "interrupted by signal");
	      exit (126);
	    }
	}
      if (running)
	killpg (child, SIGCONT);
      else
	killpg (child, MY_STOP_SIGNAL);
      pause ();
      sig2send = sendsig;
      if (sig2send > 0)
	{
	  sendsig = 0;
	  killpg (child, sig2send);
	}
    };
}

// compile with gcc -O -g -Wall half.c -o half
