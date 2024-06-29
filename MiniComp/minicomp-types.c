// file misc-basile/MiniComp/minicomp-gccjit.c
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
#include "minicomp.h"

#ifndef GITID
#error GITID should be defined in compilation command
#endif

#ifndef MD5SUM
#error MD5SUM should be defined in compilation command
#endif


const char minicomp_types_gitid[] = GITID;
const char minicomp_types_md5sum[] = MD5SUM;
const char minicomp_types_timestamp[] = __DATE__ "@" __TIME__;
gcc_jit_field *
minicomp_field_of_json (json_t *jcurfield, int rk, int flix)
{
  if (!json_is_object (jcurfield))
    MINICOMP_FATAL ("minicomp_field_of_json rk=%d field#%d is bad %s",
		    rk, flix,
		    json_dumps (jcurfield, JSON_INDENT (1) | JSON_SORT_KEYS));
  json_t *jfldname = json_object_get (jcurfield, "field-name");
  json_t *jfldtype = json_object_get (jcurfield, "field-type");
  if (!json_is_string (jfldname))
    MINICOMP_FATAL
      ("minicomp_field_of_json rk=%d bad field-name for field#%d is bad %s",
       rk, flix, json_dumps (jcurfield, JSON_INDENT (1) | JSON_SORT_KEYS));
  gcc_jit_type *fieldtype = minicomp_type_of_json (jfldtype, rk);
  if (!fieldtype)
    MINICOMP_FATAL
      ("minicomp_field_of_json rk=%d bad field-type for field#%d is bad %s",
       rk, flix, json_dumps (jcurfield, JSON_INDENT (1) | JSON_SORT_KEYS));
  return gcc_jit_context_new_field (minicomp_jitctx,
				    minicomp_jitloc (jcurfield), fieldtype,
				    json_string_value (jfldname));
}				/* end minicomp_field_of_json */


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
      for (int flix = 0; flix < nbfields; flix++)
	{
	  json_t *jcurfield = json_array_get (jfields, flix);
	  fldarr[flix] = minicomp_field_of_json (jcurfield, rk, flix);
	};
      gcc_jit_struct *jitstruct =
	gcc_jit_context_new_struct_type (minicomp_jitctx,
					 minicomp_jitloc (jtype),
					 ctypename, nbfields, fldarr);
      gcc_jit_type *jitopaqtyp = gcc_jit_struct_as_type (jitstruct);
      free (fldarr);
      return jitopaqtyp;
    }
  else if (!strcmp (kind, "union"))
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
      for (int flix = 0; flix < nbfields; flix++)
	{
	  json_t *jcurfield = json_array_get (jfields, flix);
	  fldarr[flix] = minicomp_field_of_json (jcurfield, rk, flix);
	};
      gcc_jit_type *jitunion =
	gcc_jit_context_new_union_type (minicomp_jitctx,
					minicomp_jitloc (jtype),
					ctypename, nbfields, fldarr);
      free (fldarr);
      return jitunion;
    }
#warning incomplete minicomp_add_type
  MINICOMP_FATAL ("incomplete minicomp_add_type rk=%d jtype=%s",
		  rk, json_dumps (jtype, JSON_INDENT (1) | JSON_SORT_KEYS));
}				/* end minicomp_add_type */

gcc_jit_type *
minicomp_type_by_name (const char *tyname)
{
  void *ptr = NULL;
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
  else if (json_is_object (jtype))
    {
    }
  MINICOMP_FATAL ("incomplete minicomp_type_of_json rk=%d jtype=%s",
		  rk, json_dumps (jtype, JSON_INDENT (1) | JSON_SORT_KEYS));
#warning incomplete minicomp_type_of_json
}				/* end minicomp_type_of_json  */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make minicomp-gccjit" ;;
 ** End: ;;
 ****************/

/// end of minicomp-types.c
