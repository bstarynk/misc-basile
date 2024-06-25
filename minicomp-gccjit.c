// file misc-basile/minicomp-gccjit.c
// SPDX-License-Identifier: GPL-3.0-or-later
//  © Copyright Basile Starynkevitch 2024
/// program released under GNU General Public License v3+
///
/// this is free software; you can redistribute it and/or modify it under
/// the terms of the GNU General Public License as published by the Free
/// Software Foundation; either version 3, or (at your option) any later
/// version.
///
/// this is distributed in the hope that it will be useful, but WITHOUT
/// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
/// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
/// License for more details.

#define _GNU_SOURCE 1
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libgccjit.h"
#include "jansson.h"

#ifndef GITID
#error GITID should be defined in compilation command
#endif

#ifndef MD5SUM
#error MD5SUM should be definied in compilation command
#endif

const char*minicomp_progname;

const char
minicomp_gitid[]=GITID;

const char
minicomp_md5sum[]=MD5SUM;

gcc_jit_context* minicomp_jitctx;

void
minicomp_show_version(void)
{
    printf("%s version: gitid %s\n using libgccjit %d.%d.%d\n"
	   " jansson %s\n"
	   " built %s\n",
	   minicomp_progname, minicomp_gitid,
	   gcc_jit_version_major(),
	   gcc_jit_version_minor(),
	   gcc_jit_version_patchlevel(),
	   jansson_version_str(),
	   __DATE__ "@" __TIME__);
    printf("%s md5 signature: %s\n", minicomp_progname, minicomp_md5sum);
} /* end minicomp_show_version */

void
minicomp_show_help(void)
{
  printf("%s usage:\n", minicomp_progname);
  printf("\t --help                                 #show this help\n");
  printf("\t --version                              #show version info\n");
} /* end minicomp_show_help */

int
main(int argc, char**argv)
{
  minicomp_progname=argv[0];
  if (argc > 1 && !strcmp(argv[1], "--version")) {
    minicomp_show_version();
    return 0;
  }
  else if (argc > 1 && !strcmp(argv[1], "--help"))
    minicomp_show_help();
  minicomp_jitctx = gcc_jit_context_acquire();
  gcc_jit_context_release (minicomp_jitctx);
  minicomp_jitctx = NULL;
} /* end main */
/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make minicomp-gccjit" ;;
 ** End: ;;
 ****************/

/// end of minicomp-gccjit.c
