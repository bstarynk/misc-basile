// file test-dladdr.c from https://github.com/bstarynk/misc-basile/
/// SPDX-License-Identifier: GPLv3+ 
/// Copyright Basile STARYNKEVITCH (C) 2026
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <dlfcn.h>
#include <math.h>

#ifndef MY_GIT
#error missing MY_GIT in compilation command
#endif

const char my_git[]=MY_GIT;

const char*my_prog_name;
int
main(int argc, char**argv)
{
  my_prog_name = argv[0];
  if (argc>1 && !strcmp(argv[1], "--version"))
    {
      printf("%s: version git %s compiled %s\n",
	     my_prog_name, my_git, __DATE__ "@" __TIME__);
      printf("code on github.com/bstarynk/misc-basile/\n");
      printf("%s: GPLv3+ licensed, without warranty\n"
	     " see www.gnu.org/licenses/\n", __FILE__);
      return 0;
    }
  else if (argc>1 && !strcmp(argv[1], "--help"))
    {
      printf("%s: GPLv3+ licensed, without warranty, test dladdr(3)\n"
	     "See github.com/bstarynk/misc-basile/" __FILE__ "\n",
	     my_prog_name);
      return 0;
    };
  printf("%s: sin @%p\n", my_prog_name, (void*)sin);
} /* end main */
