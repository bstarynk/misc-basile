/*** in github.com/bstarynk/misc-basile/
 * file CryptArithm/neuf+deux=onze/naive0.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  © Copyright CEA and Basile Starynkevitch 2023
 *
 ****/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

enum letters_en
{
  l_D,
  l_E,
  l_F,
  l_N,
  l_O,
  l_U,
  l_X,
  l_Z,
  NbLetters
};

#define MAGIC_ASSIGN 0x9cb7	/* 40119 */
struct assign_st
{
  uint16_t magic;		/* should be MAGIC_ASSIGN */
  int8_t v[NbLetters];
};


bool
good_solution (struct assign_st *a)
{
  if (!a)
    return false;
  if (a->magic != MAGIC_ASSIGN)
    return false;
  // every v is a digit:
  for (int i = 0; i < NbLetters; i++)
    {
      int8_t cv = a->v[i];
      if (cv < 0 || cv > 9)
	return false;
    };
  // the v-s are all different
  for (int i = 0; i < NbLetters; i++)
    {
      int8_t cv = a->v[i];
      for (int j = i + 1; j < NbLetters; j++)
	if (a->v[j] == cv)
	  return false;
    }
  // the number NEUF
  int neuf = (a->l_N * 1000) + (a->l_E * 100) + (a->l_U * 10) + (a->l_F);
  // the number DEUX
  int deux = (a->l_D * 1000) + (a->l_E * 100) + (a->l_U * 10) + (a->l_X);
  if (neuf == deux)
    return false;
  // the number ONZE
  int onze = (a->l_O * 1000) + (a->l_N * 100) + (a->l_Z * 10) + (a->l_E);
  if (neuf + deux != onze)
    return false;
  return true;
} /* end good_solution */
