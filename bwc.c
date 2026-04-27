/** file bwc.c from https://github.com/bstarynk/misc-basile/
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *
   count lines and their width using getline(3)
   Find large lines

   © Copyright (C) Basile Starynkevitch 2017 - 2026
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
#include <unistr.h>

const char *progname = NULL;
bool emacsout = false;

int linlargelimit = 80;		/// above that line width limit in
				/// UTF8 glyphs large lines are
				/// noticed
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
      long oldoff = -1;
      off = ftell (f);
      if (off>=0)
	oldoff= off;
      ssize_t linbytes = getline (&linbuf, &linsiz, f);
      if (linbytes < 0)
	break;
      lincnt++;
      off += linbytes;
      if (!u8_check((const uint8_t*)linbuf, (size_t)linbytes)) {
	fprintf(stderr,
		"%s:%ld is malformed UTF8 line (byte offset %ld)\n",
		name, lincnt, oldoff);
	continue;
      }
      if (linbytes > (1 << 20))
	{
	  fprintf (stderr, "%s:%ld is huge (%ld bytes) at byte offset %ld\n",
		   name, lincnt, linbytes, oldoff);
	  exit (EXIT_FAILURE);
	};
      size_t linlen =
	u8_mbsnlen((const uint8_t*)linbuf, (size_t)linbytes);
      if (linlargelimit > 0 && (int) linlen > linlargelimit)
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
    ("%s:: (%ld lines, width %ld, %ld bytes in %.5f cpu sec, %.3f µs/l)\n",
     name, lincnt, linwidth, off, cput, (cput * 1.0e6) / lincnt);
  if (largcnt > 0)
    {
      printf ("# %s has %d large lines:\n", name, largcnt);
      if (emacsout) {
        for (int i = 0; i < largcnt; i++)
          printf("%s:%d: LARGE\n", name, largarr[i]);
      } else {
        for (int i = 0; i < largcnt; i++) {
	     if ((i+1) % 9 == 0)
	       fputs("\n ...", stdout);
	     printf (" %d", largarr[i]);
      }
      if (largcnt > 40)
	     printf(" # large in %s\n", name);
      else
	     fputc ('\n', stdout);
      fflush (NULL);
    };
  free (linbuf);
  free (largarr);
}				/* end count_lines */


void
usage(void)
{
  printf("usage: %s [ -m # mmap | -p # plain ]\n"
	 "\t [ -l limit ]\n"
	 "\t [ -e ]\n"
	 "\t files... \n",
	 progname);
  printf("\t with -m use mmap(2); with -p dont\n");
  printf("\t -l 80 is the default line length limit\n");
  printf("\t -e for GNU emacs friendly output <file>:<line>\n");
  printf("\t also with --version and --help\n");
} /* end of usage */

int
main (int argc, char **argv)
{
  progname = argv[0];
  bool withmmap = false;
  if (argc < 2 || !strcmp (argv[1], "-h") || !strcmp (argv[1], "--help"))
    {
      usage();
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
      if (!strcmp (argv[ix], "-e"))
	{
	  emacsout = true;
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

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make bwc" ;;
 ** End: ;;
 ****************/


/// end of file misc-basile/bwc.c
