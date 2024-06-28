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
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "libgccjit.h"
#include "jansson.h"
#include "glib.h"

#ifndef GITID
#error GITID should be defined in compilation command
#endif

#ifndef MD5SUM
#error MD5SUM should be definied in compilation command
#endif

const char *minicomp_progname;

const char minicomp_gitid[] = GITID;

const char minicomp_md5sum[] = MD5SUM;

const char *minicomp_generated_elf_so;
char minicomp_basename[128];

gcc_jit_context *minicomp_jitctx;

gcc_jit_object **minicomp_jitobvec;
int minicomp_nbjitob;
int minicomp_size_jitobvec;

GHashTable *minicomp_name2type_hashtable;	// associate strings to gcc_jit_type pointers
GHashTable *minicomp_type2name_hashtable;	// associate gcc_jit_type to names

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
  printf ("%s version gitid %s built %s\n",
	  minicomp_progname, minicomp_gitid, __DATE__ "@" __TIME__);
  printf ("\t md5sum %s\n", minicomp_md5sum);
  printf ("\t using libgccjit %d.%d.%d\n",
	  gcc_jit_version_major (),
	  gcc_jit_version_minor (), gcc_jit_version_patchlevel ());
  printf ("\t using jansson library for JSON %s\n", jansson_version_str ());
  printf ("\t using Glib %d.%d.%d checked %s at %s:%d\n",
	  glib_major_version, glib_minor_version, glib_micro_version,
	  glib_check_version (2, 79, 1) ? : "good", __FILE__, __LINE__);
}				/* end minicomp_show_version */

void
minicomp_show_help (void)
{
  printf ("%s usage:\n", minicomp_progname);
  printf ("\t --help                             #show this help\n");
  printf ("\t --version                          #show version info\n");
  printf ("\t -O[0-2g]                           #GCC optimization\n");
  printf
    ("\t -o <filename> | --output=ELFFILE   #generated ELF shared object\n");
}				/* end minicomp_show_help */

