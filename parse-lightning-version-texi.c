// SPDX-License-Identifier: GPL-3.0-or-later
// file misc-basile/parse-lightning-version-texi.c
// related to https://lists.gnu.org/archive/html/lightning/2026-06/msg00010.html
/***
    © Copyright 2026 (C) Basile Starynkevitch, France <basile@starynkevitch.net>
   program released under GNU General Public License v3+

   this is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   this is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   The executable takes as a unique argument the doc/version.texi file of GNU lightning.
****/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

const char *prog_name;
char lightning_updated[80];
char lightning_version[80];

/// the parse_lightning_version files return 0 if done ok, and some
/// number (eg a line number) on failure
int
parse_lightning_version (FILE *f)
{
  char linebuf[512];
  assert (f != nullptr);
  do
    {
      int pos = -1;
      memset (linebuf, 0, sizeof (linebuf));
      char *l = fgets (linebuf, sizeof (linebuf) - 1, f);	// so last byte remain zeroed
      if (!l) {
	if (feof(f))
	  break;
	else
	  return __LINE__;
      };
      if (sscanf (linebuf, "@ set UPDATED %n", &pos) >= 0 && pos > 0)
	{
	  strncpy (lightning_updated, linebuf + pos,
		   sizeof (lightning_updated) - 1);
	};
      if (sscanf (linebuf, "@ set VERSION %n", &pos) >= 0 && pos > 0)
	{
	  strncpy (lightning_version, linebuf + pos,
		   sizeof (lightning_version) - 1);
	};
      /// any other lines is ignored
    }
  while (!feof (f));
  if (!lightning_updated[0])
    return __LINE__;
  if (!lightning_version[0])
    return __LINE__;
  return 0;
}				/* end of parse_lightning_version */


/* the generate_lightning_version emits a C file and gives 0 on success */
int
generate_lightning_version (FILE *f, const char *path)
{
  int major = -1;
  int minor = -1;
  int patchlevel = -1;
  if (!f)
    return __LINE__;
  if (!path)
    return __LINE__;
  if (!lightning_updated[0])
    return __LINE__;
  if (!lightning_version[0])
    return __LINE__;
  int p = -1;
  if (sscanf (lightning_version, "%d.%d.%d %n",
	      &major, &minor, &patchlevel, &p) < 3)
    return __LINE__;
  if (fprintf (f, "/// generated file %s for GNU lightning\n", path) <= 0)
    return __LINE__;
  if (fputs ("/// SPDX-License-Identifier: GPL-3.0-or-later\n", f) < 0)
    return __LINE__;
  if (fprintf (f, "int jit_lightning_major(void) { return %d; }\n", major) <
      0)
    return __LINE__;
  if (fprintf (f, "int jit_lightning_major(void) { return %d; }\n", minor) <
      0)
    return __LINE__;
  if (fprintf
      (f, "int jit_lightning_patchlevel(void) { return %d; }\n",
       patchlevel) < 0)
    return __LINE__;
  if (fprintf (f, "/// end of generated file %s for GNU lightning\n", path) <=
      0)
    return __LINE__;
  if (fflush (f) != 0)
    return __LINE__;
  return 0;
}				/* end of generate_lightning_version */

int
main (int argc, char **argv)
{
  int err = -1;
  prog_name = argv[0];
  const char *generatedpath = "_lightning-version.c";
  if (argc < 1)
    {
      fprintf (stderr,
	       "%s requires one argument (maybe --help or --version, often a textual file path\n",
	       prog_name);
      exit (EXIT_FAILURE);
    };
  if (!strcmp (argv[1], "--help"))
    {
      printf ("%s usage: one path argument or --help or --version\n",
	      prog_name);
      printf
	("%s is a utility related to lists.gnu.org/archive/html/lightning/2026-06/msg00010.html\n",
	 prog_name);
      fflush (NULL);
      exit (EXIT_SUCCESS);
    };
  if (!strcmp (argv[1], "--version"))
    {
      printf ("%s is a utility for GNU lightning\n"
	      " related to lists.gnu.org/archive/html/lightning/2026-06/msg00010.html\n",
	      prog_name);
      printf ("%s was compiled on %s at %s\n", prog_name, __DATE__, __TIME__);
      printf
	("the source code is on github.com/bstarynk/misc-basile/parse-lightning-version-texi.c\n");
      printf ("%s is FREE SOFTWARE, GPL licensed, without warranty\n",
	      prog_name);
      fflush (NULL);
      exit (EXIT_SUCCESS);
    };
  FILE *versionf = fopen (argv[1], "r");
  if (!versionf)
    {
      fprintf (stderr, "%s failed to open textual file %s: %s\n", prog_name,
	       argv[1], strerror (errno));
      exit (EXIT_FAILURE);
    };
  errno = 0;
  err = parse_lightning_version (versionf);
  if (err)
    {
      fprintf (stderr,
	       "%s failed to parse GNU lightning version file %s (%d) errno:%d:%s\n",
	       prog_name, argv[1], err, errno, strerror (errno));
      exit (EXIT_FAILURE);
    };
  errno = 0;
  FILE *genf = fopen (generatedpath, "w");
  if (!genf)
    {
      fprintf (stderr,
	       "%s failed to open GNU lightning generated code version file %s (%d) errno:%d:%s\n",
	       prog_name, generatedpath, err, errno, strerror (errno));
      exit (EXIT_FAILURE);
    };
  err = generate_lightning_version (genf, generatedpath);
  if (err)
    {
      fprintf (stderr,
	       "%s failed to generate GNU lightning version code file %s (%d) errno:%d:%s\n",
	       prog_name, generatedpath, err, errno, strerror (errno));
      exit (EXIT_FAILURE);
    };
  return 0;
}

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make parse-lightning-version-texi" ;;
 ** End: ;;
 ****************/
