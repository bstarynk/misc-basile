// SPDX-License-Identifier: GPL-3.0-or-later
/* file build-with-guile.c from https://github.com/bstarynk/misc-basile/

   A program compiling a set of C or C++ files extracting GUILE
   scripts from comments inside

    ©  Copyright Basile Starynkevitch  2025
   program released under GNU General Public License

   this is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   this is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   Coding convention: all symbols are prefixed with bwg_
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <syslog.h>
#include <gc/gc.h>
#include <stdarg.h>

#include "libguile.h"

#ifndef BUILDWITHGUILE_GIT
#error compilation command for build-with-guile should define macro BUILDWITHGUILE_GIT as a C string
#endif

void bwg_fatal_at (const char *fil, int lin, const char *fmt, ...)
  __attribute__((noreturn, format (printf, 3, 4)));

#define BWG_FATAL_AT_BIS(Fil,Lin,Fmt,...) do{bwg_fatal_at(Fil,Lin,Fmt,##__VA_ARGS__);}while(0)
#define BWG_FATAL_AT(Fil,Lin,Fmt,...) BWG_FATAL_AT_BIS((Fil),(Lin),Fmt,##__VA_ARGS__)
#define BWG_FATAL(Fmt,...) BWG_FATAL_AT(__FILE__,__LINE__,Fmt,##__VA_ARGS__)

////////////////////////////////////////////////////////////////


#define BWG_MAX_ALLOC (1<<30)	/* one gigabyte of maximal allocation */

const char bwg_gitid[] = BUILDWITHGUILE_GIT;

/// program name and arguments
char *bwg_progname;
int bwg_argcnt;
char *const *bwg_argvec;

/// processed program arguments passed to guile using scm_boot_guile
char **bwg_scm_argvec;
int bwg_scm_argcnt;

typedef void bwg_todo_sig_t (void *a1, void *a2, intptr_t a3, intptr_t a4);

// a todo structure 
struct bwg_todo_with_guile_st
{
  bwg_todo_sig_t *todo_funptr;
  void *todo_argptr1;
  void *todo_argptr2;
  intptr_t todo_argint1;
  intptr_t todo_argint2;
};
#warning should declare and handle a vector of struct bwg_todo_with_guile_st

/// allocation routine - wrapping GC_malloc
extern void *bwg_alloc_at (size_t nbytes, const char *file, int lineno,
			   const char *func);
#define BWG_ALLOC(Nbytes) bwg_alloc_at((Nbytes), __FILE__, __LINE__, __FUNCTION__)


/// atomic allocation routine - wrapping GC_malloc_atomic
extern void *bwg_atomalloc_at (size_t nbytes, const char *file, int lineno,
			       const char *func);
#define BWG_ATOMALLOC(Nbytes) bwg_atomalloc_at((Nbytes), __FILE__, __LINE__, __FUNCTION__)



void *
bwg_alloc_at (size_t nbytes, const char *file, int lineno, const char *func)
{
  void *res = NULL;
  if (nbytes == 0)
    return NULL;
  if (nbytes > BWG_MAX_ALLOC)
    {
      BWG_FATAL_AT (file, lineno,
		    "too many bytes %zd = %dMby to allocate from %s", nbytes,
		    (int) (nbytes >> 20), func);
      exit (EXIT_FAILURE);
    };
  res = GC_malloc (((nbytes + 30) | 0xf) + 1);
  if (!res)
    {
      BWG_FATAL_AT (file, lineno,
		    "failed to allocate %zd bytes = %dMby to allocate from %s",
		    nbytes, (int) (nbytes >> 20), func);
    }
  memset (res, 0, (nbytes | 0xf) + 1);
  return res;
}				/* end bwg_alloc_at */


void *
bwg_atomalloc_at (size_t nbytes, const char *file, int lineno,
		  const char *func)
{
  void *res = NULL;
  if (nbytes == 0)
    return NULL;
  if (nbytes > BWG_MAX_ALLOC)
    {
      BWG_FATAL_AT (file, lineno,
		    "too many bytes %zd = %dMby to atomic-allocate from %s",
		    nbytes, (int) (nbytes >> 20), func);
      exit (EXIT_FAILURE);
    };
  res = GC_malloc_atomic (((nbytes + 30) | 0xf) + 1);
  if (!res)
    {
      BWG_FATAL_AT (file, lineno,
		    "failed to atomic-allocate %zd bytes = %dMby from %s",
		    nbytes, (int) (nbytes >> 20), func);
      exit (EXIT_FAILURE);
      return NULL;
    }
  memset (res, 0, (nbytes | 0xf) + 1);
  return res;
}				/* end bwg_atomicalloc_at */