void
minicomp_process_json (FILE *fil, const char *name)
{
  json_error_t jerr = { };
  json_t *js = json_loadf (fil,
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

gcc_jit_location *
minicomp_jitloc (json_t *jloc)
{
  if (json_is_object (jloc))
    {
      json_t *jfil = json_object_get (jloc, "fil");
      json_t *jlin = json_object_get (jloc, "lin");
      json_t *jcol = json_object_get (jloc, "col");
      if (json_is_string (jfil) && json_is_integer (jlin))
	{
	  if (json_is_integer (jcol))
	    return gcc_jit_context_new_location (minicomp_jitctx,
						 json_string_value (jfil),
						 json_integer_value (jlin),
						 json_integer_value (jcol));
	  else
	    return gcc_jit_context_new_location (minicomp_jitctx,
						 json_string_value (jfil),
						 json_integer_value (jlin),
						 0);
	}
    }
  return NULL;
}				/* end minicomp_jitloc */

gcc_jit_type *minicomp_type_of_json (json_t * jtype, int rk);

gcc_jit_type *
minicomp_add_type (json_t *jtype, const char *kind, int rk)
{
  const char *ctypename = NULL;
  char tynamebuf[256];
  memset (tynamebuf, 0, 16 + sizeof (tynamebuf) / 4);
  json_t *jname = json_object_get (jtype, "name");
  if (json_is_string (jname))
    ctypename = json_string_value (jname);
  else
    memset (tynamebuf, 0, sizeof (tynamebuf));
  if (!strcmp (kind, "opaque"))
    {
      if (!ctypename)
	{
	  snprintf (tynamebuf, sizeof (tynamebuf),
		    "__opaque_%d_%s", rk, minicomp_basename);
	  ctypename = tynamebuf;
	  gcc_jit_struct *jitopaqstruct =
	    gcc_jit_context_new_opaque_struct (minicomp_jitctx,
					       minicomp_jitloc (jtype),
					       ctypename);
	  gcc_jit_type *jitopaqtyp = gcc_jit_struct_as_type (jitopaqstruct);
	  return jitopaqtyp;
	}
    }
  else if (!strcmp (kind, "struct"))
    {
      json_t *jfields = json_object_get (jtype, "fields");
      if (!json_is_array (jfields))
	MINICOMP_FATAL ("minicomp_add_type rk=%d missing fields in jtype=%s",
			rk,
			json_dumps (jtype, JSON_INDENT (1) | JSON_SORT_KEYS));
      int nbfields = json_array_size (jfields);
      gcc_jit_field **fldarr =
	calloc (((nbfields + 1) | 7), sizeof (gcc_jit_field *));
      if (!fldarr)
	MINICOMP_FATAL
	  ("minicomp_add_type rk=%d out of memory for %d fields (%s)", rk,
	   nbfields, strerror (errno));
#warning incomplete minicomp_add_type struct
    }
#warning incomplete minicomp_add_type
  MINICOMP_FATAL ("incomplete minicomp_add_type rk=%d jtype=%s",
		  rk, json_dumps (jtype, JSON_INDENT (1) | JSON_SORT_KEYS));
}				/* end minicomp_add_type */

gcc_jit_type *
minicomp_type_by_name (const char *tyname)
{
  void *ptr = NULL;
  gcc_jit_type *res = NULL;
  if (false)
    return NULL;
  else if (!strcmp (tyname, "void"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_VOID);
  else if (!strcmp (tyname, "pointer"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_VOID_PTR);
  else if (!strcmp (tyname, "bool"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_BOOL);
  else if (!strcmp (tyname, "char"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_CHAR);
  else if (!strcmp (tyname, "schar"))
    return gcc_jit_context_get_type (minicomp_jitctx,
				     GCC_JIT_TYPE_SIGNED_CHAR);
  else if (!strcmp (tyname, "uchar"))
    return gcc_jit_context_get_type (minicomp_jitctx,
				     GCC_JIT_TYPE_UNSIGNED_CHAR);
  else if (!strcmp (tyname, "short"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_SHORT);
  else if (!strcmp (tyname, "ushort"))
    return gcc_jit_context_get_type (minicomp_jitctx,
				     GCC_JIT_TYPE_UNSIGNED_SHORT);
  else if (!strcmp (tyname, "int"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_INT);
  else if (!strcmp (tyname, "uint"))
    return gcc_jit_context_get_type (minicomp_jitctx,
				     GCC_JIT_TYPE_UNSIGNED_INT);
  else if (!strcmp (tyname, "long"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_LONG);
  else if (!strcmp (tyname, "ulong"))
    return gcc_jit_context_get_type (minicomp_jitctx,
				     GCC_JIT_TYPE_UNSIGNED_LONG);
  else if (!strcmp (tyname, "longlong"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_LONG_LONG);
  else if (!strcmp (tyname, "ulonglong"))
    return gcc_jit_context_get_type (minicomp_jitctx,
				     GCC_JIT_TYPE_UNSIGNED_LONG_LONG);
  else if (!strcmp (tyname, "uint8"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_UINT8_T);
  else if (!strcmp (tyname, "uint16"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_UINT16_T);
  else if (!strcmp (tyname, "uint32"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_UINT32_T);
  else if (!strcmp (tyname, "uint64"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_UINT64_T);
  else if (!strcmp (tyname, "uint128"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_UINT128_T);
  else if (!strcmp (tyname, "int8"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_INT8_T);
  else if (!strcmp (tyname, "int16"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_INT16_T);
  else if (!strcmp (tyname, "int32"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_INT32_T);
  else if (!strcmp (tyname, "int64"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_INT64_T);
  else if (!strcmp (tyname, "int128"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_INT128_T);
  else if (!strcmp (tyname, "float"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_FLOAT);
  else if (!strcmp (tyname, "double"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_DOUBLE);
  else if (!strcmp (tyname, "long_double"))
    return gcc_jit_context_get_type (minicomp_jitctx,
				     GCC_JIT_TYPE_LONG_DOUBLE);
  else if (!strcmp (tyname, "constcharptr"))
    return gcc_jit_context_get_type (minicomp_jitctx,
				     GCC_JIT_TYPE_CONST_CHAR_PTR);
  else if (!strcmp (tyname, "size_t"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_SIZE_T);
  else if (!strcmp (tyname, "FILEptr"))
    return gcc_jit_context_get_type (minicomp_jitctx, GCC_JIT_TYPE_FILE_PTR);
  else if (!strcmp (tyname, "complex_float"))
    return gcc_jit_context_get_type (minicomp_jitctx,
				     GCC_JIT_TYPE_COMPLEX_FLOAT);
  else if (!strcmp (tyname, "complex_double"))
    return gcc_jit_context_get_type (minicomp_jitctx,
				     GCC_JIT_TYPE_COMPLEX_DOUBLE);
  else if (!strcmp (tyname, "complex_long_double"))
    return gcc_jit_context_get_type (minicomp_jitctx,
				     GCC_JIT_TYPE_COMPLEX_LONG_DOUBLE);
  else if ((ptr = g_hash_table_lookup (minicomp_name2type_hashtable, tyname))
	   != NULL)
    return (gcc_jit_type *) ptr;
  return NULL;
}				/* end minicomp_type_by_name */

gcc_jit_type *
minicomp_type_of_json (json_t *jtype, int rk)
{
  if (!jtype)
    MINICOMP_FATAL ("null jtype rk#%d", rk);
  if (json_is_string (jtype))
    {
      gcc_jit_type *ty = minicomp_type_by_name (json_string_value (jtype));
      if (!ty)
	MINICOMP_FATAL ("no jtype named %s rk#%d", json_string_value (jtype),
			rk);
      return ty;
    }
  MINICOMP_FATAL ("incomplete minicomp_type_of_json rk=%d jtype=%s",
		  rk, json_dumps (jtype, JSON_INDENT (1) | JSON_SORT_KEYS));
#warning incomplete minicomp_type_of_json
}				/* end minicomp_type_of_json  */

void
minicomp_first_pass (void)
{
  minicomp_name2type_hashtable = g_hash_table_new (g_str_hash, g_str_equal);
  minicomp_type2name_hashtable = g_hash_table_new (NULL, NULL);
  int jcln = json_array_size (minicomp_json_code_array);
  /// we first need to find an output file in the JSON components if it was not given on the command line
  {
    for (int ix = 0; ix < jcln; ix++)
      {
	json_t *jcomp = json_array_get (minicomp_json_code_array, ix);
	json_t *jgencod = NULL;
	json_t *jtypestr = NULL;
	if (json_is_object (jcomp))
	  {
	    if (!minicomp_generated_elf_so
		&& (jgencod = json_object_get (jcomp, "generated-code"))
		&& json_is_string (jgencod))
	      {
		minicomp_generated_elf_so = json_string_value (jgencod);
		break;
	      }
	    else if ((jtypestr = json_object_get (jcomp, "type"))
		     && json_is_string (jtypestr))
	      minicomp_add_type (jcomp, json_string_value (jtypestr), ix);
	  }
      }
  }
  /// allocate the vector of gcc_jit_object-s
  minicomp_size_jitobvec = 1 + ((jcln + 7) | 0xf);
  minicomp_jitobvec =
    calloc (minicomp_size_jitobvec, sizeof (gcc_jit_object *));
  if (!minicomp_jitobvec)
    MINICOMP_FATAL ("cannot allocate vector for %d gccjit objects (%s)",
		    minicomp_size_jitobvec, strerror (errno));
  minicomp_nbjitob = 0;
}				/* end minicomp_first_pass */

void
minicomp_set_basename (void)
{
  char *dp = minicomp_basename;
  if (!minicomp_generated_elf_so)
    MINICOMP_FATAL
      ("cannot set basename without knowing generated ELF shared object");
  const char *sp =
    strrchr (minicomp_generated_elf_so, '/') ? : minicomp_generated_elf_so;
  if (*sp == '/')
    sp++;
  if (minicomp_basename[0])
    return;
  while (*sp != '.'
	 && dp < minicomp_basename + sizeof (minicomp_basename) - 1)
    {
      if (isalnum (*sp))
	*(dp++) = *sp;
      else
	*(dp++) = '_';
      sp++;
    };
  if (!minicomp_basename[0])
    snprintf (minicomp_basename, sizeof (minicomp_basename),
	      "_minicomp_gccjit%.9s_p%d_", minicomp_gitid, (int) getpid ());
}				/* end minicomp_set_basename */

void
minicomp_generate_code (void)
{
  gcc_jit_context_set_bool_option (minicomp_jitctx,
				   GCC_JIT_BOOL_OPTION_DEBUGINFO, true);
  gcc_jit_context_add_driver_option (minicomp_jitctx, "-fPIC");	/// position independent code
#warning unimplemented minicomp_generate_code
  MINICOMP_FATAL ("unimplemented minicomp_generate_code");
}				/* end minicomp_generate_code */

void
minicomp_handle_arguments (int argc, char **argv)
{
  int aix = 0;
  char argbuf[256];
  memset (argbuf, 0, sizeof (argbuf));
  for (aix = 1; aix < argc; aix++)
    {
      int c = -1;
      memset (argbuf, 0, sizeof (argbuf));
      const char *curarg = argv[aix];
      if (curarg[0] == '-')
	{
	  if (curarg[1] == 'O'	//letter upper-case O, for optimization, eg -O2
	      && (isdigit (curarg[2]) || curarg[2] == 'g'
		  || curarg[2] == 's'))
	    {
	      if (isdigit (curarg[2]))
		gcc_jit_context_set_int_option (minicomp_jitctx,
						GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL,
						atoi (curarg + 2));
	      else
		gcc_jit_context_add_driver_option (minicomp_jitctx, curarg);
	    }
	  else if (curarg[1] == 'f')
	    gcc_jit_context_add_driver_option (minicomp_jitctx, curarg);
	  else if (!strcmp (curarg, "-o") && aix + 1 < argc)
	    {
	      if (minicomp_generated_elf_so)
		MINICOMP_FATAL ("generated output file already is %s,"
				" rejecting arg#%d %s",
				minicomp_generated_elf_so, aix + 1,
				argv[aix + 1]);
	      minicomp_generated_elf_so = argv[aix + 1];
	      aix++;
	    }
	  else if (sscanf (curarg, "--output=%n", &c) >= 0 && curarg[c])
	    {
	      const char *out = curarg + strlen ("--output=");
	      if (!out)
		MINICOMP_FATAL ("missing output file after --output=");
	      if (minicomp_generated_elf_so)
		MINICOMP_FATAL ("generated output file already is %s,"
				" rejecting arg#%d %s",
				minicomp_generated_elf_so, aix, curarg);
	      minicomp_generated_elf_so = out;
	    }
	  else if (sscanf (curarg, "--base=%100[A-Za-z0-9_]%n", argbuf, &c) >
		   0 && argbuf[0] && c > 0)
	    {
	      if (minicomp_basename[0])
		MINICOMP_FATAL
		  ("already given basename %s rejecting arg#%d %s",
		   minicomp_basename, aix, curarg);
	      argbuf[120] = (char) 0;
	      strncpy (minicomp_basename, argbuf,
		       sizeof (minicomp_basename) - 1);
	    }
#warning incomplete minicomp_handle_arguments
	  else
	    MINICOMP_FATAL
	      ("unexpected minicomp_handle_arguments curarg#%d=%s", aix,
	       curarg);
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
	    MINICOMP_FATAL ("pclose input JSON pipe %s failed returning %d",
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
  if (!minicomp_basename[0])
    minicomp_set_basename ();
  minicomp_first_pass ();
  minicomp_generate_code ();
  json_decref (minicomp_json_code_array);
  minicomp_json_code_array = NULL;
  g_hash_table_destroy (minicomp_type2name_hashtable);
  g_hash_table_destroy (minicomp_name2type_hashtable);
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
