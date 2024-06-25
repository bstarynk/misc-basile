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


const char
minicomp_gitid[]=GITID;

gcc_jit_context* jitctx;

int
main(int argc, char**argv)
{
  if (argc > 1 && !strcmp(argv[1], "--version")) {
    printf("%s version: gitid %s\n using libgccjit %d.%d.%d\n built %s\n",
	   argv[0], minicomp_gitid,
	   gcc_jit_version_major(),
	   gcc_jit_version_minor(),
	   gcc_jit_version_patchlevel(), __DATE__ "@" __TIME__);
    return 0;
  };
  jitctx = gcc_jit_context_acquire();
  gcc_jit_context_release (jitctx);
  jitctx = NULL;
} /* end main */
/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make minicomp-gccjit" ;;
 ** End: ;;
 ****************/

/// end of minicomp-gccjit.c
