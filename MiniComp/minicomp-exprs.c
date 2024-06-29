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


const char minicomp_exprs_gitid[] = GITID;
const char minicomp_exprs_md5sum[] = MD5SUM;
const char minicomp_exprs_timestamp[] = __DATE__ "@" __TIME__;

gcc_jit_rvalue *
minicomp_expr_of_json (json_t *jexpr, int rk)
{
  if (!jexpr)
    MINICOMP_FATAL ("minicomp_expr_of_json null json_t at rank %d", rk);
  if (json_is_null (jexpr))
    {
      return gcc_jit_context_null (minicomp_jitctx,
				   gcc_jit_context_get_type (minicomp_jitctx,
							     GCC_JIT_TYPE_VOID_PTR));
    }
  else if (json_is_integer (jexpr))
    {
      long expval = json_integer_value (jexpr);
      if (expval == 0)
	{
	  if (minicomp_on_32_bits)
	    return gcc_jit_context_zero (minicomp_jitctx,
					 gcc_jit_context_get_type
					 (minicomp_jitctx, GCC_JIT_TYPE_INT));
	  else
	    return gcc_jit_context_zero (minicomp_jitctx,
					 gcc_jit_context_get_type
					 (minicomp_jitctx,
					  GCC_JIT_TYPE_LONG));
	}
      else if (expval == 1)
	{
	  if (minicomp_on_32_bits)
	    return gcc_jit_context_one (minicomp_jitctx,
					gcc_jit_context_get_type
					(minicomp_jitctx, GCC_JIT_TYPE_INT));
	  else
	    return gcc_jit_context_one (minicomp_jitctx,
					gcc_jit_context_get_type
					(minicomp_jitctx, GCC_JIT_TYPE_LONG));
	}
      else
	{
	  if (minicomp_on_32_bits)
	    return gcc_jit_context_new_rvalue_from_int (minicomp_jitctx,
							gcc_jit_context_get_type
							(minicomp_jitctx,
							 GCC_JIT_TYPE_INT),
							(int) expval);
	  else
	    return gcc_jit_context_new_rvalue_from_long (minicomp_jitctx,
							 gcc_jit_context_get_type
							 (minicomp_jitctx,
							  GCC_JIT_TYPE_LONG),
							 (int) expval);
	}
    }
#warning minicomp_expr_of_json incomplete
  MINICOMP_FATAL ("minicomp_expr_of_json incomplete rk#%d jexpr %s",
		  rk, json_dumps (jexpr, JSON_INDENT (1) | JSON_SORT_KEYS));
}



/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make minicomp-gccjit" ;;
 ** End: ;;
 ****************/

/// end of minicomp-exprs.c
