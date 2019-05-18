/*** file makeprimes.c from https://github.com/bstarynk/misc-basile/
 *
 * it uses /usr/games/primes (which is a clever program)
 *
 * it takes two arguments LIM and FRA
 * the optional third CMD is a replacement for /usr/games/primes
 *
 * it prints prime numbers, with a comma after each of them, from 2 to
 * LIM
 *
 * the prime numbers are growing by a fraction of FRA from one to the next.
 *
 * in other words, it is like
 * /usr/games/primes 3 $LIM |  awk '($1>p+p/FRA){print $1, ","; p=$1}'
 ***/

/// FIXME: should maybe use https://github.com/kimwalisch/primesieve

/** Copyright (C)  2019  Basile Starynkevitch
    makeprimes is showing some prime numbers, using the BSD primes program

    makeprimes is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3, or (at your option)
    any later version.

    makeprimes is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with makeprimes; see the file COPYING3.   If not see
    <http://www.gnu.org/licenses/>.
***/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BSD_PRIMES_PROG "/usr/games/primes"

int
main (int argc, char **argv)
{
  if (argc < 3)
    {
      fprintf (stderr, "usage: %s LIM FRA [CMD]; see comments in %s\n",
	       argv[0], __FILE__);
      exit (EXIT_FAILURE);
    };
  long lim = atol (argv[1]);
  int fra = atoi (argv[2]);
  char *cmd = (argc == 4) ? argv[3] : BSD_PRIMES_PROG;
  if (lim < 1000)
    lim = 1000;
  if (fra < 3)
    fra = 3;
  printf ("// primes up to %ld, growing with %d, using %s\n", lim, fra, cmd);
  char cmdbuf[100];
  memset (cmdbuf, 0, sizeof (cmdbuf));
  snprintf (cmdbuf, sizeof (cmdbuf), "%s 2 %ld", cmd, lim);
  FILE *pcmd = popen (cmdbuf, "r");
  if (!pcmd)
    {
      perror (cmdbuf);
      exit (EXIT_FAILURE);
    };
  printf ("//// piping %s\n", cmdbuf);
  int incnt = 0;
  int outcnt = 0;
  long n = 0;
  long prevn = 0;
  while (fscanf (pcmd, " %ld", &n) > 0)
    {
      incnt++;
      if (n > prevn + prevn / fra)
	{
	  printf (" %ld,", n);
	  outcnt++;
	  if (outcnt % 4 == 0)
	    {
	      putchar ('\n');
	      if (outcnt % 100 == 0)
		{
		  printf ("//#%d of %d\n", outcnt, incnt);
		  fflush (NULL);
		};
	      if (outcnt < 100 || outcnt % 64 == 0)
		fflush (NULL);
	    }
	  prevn = n;
	};
    }
  if (pclose (pcmd))
    perror ("pclose");
  printf ("\n/// end, read %d primes, printed %d primes, so %.5f%%\n",
	  incnt, outcnt, (100.0 * outcnt) / incnt);
  fflush (NULL);
}

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "gcc -Wall -O2 -g makeprimes.c -o makeprimes" ;;
 ** End: ;;
 ****************/
