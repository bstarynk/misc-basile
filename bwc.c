/* file bwc.c from https://github.com/bstarynk/misc-basile/
   count lines and their width using getline(3)

   © Copyright Basile Starynkevitch 2017
   program released under GNU general public license

   this is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   this is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

const char *progname = NULL;

void
count_lines (FILE * f, char *name)
{
  size_t linsiz = 256;
  clock_t stc = clock ();
  char *linbuf = malloc (linsiz);
  long lincnt = 0;
  long linwidth = 0;
  long off = 0;
  if (!linbuf)
    {
      perror ("malloc linbuf");
      exit (EXIT_FAILURE);
    };
  memset (linbuf, 0, linsiz);
  do
    {
      off = ftell(f);
      ssize_t linlen = getline (&linbuf, &linsiz, f);
      if (linlen < 0)
	break;
      off += linlen;
      lincnt++;
      if (linwidth < linlen)
	linwidth = linlen;
      if (linlen > (1 << 20))
	{
	  fprintf (stderr, "in file %s line#%ld is huge: %ld\n",
		   name, lincnt, linwidth);
	  exit (EXIT_FAILURE);
	};
    }
  while (!feof (f));
  clock_t stf = clock ();
  double cput = (stf - stc) * 1.0e-6;
  printf ("%s: %ld lines, maxwidth %ld, %ld bytes in %.5f cpu sec, %.3f µs/l\n",
	  name, lincnt, linwidth, off, cput, (cput * 1.0e6) / lincnt);
  free (linbuf);
}				/* end count_lines */

int
main (int argc, char **argv)
{
  progname = argv[0];
  bool withmmap = false;
  if (argc < 2 || !strcmp (argv[1], "-h") || !strcmp (argv[1], "--help"))
    {
      fprintf (stderr, "usage: %s [ -m # mmap | -p # plain ] files... \n",
	       argv[0]);
      exit (EXIT_SUCCESS);
    };
  for (int ix = 1; ix < argc; ix++)
    {
      if (!strcmp (argv[ix], "-m"))
	{
	  withmmap = true;
	  continue;
	};
      if (!strcmp (argv[ix], "-p"))
	{
	  withmmap = false;
	  continue;
	};
      FILE *f = fopen (argv[ix], withmmap ? "rm" : "r");
      if (!f)
	{
	  perror (argv[ix]);
	  exit (EXIT_FAILURE);
	};
      count_lines (f, argv[ix]);
    }
  return EXIT_SUCCESS;
}				/* end main */


/// end of file misc-basile/bwc.c
