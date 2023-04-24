/*** in github.com/bstarynk/misc-basile/
 * file CryptArithm/neuf+deux=onze/naive0.c
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 *  © Copyright CEA and Basile Starynkevitch 2023
 *
 ****/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>


char *progname;

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
  int neuf =			//
    (a->v[l_N] * 1000) + (a->v[l_E] * 100) + (a->v[l_U] * 10) + (a->v[l_F]);
  // the number DEUX
  int deux =			//
    (a->v[l_D] * 1000) + (a->v[l_E] * 100) + (a->v[l_U] * 10) + (a->v[l_X]);
  if (neuf == deux)
    return false;
  // the number ONZE
  int onze =
    (a->v[l_O] * 1000) + (a->v[l_N] * 100) + (a->v[l_Z] * 10) + (a->v[l_E]);
  if (neuf + deux != onze)
    return false;
  return true;
}				/* end good_solution */


void
print_solution (struct assign_st *a)
{
  if (!a)
    return;
  if (a->magic != MAGIC_ASSIGN)
    return;
#define P(Letter) do {printf("%s = %d\n", #Letter, a->v[l_##Letter]);}while(0)
  P (D);
  P (E);
  P (F);
  P (N);
  P (O);
  P (U);
  P (X);
  P (Z);
  fflush (stdout);
}				/* end print_solution */

int
main (int argc, char **argv)
{
  int64_t cnt = 0;
  int nbsol = 0;
  progname = (argc > 0) ? argv[0] : __FILE__;
  struct assign_st a;
  struct timespec ts_elapsed_start = { 0, 0 };
  struct timespec ts_elapsed_end = { 0, 0 };
  struct timespec ts_cpu_start = { 0, 0 };
  struct timespec ts_cpu_end = { 0, 0 };
  if (clock_gettime (CLOCK_MONOTONIC, &ts_elapsed_start))
    {
      perror ("clock_gettime CLOCK_MONOTONIC start");
      exit (EXIT_FAILURE);
    };
  if (clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &ts_cpu_start))
    {
      perror ("clock_gettime CLOCK_PROCESS_CPUTIME_ID start");
      exit (EXIT_FAILURE);
    };
  memset (&a, 0, sizeof (a));
  for (uint8_t nD = 0; nD <= 9; nD++)
    {
      for (uint8_t nE = 0; nE <= 9; nE++)
	{
	  for (uint8_t nF = 0; nF <= 9; nF++)
	    {
	      for (uint8_t nN = 0; nN <= 9; nN++)
		{
		  for (uint8_t nO = 0; nO <= 9; nO++)
		    {
		      for (uint8_t nU = 0; nU <= 9; nU++)
			{
			  for (uint8_t nX = 0; nX <= 9; nX++)
			    {
			      for (uint8_t nZ = 0; nZ <= 9; nZ++)
				{
				  cnt++;
				  a.magic = MAGIC_ASSIGN;
				  a.v[l_D] = nD;
				  a.v[l_E] = nE;
				  a.v[l_F] = nF;
				  a.v[l_N] = nN;
				  a.v[l_O] = nO;
				  a.v[l_U] = nU;
				  a.v[l_X] = nX;
				  a.v[l_Z] = nZ;
				  if (good_solution (&a))
				    {
				      nbsol++;
				      printf ("\nCNT:%ld SOL#%d:\n", cnt,
					      nbsol);
				      print_solution (&a);
				      putchar ('\n');
				      fflush (stdout);
				    }
				}
			    }
			}
		    }
		}
	    }
	}
    }
  if (clock_gettime (CLOCK_MONOTONIC, &ts_elapsed_end))
    {
      perror ("clock_gettime CLOCK_MONOTONIC end");
      exit (EXIT_FAILURE);
    };
  if (clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &ts_cpu_end))
    {
      perror ("clock_gettime CLOCK_PROCESS_CPUTIME_ID end");
      exit (EXIT_FAILURE);
    };
  printf ("%s found %d solutions in %f elapsed, %f CPU seconds\n",
	  progname, nbsol,
	  (double) (ts_elapsed_end.tv_sec - ts_elapsed_start.tv_sec)
	  + 1.0e-9 * (ts_elapsed_end.tv_nsec - ts_elapsed_start.tv_nsec),
	  (double) (ts_cpu_end.tv_sec - ts_cpu_start.tv_sec)
	  + 1.0e-9 * (ts_cpu_end.tv_nsec - ts_cpu_start.tv_nsec));
  fflush (NULL);
  return 0;
}				/* end main */


/***
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "gcc -Wall -Wextra -O3 -g naive0.c -o naive0" ;;
 ** End: ;;
 **
 ***/