/// function called at exit time
void
bwg_atexit (void)
{
}				/* end bwg_atexit */


void
bwg_fatal_at (const char *fil, int lin, const char *fmt, ...)
{
  char buf[1024];
  va_list arg;
  memset (buf, 0, sizeof (buf));
  va_start (arg, fmt);
  vsnprintf (buf, sizeof (buf) - 1, fmt, arg);
  va_end (arg);
  syslog (LOG_CRIT, "%s fatal at %s:%d: %s", bwg_progname, fil, lin, buf);
  exit (EXIT_FAILURE);
}				/* end bwg_fatal_at */

/// show version information
void
bwg_show_version (void)
{
  printf ("%s version:\n", bwg_progname);
  printf ("\t git %s built %s @ %s (%s)\n", bwg_gitid, __DATE__, __TIME__,
	  __FILE__);
  fflush (NULL);
}				/* end bwg_show_version */

void
bwg_show_help (void)
{
  printf ("%s help:\n", bwg_progname);
  printf ("\t --version | -V                  # show version information\n");
  printf ("\t --help | -H                     # show this help\n");
  printf ("\t --source | -S <source-file>     # C or C++ file to process\n");
  printf
    ("\t --guile | -G <guile-expr>       # Guile R5RS expression to evaluate\n");
  printf ("\t --load | -L <guile-file>        # Guile script file to load\n");
  printf
    ("# GNU guile is a Scheme interpreter, see www.gnu.org/software/guile\n");
  fflush (NULL);
}				/* end bwg_show_help */

const struct option bwg_optarr[] = {
  {.name = "version",.has_arg = no_argument,.flag = (int *) 0,.val = 0},	/* -V */
  {.name = "help",.has_arg = no_argument,.flag = (int *) 0,.val = 0},	/* -H */
  {.name = "source",.has_arg = required_argument,.flag = (int *) 0,.val = 0},	/* -S<C/C++-file> */
  {.name = "guile",.has_arg = required_argument,.flag = (int *) 0,.val = 0},	/* -G<guile-expr> */
  {.name = "load",.has_arg = required_argument,.flag = (int *) 0,.val = 0},	/* -L<guile-file> */
  {.name = NULL,.has_arg = 0,.flag = (int *) 0,.val = 0}
};

void
bwg_handle_program_arguments (int argc, char **argv)
{
  if (argc <= 1)
    return;
  if (!strcmp (argv[1], "--version") || !strcmp (argv[1], "-V"))
    bwg_show_version ();
  else if (!strcmp (argv[1], "--help") || !strcmp (argv[1], "-H"))
    bwg_show_help ();
  int gor = -1;
  int ix = -1;
  do
    {
      gor = getopt_long (argc, argv, "VHS:G:L:", bwg_optarr, &ix);
      switch (gor)
	{
	case 'V':
	  bwg_show_version ();
	  exit (EXIT_SUCCESS);
	  break;
	case 'H':
	  bwg_show_help ();
	  exit (EXIT_SUCCESS);
	  break;
	case 'S':		/* C++ source file */
	  if (access (optarg, R_OK))
	    {
	      BWG_FATAL ("C or C++ file to process %s is not accessible - %s",
			 optarg, strerror (errno));
	      exit (EXIT_FAILURE);
	      return;
	    };
	  break;
	case 'G':		/* Guile expression */
	  /* optarg is a Guile expression like (+ 2 3) */
	  break;
	case 'L':		/* Guile file */
	  if (access (optarg, R_OK))
	    {
	      BWG_FATAL
		("Guile Scheme script file to load %s is not accessible - %s",
		 optarg, strerror (errno));
	      exit (EXIT_FAILURE);
	      return;
	    };
	  break;
	};
    }
  while (gor >= 0);
}				/* end bwg_handle_program_arguments */

int
main (int argc, char **argv)
{
  bwg_progname = argv[0];
  bwg_argcnt = argc;
  bwg_argvec = argv;
  GC_INIT ();
  bwg_handle_program_arguments (argc, argv);
  atexit (bwg_atexit);
}				/* end main */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make build-with-guile" ;;
 ** End: ;;
 ****************/


// end of file misc-basile/build-with-guile.c
