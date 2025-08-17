/** file bwc.c from https://github.com/bstarynk/misc-basile/
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *
   count lines and their width using getline(3)
   Find large lines

   © Copyright Basile Starynkevitch 2017 - 2025
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
#include <errno.h>

const char *progname = NULL;

int linlargelimit = 80;		/// above that line width limit in bytes large lines are noticed
void
count_lines (FILE *f, char *name)
{
  size_t linsiz = 256;
  int largdim = 32;
  int largcnt = 0;
  int *largarr = calloc (largdim, sizeof (int));
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
  if (!largarr)
    {
      perror ("calloc largarr");
      exit (EXIT_FAILURE);
    };
  memset (linbuf, 0, linsiz);
  do
    {
      off = ftell (f);
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
      if (linlargelimit > 0 && linlen > linlargelimit)
	{
	  if (largcnt > largdim)
	    {
	      int newdim = ((largdim + largdim / 4 + 4) | 0xf) + 1;
	      int *newarr = calloc (newdim, sizeof (int));
	      if (!newarr)
		{
		  fprintf (stderr,
			   "%s: calloc newarr (newdim=%d) failed - %s\n",
			   progname, newdim, strerror (errno));
		  exit (EXIT_FAILURE);
		}
	      memcpy (newarr, largarr, sizeof (int) * largcnt);
	      int *oldarr = largarr;
	      largarr = newarr;
	      free (oldarr);
	      largdim = newdim;
	    };
	  largarr[largcnt++] = lincnt;
	};
    }
  while (!feof (f));
  clock_t stf = clock ();
  double cput = (stf - stc) * 1.0e-6;
  printf
    ("%s: %ld lines, maxwidth %ld, %ld bytes in %.5f cpu sec, %.3f µs/l\n",
     name, lincnt, linwidth, off, cput, (cput * 1.0e6) / lincnt);
  if (largcnt > 0)
    {
      printf ("%s has %d large lines:\n", name, largcnt);
      for (int i = 0; i < largcnt; i++) {
	if ((i+1) % 8 == 0)
	  puts("\n ...");
	printf (" %d", largarr[i]);
      }
      fputc ('\n', stdout);
      fflush (NULL);
    };
  free (linbuf);
  free (largarr);
}				/* end count_lines */

int
main (int argc, char **argv)
{
  progname = argv[0];
  bool withmmap = false;
  if (argc < 2 || !strcmp (argv[1], "-h") || !strcmp (argv[1], "--help"))
    {
      fprintf (stderr,
	       "usage: %s [ -m # mmap | -p # plain ] [ -l limit ] files... \n",
	       progname);
      fprintf (stderr, "\t with -m use mmap(2); with -p dont\n");
      fprintf (stderr, "\t -l 80 is the default line line length limit\n");
      fprintf (stderr, "\t also with --version and --help\n");
      exit (EXIT_SUCCESS);
    };
  if (argc < 2 || !strcmp (argv[1], "-V") || !strcmp (argv[1], "--version"))
    {
      printf ("%s version %s built %s\n",
	      progname, BWC_GIT, __DATE__ "@" __TIME__);
      fflush (NULL);
      exit (EXIT_SUCCESS);
    }
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
      if (!strcmp (argv[ix], "-l"))
	{
	  if (ix < argc + 1)
	    linlargelimit = atoi (argv[ix + 1]);
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
