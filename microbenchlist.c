// file microbenchlist.c

/* Copyright © 2017 Basile Starynkevitch
   <basile@starynkevitch.net>

This microbenchlist is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

microbenchlist is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

*/
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define PERIOD 0x40000

struct node_st
{
  long v;
  struct node_st *next;
};

extern void printptrint (const char *str, int lin, void *ptr, int cnt);

long
sumlist0 (struct node_st *n)
{
  long s = 0;
  while (n)
    {
      s += n->v;
      n = n->next;
    };
  return s;
}


long
sumlist1 (struct node_st *n)
{
  long s = 0;
  int c = 0;
  while (n)
    {
      s += n->v;
      c++;
      if ((c % PERIOD) == 0)
	printptrint ("n", __LINE__, n, c);
      n = n->next;
    };
  return s;
}				/* end sumlist1 */

long
sumlist2 (struct node_st *n)
{
  struct
  {
    struct node_st *nn;
    long ss;
    int cc;
  } _;
  _.nn = n;
  _.ss = 0;
  _.cc = 0;
  while (_.nn)
    {
      _.ss += _.nn->v;
      _.cc++;
      if ((_.cc % PERIOD) == 0)
	printptrint ("nn", __LINE__, _.nn, _.cc);
      _.nn = _.nn->next;
    };
  return _.ss;
}				/* end sumlist2 */


long
sumlist3 (struct node_st *n)
{
  struct
  {
    struct node_st *nn;
    long ss;
    int cc;
  } _;
  _.nn = n;
  _.ss = 0;
  _.cc = 0;
  while (_.nn)
    {
      _.ss += _.nn->v;
      _.cc++;
      _.nn = _.nn->next;
    };
  return _.ss;
}				/* end sumlist3 */

long
sumlist4 (struct node_st *n)
{
  struct
  {
    struct node_st *nn;
    long ss;
    int cc;
  } _;
  _.nn = n;
  _.ss = 0;
  _.cc = 0;
  while (_.nn)
    {
      _.ss += _.nn->v;
      _.cc++;
      if ((_.cc % PERIOD) == 0)
	printptrint ("loc4", __LINE__, &_, _.cc);
      _.nn = _.nn->next;
    };
  return _.ss;
}				/* end sumlist4 */

long
sumlist5 (struct node_st *n)
{
  long s = 0;
  int c = 0;
  struct
  {
    struct node_st *nn;
    long ss;
    int cc;
  } _;

  _.nn = 0;
  _.ss = 0;
  _.cc = 0;
  while (n)
    {
      s += n->v;
      n = n->next;
      c++;
      if ((c % PERIOD) == 0)
	{
	  _.nn = n;
	  _.ss = s;
	  _.cc = c;
	  printptrint ("loc5", __LINE__, &_, c);
	}
    };
  return s;
}				/* end sumlist5 */


int
main (int argc, char **argv)
{
  long k = (argc > 1) ? (1024 * atoi (argv[1])) : (1024 * 1024);
  if (k < 1000)
    k = 1000;
  printf ("start k=%ld\n", k);
  srandom ((int) time (NULL) + (int) getpid ());
  struct node_st *root = NULL;
  struct node_st *n = NULL;
  for (long i = 0; i < k; i++)
    {
      struct node_st *p = n;
      n = malloc (sizeof (struct node_st));
      if (!n)
	{
	  perror ("malloc");
	  exit (EXIT_FAILURE);
	};
      n->v = (random () & 0x3ffff);
      n->next = p;
    }
  root = n;
  n = NULL;
  clock_t cl0 = clock ();
  printf ("created %ld nodes in %.4f sec (%.4f µs/node) root@%p\n",
	  k, cl0 * 1.0e-6, (cl0 * 1.0) / (double) k, root);
  long l0 = sumlist0 (root);
  clock_t cl1 = clock ();
  printf ("counted l0=%ld in %.4f sec (%.4f µs/node)\n",
	  l0, (cl1 - cl0) * 1.0e-6, ((cl1 - cl0) * 1.0) / (double) k);
  long l1 = sumlist1 (root);
  clock_t cl2 = clock ();
  printf ("counted l1=%ld in %.4f sec (%.4f µs/node)\n",
	  l1, (cl2 - cl1) * 1.0e-6, ((cl2 - cl1) * 1.0) / (double) k);
  long l2 = sumlist2 (root);
  clock_t cl3 = clock ();
  printf ("counted l2=%ld in %.4f sec (%.4f µs/node)\n",
	  l2, (cl3 - cl2) * 1.0e-6, ((cl3 - cl2) * 1.0) / (double) k);
  long l3 = sumlist3 (root);
  clock_t cl4 = clock ();
  printf ("counted l3=%ld in %.4f sec (%.4f µs/node)\n",
	  l3, (cl4 - cl3) * 1.0e-6, ((cl4 - cl3) * 1.0) / (double) k);
  long l4 = sumlist4 (root);
  clock_t cl5 = clock ();
  printf ("counted l4=%ld in %.4f sec (%.4f µs/node)\n",
	  l4, (cl5 - cl4) * 1.0e-6, ((cl5 - cl4) * 1.0) / (double) k);
  long l5 = sumlist5 (root);
  clock_t cl6 = clock ();
  printf ("counted l5=%ld in %.4f sec (%.4f µs/node)\n",
	  l5, (cl6 - cl5) * 1.0e-6, ((cl6 - cl5) * 1.0) / (double) k);
  ///
  cl0 = clock ();
  l0 = sumlist0 (root);
  cl1 = clock ();
  printf ("recounted l0=%ld in %.4f sec (%.4f µs/node)\n",
	  l0, (cl1 - cl0) * 1.0e-6, ((cl1 - cl0) * 1.0) / (double) k);
  l1 = sumlist1 (root);
  cl2 = clock ();
  printf ("recounted l1=%ld in %.4f sec (%.4f µs/node)\n",
	  l1, (cl2 - cl1) * 1.0e-6, ((cl2 - cl1) * 1.0) / (double) k);
  l2 = sumlist2 (root);
  cl3 = clock ();
  printf ("recounted l2=%ld in %.4f sec (%.4f µs/node)\n",
	  l2, (cl3 - cl2) * 1.0e-6, ((cl3 - cl2) * 1.0) / (double) k);
  l3 = sumlist3 (root);
  cl4 = clock ();
  printf ("recounted l3=%ld in %.4f sec (%.4f µs/node)\n",
	  l3, (cl4 - cl3) * 1.0e-6, ((cl4 - cl3) * 1.0) / (double) k);
  l4 = sumlist4 (root);
  cl5 = clock ();
  printf ("recounted l4=%ld in %.4f sec (%.4f µs/node)\n",
	  l4, (cl5 - cl4) * 1.0e-6, ((cl5 - cl4) * 1.0) / (double) k);
  l5 = sumlist5 (root);
  cl6 = clock ();
  printf ("recounted l5=%ld in %.4f sec (%.4f µs/node)\n",
	  l5, (cl6 - cl5) * 1.0e-6, ((cl6 - cl5) * 1.0) / (double) k);
  return 0;
}				/* end main */

void
printptrint (const char *str, int lin, void *ptr, int cnt)
{
  printf ("! %s l:%d: %p #%d\n", str, lin, ptr, cnt);
}
