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
#include <errno.h>
#include <getopt.h>
#include <gc/gc.h>

#include "libguile.h"

#ifndef BUILDWITHGUILE_GIT
#error compilation command for build-with-guile should define macro BUILDWITHGUILE_GIT as a C string
#endif

const char bwg_gitid[] = BUILDWITHGUILE_GIT;
char *bwg_progname;
int bwg_argcnt;
char*const* bwg_argvec;


/// function called at exit time
void
bwg_atexit(void)
{
} /* end bwg_atexit */

int
main(int argc, char**argv)
{
  bwg_progname = argv[0];
  bwg_argcnt = argc;
  bwg_argvec = argv;
  GC_INIT();
  atexit(bwg_atexit);
} /* end main */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make build-with-guile" ;;
 ** End: ;;
 ****************/


// end of file misc-basile/build-with-guile.c
