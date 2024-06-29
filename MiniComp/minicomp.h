// file misc-basile/MiniComp/minicomp.h
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

#ifndef MINICOMP_HEADER_INCLUDED
#define _GNU_SOURCE 1
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdalign.h>
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

extern const char *minicomp_progname;

extern const char minicomp_gitid[];

extern const char minicomp_md5sum[];

extern const char *minicomp_generated_elf_so;

#define MINICOMP_SIZE_BASENAME 128

extern char minicomp_basename[MINICOMP_SIZE_BASENAME];

extern gcc_jit_context *minicomp_jitctx;

extern bool minicomp_on_32_bits;

extern gcc_jit_object **minicomp_jitobvec;
extern int minicomp_nbjitob;
extern int minicomp_size_jitobvec;

extern GHashTable *minicomp_name2type_hashtable;	// associate strings to gcc_jit_type pointers
extern GHashTable *minicomp_type2name_hashtable;	// associate gcc_jit_type to names

extern volatile const char *minicomp_fatal_file;
extern volatile int minicomp_fatal_line;
extern void minicomp_fatal_stop_at (const char *, int)
  __attribute__((noreturn));
#define MINICOMP_FATAL_AT_BIS(Fil,Lin,Fmt,...) do {			\
    fprintf(stderr, "MINICOMP-GCCJIT FATAL:%s:%d: <%s>\n " Fmt "\n\n",	\
            Fil, Lin, __func__, ##__VA_ARGS__);				\
    minicomp_fatal_stop_at(Fil,Lin); } while(0)


extern json_t *minicomp_json_code_array;

#define MINICOMP_FATAL_AT(Fil,Lin,Fmt,...) MINICOMP_FATAL_AT_BIS(Fil,Lin,Fmt,##__VA_ARGS__)

#define MINICOMP_FATAL(Fmt,...) MINICOMP_FATAL_AT(__FILE__,__LINE__,Fmt,##__VA_ARGS__)

/// implemented in minicomp-gccjit.c
extern bool minicomp_on_32_bits;
extern gcc_jit_location *minicomp_jitloc (json_t * jloc);

/// implemented in minicomp-types.c
extern const char minicomp_types_gitid[];
extern const char minicomp_types_md5sum[];
extern const char minicomp_types_timestamp[];
extern gcc_jit_field *minicomp_field_of_json (json_t * jcurfield, int rk,
					      int flix);
extern gcc_jit_type *minicomp_add_type (json_t * jtype, const char *kind,
					int rk);
extern gcc_jit_type *minicomp_type_by_name (const char *tyname);
extern gcc_jit_type *minicomp_type_of_json (json_t * jtype, int rk);

/// implemented in minicomp-stmts.c
extern const char minicomp_stmts_gitid[];
extern const char minicomp_stmts_md5sum[];
extern const char minicomp_stmts_timestamp[];

/// implemented in minicomp-exprs.c
extern const char minicomp_exprs_gitid[];
extern const char minicomp_exprs_md5sum[];
extern const char minicomp_exprs_timestamp[];
extern gcc_jit_rvalue *minicomp_expr_of_json (json_t * jexpr, int rk);

#endif /*MINICOMP_HEADER_INCLUDED */
/// end of header file minicomp.h
