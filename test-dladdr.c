// file test-dladdr.c from https://github.com/bstarynk/misc-basile/
/// SPDX-License-Identifier: GPLv3+ 
/// Copyright Basile STARYNKEVITCH (C) 2026

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <dlfcn.h>
#include <math.h>
#include <errno.h>

#ifndef MY_GIT
#error missing MY_GIT in compilation command
#endif

const char my_git[] = MY_GIT;

const char *my_prog_name;

void
my_test_dladdr_at (int lin, void *ad)
{
  Dl_info inf;
  memset (&inf, 0, sizeof (inf));
  errno = 0;
  if (dladdr (ad, &inf))
    {
      printf ("%s:%d success dladdr ad=%p\n", __FILE__, lin, ad);
      ///
      if (inf.dli_fname)
	printf (" dli_fname=%s\n", inf.dli_fname);
      else
	printf (" no dli_fname\n");
      ///
      if (inf.dli_fbase)
	printf (" dli_fbase=%p\n", inf.dli_fbase);
      else
	printf (" no dli_fbase\n");
      ///
      if (inf.dli_sname)
	printf (" dli_sname=%s\n", inf.dli_fname);
      else
	printf (" no dli_sname\n");
      ///
      if (inf.dli_saddr)
	printf (" dli_saddr=%p\n", inf.dli_saddr);
      else
	printf (" no dli_saddr\n");
    }
  else
    {
      printf ("%s:%d dladdr %p failed %s\n",
	      __FILE__, lin, ad, strerror (errno));
    };
}

#define MY_TEST_DLADDR(A) do {			\
   puts("MY_TEST_DLADDR(" #A ")::");		\
   my_test_dladdr_at(__LINE__, (void*)(A));	\
} while(0)

int
main (int argc, char **argv)
{
  my_prog_name = argv[0];
  if (argc > 1 && !strcmp (argv[1], "--version"))
    {
      printf ("%s: version git %s compiled %s\n",
	      my_prog_name, my_git, __DATE__ "@" __TIME__);
      printf ("code on github.com/bstarynk/misc-basile/\n");
      printf ("%s: GPLv3+ licensed, without warranty\n"
	      " see www.gnu.org/licenses/\n", __FILE__);
      return 0;
    }
  else if (argc > 1 && !strcmp (argv[1], "--help"))
    {
      printf ("%s: GPLv3+ licensed, without warranty, test dladdr(3)\n"
	      "See github.com/bstarynk/misc-basile/" __FILE__ "\n",
	      my_prog_name);
      return 0;
    };
  printf ("%s: sin @%p\n", my_prog_name, (void *) sin);
  MY_TEST_DLADDR (my_git);
  MY_TEST_DLADDR ((char *) my_git - 128);
  MY_TEST_DLADDR (sin);
  MY_TEST_DLADDR (malloc);
  MY_TEST_DLADDR (my_test_dladdr_at);
  MY_TEST_DLADDR (main);
  MY_TEST_DLADDR (dlclose);
  MY_TEST_DLADDR ((char *) main + 100);
  MY_TEST_DLADDR ((char *) main - 100);
  return 0;
}				/* end main */
