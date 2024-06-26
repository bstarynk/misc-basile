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
#include <errno.h>
#include <string.h>
#include "libgccjit.h"
#include "jansson.h"

#ifndef GITID
#error GITID should be defined in compilation command
#endif

#ifndef MD5SUM
#error MD5SUM should be definied in compilation command
#endif

const char *minicomp_progname;

const char minicomp_gitid[] = GITID;

const char minicomp_md5sum[] = MD5SUM;

gcc_jit_context *minicomp_jitctx;

volatile const char *minicomp_fatal_file;
volatile int minicomp_fatal_line;
void minicomp_fatal_stop_at (const char *, int) __attribute__((noreturn));
#define MINICOMP_FATAL_AT_BIS(Fil,Lin,Fmt,...) do {			\
    fprintf(stderr, "MINICOMP-GCCJIT FATAL:%s:%d: <%s>\n " Fmt "\n\n",	\
            Fil, Lin, __func__, ##__VA_ARGS__);				\
    minicomp_fatal_stop_at(Fil,Lin); } while(0)


json_t *minicomp_json_code_array;

#define MINICOMP_FATAL_AT(Fil,Lin,Fmt,...) MINICOMP_FATAL_AT_BIS(Fil,Lin,Fmt,##__VA_ARGS__)

#define MINICOMP_FATAL(Fmt,...) MINICOMP_FATAL_AT(__FILE__,__LINE__,Fmt,##__VA_ARGS__)

void
minicomp_fatal_stop_at (const char *fil, int lin)
{
  minicomp_fatal_file = fil;
  minicomp_fatal_line = lin;
  fprintf (stderr, "%s git %s md5sum %s\n", minicomp_progname,
	   minicomp_gitid, minicomp_md5sum);
  fflush (NULL);
  abort ();
}				/* end minicomp_fatal_stop_at */

void
minicomp_show_version (void)
{
  printf ("%s version: gitid %s\n using libgccjit %d.%d.%d\n"
	  " jansson %s\n"
	  " built %s\n",
	  minicomp_progname, minicomp_gitid,
	  gcc_jit_version_major (),
	  gcc_jit_version_minor (),
	  gcc_jit_version_patchlevel (),
	  jansson_version_str (), __DATE__ "@" __TIME__);
  printf ("%s md5 signature: %s\n", minicomp_progname, minicomp_md5sum);
}				/* end minicomp_show_version */

void
minicomp_show_help (void)
{
  printf ("%s usage:\n", minicomp_progname);
  printf ("\t --help                                 #show this help\n");
  printf ("\t --version                              #show version info\n");
}				/* end minicomp_show_help */

void
minicomp_process_json (FILE *fil, const char *name)
{
  json_error_t jerr = { };
  json_t *js =
    json_loadf (fil,
		JSON_DISABLE_EOF_CHECK | JSON_SORT_KEYS |
		JSON_REJECT_DUPLICATES,
		&jerr);
  if (!js)
    MINICOMP_FATAL ("failed to load JSON file %s:\n"
		    "error %s, in %s:%d:%d (p %d)",
		    name, jerr.text, jerr.source, jerr.line, jerr.column,
		    jerr.position);
  else if (json_is_array (js))
    {
      size_t ln = json_array_size (js);
      for (size_t ix = 0; ix < ln; ix++)
	{
	  json_t *jcomp = json_array_get (js, ix);
	  if (jcomp)
	    json_array_append (minicomp_json_code_array, jcomp);
	}
    }
  else if (json_is_object (js))
    {
      json_array_append (minicomp_json_code_array, js);
    }
  else
    {
      const char *jty = "?";
      if (json_is_string (js))
	jty = "string";
      else if (json_is_integer (js))
	jty = "integer";
      else if (json_is_real (js))
	jty = "real";
      else if (json_is_boolean (js))
	jty = "boolean";
      else if (json_is_null (js))
	jty = "null";
      MINICOMP_FATAL
	("JSON file %s dont contain array or JSON object but jansson-type #%d %s",
	 name, json_typeof (js), jty);
    };
  json_decref (js);
}				/* end minicomp_process_json */

void
minicomp_generate_code (void)
{
#warning unimplemented minicomp_generate_code
  MINICOMP_FATAL ("unimplemented minicomp_generate_code");
}				/* end minicomp_generate_code */

void
minicomp_handle_arguments (int argc, char **argv)
{
  int aix = 0;
  for (aix = 1; aix < argc; aix++)
    {
      const char *curarg = argv[aix];
      if (curarg[0] == '-')
	{
#warning incomplete minicomp_handle_arguments
	  MINICOMP_FATAL("incomplete minicomp_handle_arguments curarg#%d=%s",
			 aix, curarg);
	}
      else if (curarg[0] == '!' || curarg[0] == '|')
	{
	  FILE *pipinp = popen (curarg + 1, "r");
	  if (!pipinp)
	    MINICOMP_FATAL ("failed to popen input JSON pipe %s: %s",
			    curarg + 1, strerror (errno));
	  minicomp_process_json (pipinp, curarg);
	  int r = pclose (pipinp);
	  pipinp = NULL;
	  if (r)
	    MINICOMP_FATAL ("popen input JSON pipe %s failed returning %d",
			    curarg + 1, r);
	}
      else if (!access (curarg, R_OK))
	{
	  FILE *inpfil = fopen (curarg, "r");
	  if (!inpfil)
	    MINICOMP_FATAL ("failed to open input JSON file %s: %s",
			    curarg, strerror (errno));
	  minicomp_process_json (inpfil, curarg);
	  fclose (inpfil);
	}
    }
}				/* end minicomp_handle_arguments */

int
main (int argc, char **argv)
{
  minicomp_progname = argv[0];
  if (argc > 1 && !strcmp (argv[1], "--version"))
    {
      minicomp_show_version ();
      return 0;
    }
  else if (argc > 1 && !strcmp (argv[1], "--help"))
    minicomp_show_help ();
  minicomp_jitctx = gcc_jit_context_acquire ();
  if (!minicomp_jitctx)
    MINICOMP_FATAL ("failed to acquire the GCCJIT context (%s)",
		    strerror (errno));
  minicomp_json_code_array = json_array ();
  if (!minicomp_json_code_array)
    MINICOMP_FATAL ("failed to create initial minicomp_json_code_array (%s)",
		    strerror (errno));
  minicomp_handle_arguments (argc, argv);
  minicomp_generate_code ();
  json_decref (minicomp_json_code_array);
  minicomp_json_code_array = NULL;
  gcc_jit_context_release (minicomp_jitctx);
  minicomp_jitctx = NULL;
}				/* end main */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make minicomp-gccjit" ;;
 ** End: ;;
 ****************/

/// end of minicomp-gccjit.c
