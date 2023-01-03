/***
 *  file manydl.c from https://github.com/bstarynk/misc-basile/
 *  SPDX-License-Identifier: MIT
 *  
 *  generating lots of C functions, and dynamically compiling and loading them
 *  this is completely useless, except for testing & benchmarking 
 *  
 *  © Copyright Basile Starynkevitch 2004- 2023
 *
 *  This was released up to december 18th 2022 under GPLv3+ license.
 *  On Dec 19, 2022 relicensed under MIT licence
 *
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE manydl.c SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ***/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <dlfcn.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/utsname.h>

#ifndef MANYDL_GIT
#error missing MANYDL_GIT string as preprocessing flag
#endif /*MANYDL_GIT */

const char manydl_git[] = MANYDL_GIT;

#define NAME_BUFLEN 32

int maxcnt = 100;		/* number of generated plugins */
#define MINIMAL_COUNT 10
int meansize = 300;		/* mean size of generated C file */
#define MINIMAL_SIZE 15
int makenbjobs = 4;
#define MINIMAL_NBJOBS 3
#define MAXIMAL_NBJOBS 50

bool verbose = false;

bool didcleanup = false;

// when fakerun is true, don't compile or dlopen the plugins
bool fakerun = false;

// a potential script to run on C code
const char *terminatingscript = NULL;

long random_seed;

const char *pluginsuffix = ".so";

const char *makeprog = "/usr/bin/make";

extern void show_help (void);
extern void show_version (void);

typedef int (*funptr_t) (int, int);	/* type of function pointers */

funptr_t *funarr = NULL;
char **namarr = NULL;		/* array of names */
void **hdlarr = NULL;		/* array of dlopen handles */

char myhostname[64];
char *progname;
#define INDENT_PROGRAM "/usr/bin/indent"

double start_elapsed_clock = NAN, start_cpu_clock = NAN;
double generate_elapsed_clock = NAN, generate_cpu_clock = NAN;
double compile_elapsed_clock = NAN, compile_cpu_clock = NAN;
double dlopen_elapsed_clock = NAN, dlopen_cpu_clock = NAN;
double compute_elapsed_clock = NAN, compute_cpu_clock = NAN;
double terminating_elapsed_clock = NAN, terminating_cpu_clock = NAN;

extern long dynstep;
extern void say_fun_a_b_c_d (const char *fun, int a, int b, int c, int d);

/* dynstep, tab & say_fun_a_b_c_d are used in generated files */
long dynstep;			/* number of executed lines */
#define MAXTAB 256		/* length of tab should be a power of two */
int tab[MAXTAB];		/* a table */

extern int tab[MAXTAB];

extern double my_clock (clockid_t);

double
my_clock (clockid_t clid)
{
  struct timespec ts = { 0, 0 };
  if (clock_gettime (clid, &ts))
    {
      perror ("clock_gettime");
      exit (EXIT_FAILURE);
    };
  return (double) ts.tv_sec + 1.0e-9 * ts.tv_nsec;
}				/* end my_clock */

void				/* printing function for generated files */
say_fun_a_b_c_d (const char *fun, int a, int b, int c, int d)
{
  printf ("<%s> a=%d b=%d c=%d d=%d\n", fun, a, b, c, d);
}

double secpertick;
clock_t firstclock;

#define DICE(N) ((int)(lrand48 () % (N)))


void compute_name_for_index (char name[static NAME_BUFLEN], int ix);


void
compute_name_for_index (char name[static NAME_BUFLEN], int ix)
{
  assert (ix >= 0 && ix < maxcnt);
  memset (name, 0, NAME_BUFLEN);
  if (maxcnt < 1600)
    snprintf (name, NAME_BUFLEN, "genf_%c_%02d",
	      "ABCDEFGHIJKLMNOPQ"[ix % 16], ix / 16);
  else if (maxcnt < 160000)
    snprintf (name, NAME_BUFLEN, "genf_%c_%04d",
	      "ABCDEFGHIJKLMNOPQ"[ix % 16], ix / 16);
  else if (maxcnt < 16000000)
    snprintf (name, NAME_BUFLEN, "genf_%c_%06d",
	      "ABCDEFGHIJKLMNOPQ"[ix % 16], ix / 16);
  else
    snprintf (name, NAME_BUFLEN, "genf_%c_%08d",
	      "ABCDEFGHIJKLMNOPQ"[ix % 16], ix / 16);
}				/* end compute_name_for_index */

/* generate a file containing one single randomly coded function
   (named like the file, ie function foo in foo.c), using the lrand48
   random number generator; return the size */

int
generate_file (const char *name)
{
  char pathsrc[100];
  FILE *f = NULL;
  int l = 0;
  int i = 0;
  int prevjmpix = 0;
#define MAXLAB 32
  enum labstate
  { LAB_NONE = 0, LAB_JUMPED, LAB_DEFINED } lab[MAXLAB];
  memset (pathsrc, 0, sizeof (pathsrc));
  memset (lab, 0, sizeof (lab));
  snprintf (pathsrc, sizeof (pathsrc) - 1, "%s.c", name);
  f = fopen (pathsrc, "w");
  if (!f)
    {
      perror (pathsrc);
      exit (EXIT_FAILURE);
    };
  /* random length of generated function */
  l = meansize / 2 + DICE (meansize);
  fprintf (f, "/* generated file %s length %d meansize %d*/\n", pathsrc, l,
	   meansize);
  fprintf (f, "extern long dynstep;\n" "extern int tab[%d];\n", MAXTAB);
  fprintf (f,
	   "extern void say_fun_a_b_c_d(const char*fun, int a, int b, int c, int d);\n");
  fprintf (f, "const char gentimestamp_%s[] = __DATE__ \"@\" __TIME__;\n",
	   name);
  fprintf (f, "int %s(int a, int b) {\n", name);
  fputs ("  int c=0, d=1, e=2, f=3, g=4, h=5, i=6, j=7, k=8, l=a+b;\n", f);
  fputs ("  long initdynstep = dynstep;\n", f);
#define RANVAR ('a'+DICE(12))
  for (i = 0; i < l; i++)
    {
      switch (DICE (16))
	{
	case 0:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f, " %c = (%c * %d + %c * %d + %d) & 0xffffff;\n",
		   RANVAR, RANVAR, 2 + DICE (8), RANVAR, 3 + 2 * DICE (8),
		   DICE (32));
	  if (DICE (8) == 0)
	    fprintf (f, " dynstep++;\n");
	  break;
	case 1:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f,
		   " %c = (%c * %d + tab[%c & %#x] - %c) & 0xffffff; tab[%d]++;\n",
		   RANVAR, RANVAR, 1 + DICE (16), RANVAR, MAXTAB - 1, RANVAR,
		   DICE (MAXTAB));
	  if (DICE (8) == 0)
	    fprintf (f, " dynstep++;\n");
	  break;
	case 2:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f,
		   " if (%c > %c + %d) %c++; else %c=(tab[%d] + %c) & 0xffffff;\n",
		   RANVAR, RANVAR, 5 + DICE (10), RANVAR, RANVAR,
		   DICE (MAXTAB), RANVAR);
	  if (DICE (8) == 0)
	    fprintf (f, " dynstep++;\n");
	  if (DICE (5) == 0)
	    {
	      char v1 = RANVAR;
	      int nbcas = DICE (10) + 2;
	      fprintf (f, " switch ((%c & 0xfff) %% %d) {\n", v1, nbcas);
	      for (int ca = 0; ca < nbcas; ca++)
		{
		  fprintf (f, "   case %d:\n", ca);
		  fprintf (f, "   %c %c= (%c & 0xfff) + %d;\n",
			   "+-"[DICE (2)],
			   RANVAR, RANVAR, 3 + DICE (15 + nbcas));
		  fprintf (f, "   break;\n");
		}
	      fprintf (f, " } //end switch from %d\n", __LINE__);
	    }
	  break;
	case 3:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f, " tab[%c & %#x] += %d + ((%c - %c) & 0x3ff); %c++;\n",
		   RANVAR, MAXTAB - 1, DICE (8) + 2, RANVAR, RANVAR, RANVAR);
	  if (DICE (8) == 0)
	    fprintf (f, " dynstep++;\n");
	  break;
	case 4:
	  {
	    fprintf (f, "// from %d\n", __LINE__);
	    char lvar = RANVAR;
	    fprintf (f,
		     " while (%c>0) { dynstep++; %c -= (%c/3) + %d; }; %c=%d+%c;\n",
		     lvar, lvar, lvar, DICE (16) + 10, lvar, DICE (8),
		     RANVAR);
	  }
	  break;
	case 5:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f, " %c++, %c++;\n", RANVAR, RANVAR);
	  if (DICE (8) == 0)
	    fprintf (f, " %c++, %c++;\n", RANVAR, RANVAR);
	  if (DICE (16) == 0)
	    fprintf (f, " %c += (%c &0xfffff);\n", RANVAR, RANVAR);
	  if (DICE (16) == 0)
	    fprintf (f, " %c -= (1+ (%c&0x7ffff));\n", RANVAR, RANVAR);
	  if (DICE (8) == 0)
	    {
	      fprintf (f, " %c++, %c++;\n", RANVAR, RANVAR);
	      fprintf (f, " dynstep++;\n");
	    };
	  break;
	case 6:
	  {
	    fprintf (f, "// from %d\n", __LINE__);
	    int labrank = DICE (MAXLAB);
	    fprintf (f, " dynstep+=%d;\n", i - prevjmpix);
	    prevjmpix = i;
	    switch (lab[labrank])
	      {
	      case LAB_NONE:
		fprintf (f, " if (%d*%c<%d+%c) {%c++; goto lab%d;};\n",
			 DICE (8) + 2, RANVAR, DICE (10), RANVAR, RANVAR,
			 labrank);
		lab[labrank] = LAB_JUMPED;
		break;
	      case LAB_JUMPED:
		fprintf (f, "lab%d: %c++;\n", labrank, RANVAR);
		lab[labrank] = LAB_DEFINED;
		break;
	      default:;
	      };
	  }
	  break;
	case 7:
	  {
	    fprintf (f, "// from %d\n", __LINE__);
	    fprintf (f,
		     " %c = (%c / ((%c & 0xffff) + 2) + %c * %d) & 0xffffff;\n",
		     RANVAR, RANVAR, RANVAR, RANVAR, DICE (10) + 3);
	    char v1 = RANVAR;
	    char v2 = RANVAR;
	    while (v1 == v2)
	      v2 = RANVAR;
	    fprintf (f, " if (%c > %c)\n", v1, v2);
	    fprintf (f, "    tab[(%c & 0xfffff) %% %#x]++;\n",
		     RANVAR, MAXTAB - DICE (MAXTAB / 10));
	    if (DICE (100) > 20)
	      fprintf (f, " %c += %d;\n", RANVAR, DICE (12) + 5);
	  }
	  break;
	case 8:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f,
		   " %c = (%d + ((%c << (1+ (%c & 0x1f))) + %c)) & 0xffffff;\n",
		   RANVAR, DICE (32) + 5, RANVAR, RANVAR, RANVAR);
	  if (DICE (100) > 10)
	    fprintf (f,
		     " %c = (%c %% %d) + tab[(%c & 0xfffff) %% %d];\n",
		     RANVAR, RANVAR, (DICE (100) + 10),
		     RANVAR, 2 + DICE (3 * MAXTAB / 4));
	  break;
	case 9:
	  {
	    char v = RANVAR;
	    int labrank = DICE (MAXLAB);
	    fprintf (f, "// from %d\n", __LINE__);
	    fprintf (f, " %c = (%c * %d + %d) & 0xffffff;\n",
		     v, RANVAR, DICE (100) + 7, DICE (200) + 12);
	    fprintf (f,
		     " if (dynstep++ %% %d == (%c & 0x1ff) && dynstep < initdynstep + %d)\n",
		     (DICE (50) + 2), v, 100 + DICE (1000));
	    fprintf (f, "    goto lab%d;\n", labrank);
	    lab[labrank] = LAB_JUMPED;
	  }
	  break;
	case 10:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f, " tab[%d]++, tab[%c & %#x] += (%c & 0xff) + %d;\n",
		   DICE (MAXTAB), RANVAR, MAXTAB - 1, RANVAR, DICE (8) + 2);
	  fprintf (f,
		   " if (tab[%d] %% %d == 0 && dynstep > initdynstep + %d)\n"
		   "     goto end_%s;\n", DICE (MAXTAB), (40 + DICE (50)),
		   meansize + 50 + 20 * DICE (1000), name);
	  break;
	case 11:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f, " %c = %c + %d;\n", RANVAR, RANVAR, (2 + DICE (5)));
	  fprintf (f, " %c = (%c*%d) + (%c>>%d);\n",
		   RANVAR, RANVAR, (5 + DICE (32)), RANVAR, (1 + DICE (4)));
	  fprintf (f,
		   " if (%c > %d)\n"
		   "    %c = %c + (tab[%d] %% %d);\n",
		   RANVAR, DICE (1000) + 100, RANVAR, RANVAR,
		   DICE (MAXTAB / 2), (2 + DICE (1024)));
	  break;
	case 12:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f, " %c = %d;\n", RANVAR, 5 + DICE (20));
	  fprintf (f,		//
		   " if (tab[%d] > %c)\n"
		   "    %c = %c + (tab[%d] & 0xffff);\n",
		   DICE (MAXTAB / 2), RANVAR,
		   RANVAR, RANVAR, DICE (MAXTAB / 2));
	  break;
	case 13:
	  {
	    fprintf (f, "// from %d\n", __LINE__);
	    char fvar = RANVAR;
	    fprintf (f,
		     " for (%c &= %d; %c>0; %c--) {dynstep++;tab[(%c & 0xffff) %% %d] += (1+(%c&0x1f));};\n",
		     fvar, MAXTAB - 1, fvar, fvar, fvar, MAXTAB, RANVAR);
	    if (DICE (5) == 0)
	      fprintf (f, " %c += %d;\n", RANVAR, 5 + DICE (20));
	  }
	  break;
	case 14:
	  {
	    fprintf (f, "// from %d\n", __LINE__);
	    char lvar = RANVAR, rvar = RANVAR;
	    if (lvar != rvar)
	      fprintf (f, " %c = %c;\n", lvar, rvar);
	    else
	      fprintf (f, " %c++;\n", lvar);
	    fprintf (f,
		     " tab[(%c & 0x3ffff) %% %d] %c= ((%c & 0xffff) + %d);\n",
		     RANVAR, (MAXTAB / 2 + DICE (MAXTAB / 3)) % MAXTAB,
		     "+-*/%"[DICE (5)], RANVAR, DICE (64) + 4);
	    fprintf (f, " if (tab[%d] > %c)\n"	//
		     "   %c = (%c * %d) + tab[%d];\n",
		     (2 + DICE (MAXTAB / 3)) % MAXTAB, RANVAR,
		     RANVAR, RANVAR, 5 + DICE (50), DICE (2 * MAXTAB / 3));
	  };
	  break;
	case 15:
	  {
	    fprintf (f, "// from %d\n", __LINE__);
	    int labrank = DICE (MAXLAB);
	    fprintf (f, " %c = %c + %d;\n", RANVAR, RANVAR, 2 + DICE (5));
	    if (DICE (3) == 0)
	      fprintf (f, " %c = (%c + %d) & 0xfffff;\n", RANVAR, RANVAR,
		       9 + DICE (35));
	    if (DICE (8) == 0)
	      fprintf (f, " tab[(%c & 0x1fff) %% %d] += %c;\n", RANVAR,
		       DICE (MAXTAB / 3) + 1, RANVAR);
	    fprintf (f, " dynstep++;\n");
	    fprintf (f, " if (dynstep < initdynstep + %d)\n",
		     DICE (16384) + 10);
	    fprintf (f, "    goto lab%d;\n", labrank);
	    if (lab[labrank] == LAB_NONE)
	      lab[labrank] = LAB_JUMPED;
	    break;
	  };

	};
      fprintf (f, " dynstep+=%d;\n", i - prevjmpix);
    };
  for (i = 0; i < MAXLAB; i++)
    {
      if (lab[i] == LAB_JUMPED)
	fprintf (f, " lab%d:\n %c++;\n", i, RANVAR);
      if (DICE (8) == 0)
	{
	  char v = RANVAR;
	  fprintf (f, " %c = (%c + %d) & 0xfffff;\n", v, v, 10 + DICE (25));
	}
      if (DICE (4) == 0 || i == MAXLAB / 2)
	{
	  fprintf (f, " tab[(%c &0x3fffff) %% %d] += %c;\n", RANVAR, MAXTAB,
		   RANVAR);
	  fprintf (f, " %c += (%c&0xffff);\n", RANVAR, RANVAR);
	  fprintf (f, " dynstep++;\n");
	};
    }
  fprintf (f, " a &= 0xffffff;\n");
  fprintf (f, "end_%s:\n", name);
  fprintf (f, " say_fun_a_b_c_d(\"%s\", a, b, c, d);\n", name);
  fprintf (f, " return a;\n" "} /* end %s of %d instr */\n", name, l);
  fprintf (f, "\n\n\n"
	   "\n/* file %s was generated by " __FILE__ "*/\n", pathsrc);
  fprintf (f, "\n"		//
	   "/****** for Emacs...\n"	//
	   " ** Local Variables: ;;\n"	//
    );
  fprintf (f,			//
	   " ** compile-command: \"make %s%s\" ;;\n"	//
	   " ** End: ;;\n"	//
	   " *******/\n", name, pluginsuffix);
  fflush (f);
  fclose (f);
  if (!access (INDENT_PROGRAM, X_OK))
    {
      char indcmd[128];
      memset (indcmd, 0, sizeof (indcmd));
      snprintf (indcmd, sizeof (indcmd), "%s %s", INDENT_PROGRAM, pathsrc);
      int err = system (indcmd);
      if (err)
	{
	  char cwdbuf[256];
	  memset (cwdbuf, 0, sizeof (cwdbuf));
	  if (!getcwd (cwdbuf, sizeof (cwdbuf) - 2))
	    strcpy (cwdbuf, "./");
	  fprintf (stderr, "%s: failed to run %s (%d) in %s\n", progname,
		   indcmd, err, cwdbuf);
	  exit (EXIT_FAILURE);
	};
    }
  return l;
}				/* end generate_file */



/* return a *static* string containing the self & child CPU times */
const char *
timestring ()
{
  static char timbuf[120];
  struct tms ti;
  clock_t clock;
  memset (&ti, 0, sizeof (ti));
  clock = times (&ti);
  memset (timbuf, 0, sizeof (timbuf));
  snprintf (timbuf, sizeof (timbuf) - 1,
	    "CPU= %.3fu+%.3fs self, %.3fu+%.3fs children; real= %.3f",
	    secpertick * ti.tms_utime, secpertick * ti.tms_stime,
	    secpertick * ti.tms_cutime, secpertick * ti.tms_cstime,
	    secpertick * (clock - firstclock));
  return timbuf;
}				// end timestring 


void
show_help (void)
{
  printf ("%s usage (MIT licensed, no warranty)\n", progname);
  printf ("\t a nearly useless program generating many dlopen-ed plugins\n");
  printf ("\t --version | -V   : shows version information\n");
  printf ("\t --help | -h      : shows this help\n");
  printf ("\t -v               : run verbosely\n");
  printf ("\t -F               : fake run - dont compile or run plugins,\n"
	  "\t                    ... just generate C code.\n");
  printf ("\t -n <count>  : set number of generated plugins,"
	  " default is %d\n", maxcnt);
  printf ("\t -s <mean-size>   : set mean size of generated plugins,"
	  " default is %d\n", meansize);
  printf ("\t -j <job>         : number of jobs, passed to make,"
	  " default is %d\n", makenbjobs);
  printf ("\t -m <maker>       : make program, default is %s\n", makeprog);
  printf
    ("\t -R <randomseed>  : seed passed to srand48, default is unique\n");
  printf ("\t -S <p.suffix>    : plugin suffix, default is %s\n",
	  pluginsuffix);
  printf ("\t -T <script>      : terminating popen-ed script\n");
  printf ("\t                    (could be some external analyzer,\n"
	  "\t                       ... getting names of generated C files)\n");
  printf ("\t -C               : clean up the mess (old genf_* files)\n");
}				/* end of show_help */

void
show_version (void)
{
  printf ("%s git %s on %s at built %s\n", progname, manydl_git, myhostname,
	  __DATE__);
  exit (EXIT_SUCCESS);
}				/* end of show_version */


void
cleanup_the_mess (void)
{
  char **oldnamarr = NULL;
  char cwdbuf[256];
  int nbclean = 0;
  int oldnamsiz = ((2 * maxcnt + 40) | 0x3f) + 1;
  struct dirent *curent = NULL;
  double startelapsedclock = my_clock (CLOCK_MONOTONIC);
  double startcpuclock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  oldnamarr = calloc (oldnamsiz, sizeof (char *));
  if (!oldnamarr)
    {
      fprintf (stderr,
	       "%s: failed to calloc oldnamarr for %d string pointers (%s)\n",
	       progname, oldnamsiz, strerror (errno));
      exit (EXIT_FAILURE);
    };
  memset (cwdbuf, 0, sizeof (cwdbuf));
  if (!getcwd (cwdbuf, sizeof (cwdbuf) - 2))
    {
      fprintf (stderr, "%s: failed to getcwd (%s)\n", progname,
	       strerror (errno));
    };
  DIR *curdir = opendir (cwdbuf);
  if (!curdir)
    {
      fprintf (stderr,
	       "%s: failed to opendir the current directory %s for %d string pointers (%s)\n",
	       progname, cwdbuf, oldnamsiz, strerror (errno));
      exit (EXIT_FAILURE);
    };
  int oldnbnames = 0;
  while ((curent = readdir (curdir)) != NULL)
    {
      char c = 0;
      int ix = -1;
      int pos = -1;
      if (curent->d_type != DT_REG /*not regular file */ )
	continue;
      if (curent->d_name[0] == 'g'
	  && !strncmp (curent->d_name, "genf_", 5)
	  && sscanf (curent->d_name, "genf_%c_%d%n", &c, &ix, &pos) >= 2
	  && ix >= 0
	  && pos > 0 && curent->d_name[pos] == '.' && c >= 'A' && c <= 'Z')
	{
	  if (oldnbnames + 1 >= oldnamsiz)
	    {			// should grow oldnamarr
	      int biggernamsiz =
		1 + ((oldnamsiz + oldnamsiz / 4 + 10) | 0x1f);
	      char **biggernamarr = calloc (biggernamsiz, sizeof (char *));
	      if (!biggernamarr)
		{
		  fprintf (stderr,
			   "%s: failed to calloc biggernamarr for %d string pointers (%s)\n",
			   progname, biggernamsiz, strerror (errno));
		  exit (EXIT_FAILURE);
		};
	      memcpy (biggernamarr, oldnamarr, oldnamsiz * sizeof (char *));
	      oldnamsiz = biggernamsiz;
	      free (oldnamarr);
	      oldnamarr = biggernamarr;
	    };			// end growing up the oldnamarr
	  char *dupcurnam = strdup (curent->d_name);
	  if (!dupcurnam)
	    {
	      fprintf (stderr,
		       "%s: failed in the current directory %s for %d string pointers to strdup name '%s' (%s)\n",
		       progname, cwdbuf, oldnamsiz, curent->d_name,
		       strerror (errno));
	      break;
	    }
	  oldnamarr[oldnbnames++] = dupcurnam;
	}
    };
  closedir (curdir), curdir = NULL;
  for (int k = 0; k < oldnbnames; k++)
    {
      if (unlink (oldnamarr[k]))
	{
	  fprintf (stderr,
		   "%s: failed in current directory %s to unlink '%s' (%s)\n",
		   progname, cwdbuf, oldnamarr[k], strerror (errno));
	}
      else
	nbclean++;
      free (oldnamarr[k]), oldnamarr[k] = NULL;
    };
  free (oldnamarr), oldnamarr = NULL;
  double endelapsedclock = my_clock (CLOCK_MONOTONIC);
  double endcpuclock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  printf
    ("%s: cleaned and removed %d files starting with genf_ in %s in %.4f elapsed, %.4f cpu sec\n",
     progname, nbclean, cwdbuf, endelapsedclock - startelapsedclock,
     endcpuclock - startcpuclock);
  didcleanup = true;
}				/* end cleanup_the_mess */

void
get_options (int argc, char **argv)
{
  bool seeded = false;
  int opt = 0;
  while ((opt = getopt (argc, argv, "hVCFvn:s:j:m:S:R:T:")) > 0)
    {
      switch (opt)
	{
	case 'h':		/* help */
	  show_help ();
	  return;
	case 'V':		/* version */
	  show_version ();
	  return;
	case 'v':		/* verbose */
	  verbose = true;
	  break;
	case 'F':		/* fake run */
	  fakerun = true;
	  break;
	case 'T':
	  terminatingscript = optarg;
	  break;
	case 'C':		/* cleanup - remove all previous plugins
				   and previously generated genf*.c files */
	  cleanup_the_mess ();
	  break;
	case 'n':		/* number of plugins */
	  maxcnt = atoi (optarg);
	  if (maxcnt < MINIMAL_COUNT)
	    {
	      fprintf (stderr,
		       "%s: plugin count given by -n should be at least %d\n",
		       progname, MINIMAL_COUNT);
	      exit (EXIT_FAILURE);
	    };
	  break;
	case 's':		/* mean size */
	  meansize = atoi (optarg);
	  if (meansize < MINIMAL_SIZE)
	    {
	      fprintf (stderr,
		       "%s: mean size given by -s should be at least %d\n",
		       progname, MINIMAL_SIZE);
	      exit (EXIT_FAILURE);
	    }
	  break;
	case 'm':		/* make program */
	  if (strlen (optarg) < 3 || optarg[0] == '.')
	    {
	      fprintf (stderr,
		       "%s: make program given by -m %s should be a command\n"
		       "... and not starting with a dot\n", progname, optarg);
	      exit (EXIT_FAILURE);
	    }
	  makeprog = optarg;
	  break;
	case 'S':		/* plugin suffix */
	  if (optarg[0] != '.' && !isalpha (optarg[1]))
	    {
	      fprintf (stderr,
		       "%s: plugin suffix given by -S %s should start with a dot\n"
		       "... then a letter\n", progname, optarg);
	      exit (EXIT_FAILURE);
	    }
	  pluginsuffix = optarg;
	  break;
	case 'R':
	  random_seed = atol (optarg);
	  seeded = true;
	  break;
	case 'j':
	  makenbjobs = atoi (optarg);
	  if (makenbjobs < MINIMAL_NBJOBS || makenbjobs > MAXIMAL_NBJOBS)
	    {
	      fprintf (stderr, "%s: unexpected -j %d number of make jobs\n"
		       "... (should be between %d and %d)\n",
		       progname, makenbjobs, MINIMAL_NBJOBS, MAXIMAL_NBJOBS);
	    };
	  break;
	default:
	  fprintf (stderr, "%s: unexpected option %c\n", progname,
		   (char) opt);
	  show_help ();
	  exit (EXIT_FAILURE);
	}
    };
  if (seeded)
    srand48 (random_seed);
  else
    {
      time_t t = 0;
      if (!time (&t))
	perror ("time");
      long l = ((long) getpid ()) ^ ((long) t);
      srand48 (l);
    }
}				/* end get_options */


void
generate_all_c_files (void)
{
  double startelapsedclock = my_clock (CLOCK_MONOTONIC);
  double startcpuclock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  printf
    ("%s (git %s, pid %d on %s) start generating %d C files of mean size %d\n",
     progname, MANYDL_GIT, (int) getpid (), myhostname, maxcnt, meansize);
  fflush (NULL);
  int p = 1 + (((int) sqrt (maxcnt + maxcnt / 8)) | 0x1f);
  for (int ix = 0; ix < maxcnt; ix++)
    {
      char curname[64];
      memset (curname, 0, sizeof (curname));
      compute_name_for_index (curname, ix);
      generate_file (curname);
      if (verbose && ix % p == 0 && ix > 10)
	{
	  double curelapsedclock = my_clock (CLOCK_MONOTONIC);
	  double curcpuclock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
	  printf
	    ("%s: %d generated C files out of %d (so %.2f %%) in %.3f elapsed, %.3f cpu sec\n",
	     progname, ix, maxcnt, (100.0 * ix) / maxcnt,
	     curelapsedclock - startelapsedclock,
	     curcpuclock - startcpuclock);
	  fflush (NULL);
	  sync ();
	};
    }
  generate_elapsed_clock = my_clock (CLOCK_MONOTONIC);
  generate_cpu_clock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  printf ("%s generated %d C files in %.3f elapsed sec (%.4f / file)\n",
	  progname, maxcnt, generate_elapsed_clock - start_elapsed_clock,
	  (generate_elapsed_clock - start_elapsed_clock) / (double) maxcnt);
  printf ("%s generated %d C files in %.3f CPU sec (%.4f / file)\n",
	  progname, maxcnt, generate_cpu_clock - start_cpu_clock,
	  (generate_cpu_clock - start_cpu_clock) / (double) maxcnt);
  fflush (NULL);
}				/* end generate_all_c_files */

void
compile_all_plugins (void)
{
  char buildcmd[128];
  struct rusage uscompil = { };
  double childcpuclock = 0.0;
  snprintf (buildcmd, sizeof (buildcmd),
	    "%s -j%d manydl-plugins", makeprog, makenbjobs);
  printf ("%s start compiling %d plugins\n", progname, maxcnt);
  fflush (NULL);
  printf ("%s will do for %d plugins: %s\n", progname, maxcnt, buildcmd);
  fflush (NULL);
  int buildcode = system (buildcmd);
  if (buildcode > 0)
    {
      fprintf (stderr, "%s failed to run: %s (%d)\n",
	       progname, buildcmd, buildcode);
    };
  if (!getrusage (RUSAGE_CHILDREN, &uscompil))
    {
      childcpuclock =
	((double) uscompil.ru_utime.tv_sec +
	 1.0e-6 * uscompil.ru_utime.tv_usec) +
	((double) uscompil.ru_stime.tv_sec +
	 1.0e-6 * uscompil.ru_stime.tv_usec);
    }
  compile_elapsed_clock = my_clock (CLOCK_MONOTONIC);
  compile_cpu_clock = my_clock (CLOCK_PROCESS_CPUTIME_ID) + childcpuclock;
  printf ("%s compiled %d C files in %.3f elapsed sec (%.4f / file)\n",
	  progname, maxcnt, compile_elapsed_clock - generate_elapsed_clock,
	  (compile_elapsed_clock - generate_elapsed_clock) / (double) maxcnt);
  printf ("%s compiled %d C files in %.3f CPU sec (%.4f / file)\n",
	  progname, maxcnt, compile_cpu_clock - generate_cpu_clock,
	  (compile_cpu_clock - generate_cpu_clock) / (double) maxcnt);
}				/* end compile_all_plugins */


void
dlopen_all_plugins (void)
{
  double startelapsedclock = my_clock (CLOCK_MONOTONIC);
  double startcpuclock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  hdlarr = calloc ((size_t) maxcnt, sizeof (void *));
  if (!hdlarr)
    {
      fprintf (stderr, "%s: failed to calloc hdlarr for %d plugins (%s)\n",
	       progname, maxcnt, strerror (errno));
      exit (EXIT_FAILURE);
    };
  namarr = calloc ((size_t) maxcnt, sizeof (char *));
  if (!namarr)
    {
      fprintf (stderr, "%s: failed to calloc namarr for %d plugins (%s)\n",
	       progname, maxcnt, strerror (errno));
      exit (EXIT_FAILURE);
    }
  funarr = calloc ((size_t) maxcnt, sizeof (funptr_t));
  if (!funarr)
    {
      fprintf (stderr,
	       "%s: failed to calloc funarr for %d function pointers (%s)\n",
	       progname, maxcnt, strerror (errno));
      exit (EXIT_FAILURE);
    }
  printf ("%s start dlopen-ing %d plugins\n", progname, maxcnt);
  fflush (NULL);
  fflush (NULL);
  for (int ix = 0; ix < maxcnt; ix++)
    {
      char curname[64];
      memset (curname, 0, sizeof (curname));
      char pluginpath[96];
      memset (pluginpath, 0, sizeof (pluginpath));
      compute_name_for_index (curname, ix);
      namarr[ix] = strdup (curname);
      snprintf (pluginpath, sizeof (pluginpath), "./%s%s",
		curname, pluginsuffix);
      if (access (pluginpath, F_OK))
	{
	  fprintf (stderr, "%s: cannot access plugin#%d %s (%s)\n",
		   progname, ix, pluginpath, strerror (errno));
	  exit (EXIT_FAILURE);
	};
      hdlarr[ix] = dlopen (pluginpath, RTLD_NOW);
      if (!hdlarr[ix])
	{
	  fprintf (stderr, "%s: cannot dlopen plugin#%d %s (%s)\n",
		   progname, ix, pluginpath, dlerror ());
	  exit (EXIT_FAILURE);
	};
      funarr[ix] = dlsym (hdlarr[ix], namarr[ix]);
      if (!funarr[ix])
	{
	  fprintf (stderr, "%s: cannot dlsym name %s in plugin#%d %s (%s)\n",
		   progname, namarr[ix], ix, pluginpath, dlerror ());
	  exit (EXIT_FAILURE);
	};
    };
  double endelapsedclock = my_clock (CLOCK_MONOTONIC);
  double endcpuclock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  printf ("%s dlopen-ed %d plugins in %.3f elapsed sec (%.4f / plugin)\n",
	  progname, maxcnt, endelapsedclock - startelapsedclock,
	  (endelapsedclock - startelapsedclock) / (double) maxcnt);
  printf ("%s dlopen-ed %d plugins in %.3f CPU sec (%.4f / plugin)\n",
	  progname, maxcnt, endcpuclock - startcpuclock,
	  (endcpuclock - startcpuclock) / (double) maxcnt);
  dlopen_elapsed_clock = endelapsedclock;
  dlopen_cpu_clock = endcpuclock;
  fflush (NULL);
}				/* end dlopen_all_plugins */


long
do_the_random_calls_to_dlsymed_functions (void)
{
  double startelapsedclock = my_clock (CLOCK_MONOTONIC);
  double startcpuclock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  long nbcalls = ((long) maxcnt * meansize) / 16 + 1234 + DICE (128);
  printf ("%s: doing %ld random calls (git %s)\n",
	  progname, nbcalls, MANYDL_GIT);
  fflush (NULL);
  int s = maxcnt;
  int r = DICE (100);
  int p = 20 + DICE (10) + (int) sqrt (nbcalls);
  for (long n = 0; n < nbcalls; n++)
    {
      int k = DICE (maxcnt);
      r = (*funarr[k]) ((int) (n / 10) + DICE (maxcnt), s + r);
      if (n % p == 0)
	printf ("%s: after %ld calls r is %d\n", progname, n, r);
    }
  double endelapsedclock = my_clock (CLOCK_MONOTONIC);
  double endcpuclock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  printf
    ("%s: done %ld random calls (git %s) in %.3f elapsed %.4f cpu sec with %d plugins\n",
     progname, nbcalls, MANYDL_GIT, (endelapsedclock - startelapsedclock),
     (endcpuclock - startcpuclock), maxcnt);
  printf ("%s: so %.3f µs elapsed %.3f µs cpu µs per call\n", progname,
	  1.0e+6 * (endelapsedclock - startelapsedclock) / (double) nbcalls,
	  1.0e+6 * (endcpuclock - startcpuclock) / (double) nbcalls);
  compute_elapsed_clock = endelapsedclock;
  compute_cpu_clock = endcpuclock;
  fflush (NULL);
  return nbcalls;
}				/* end do_the_random_calls_to_dlsymed_functions */


void
run_terminating_script (void)
{
  double startelapsedclock = my_clock (CLOCK_MONOTONIC);
  double startcpuclock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  char cwdbuf[128];
  memset (cwdbuf, 0, sizeof (cwdbuf));
  if (NULL == getcwd (cwdbuf, sizeof (cwdbuf)))
    {
      fprintf (stderr,
	       "%s: failed to getcwd when running terminating script %s (%s)\n",
	       progname, terminatingscript, strerror (errno));
      exit (EXIT_FAILURE);
    };
  char termcmd[256];
  memset (termcmd, 0, sizeof (termcmd));
  snprintf (termcmd, sizeof (termcmd), "%s %d '%s'", terminatingscript,
	    maxcnt, cwdbuf);
  // should putenv -in static buffers- MANYDL_GIT and MANYDL_SIZE and MANYDL_MAXCOUNT etc...
  static char gitenv[64];
  static char sizenv[64];
  static char pidenv[64];
  static char cntenv[64];
  static char cwdenv[256];
  snprintf (gitenv, sizeof (gitenv), "MANYDL_GIT=%s", MANYDL_GIT);
  putenv (gitenv);
  snprintf (sizenv, sizeof (sizenv), "MANYDL_MEANSIZE=%d", meansize);
  putenv (sizenv);
  snprintf (cntenv, sizeof (cntenv), "MANYDL_MAXCOUNT=%d", maxcnt);
  putenv (cntenv);
  snprintf (pidenv, sizeof (pidenv), "MANYDL_PID=%d", (int) getpid ());
  putenv (pidenv);
  snprintf (cwdenv, sizeof (cwdenv), "MANYDL_CWD=%s", cwdbuf);
  putenv (cwdenv);
  if (verbose)
    printf ("%s: (pid %d on %s) running terminating command %s\n", progname,
	    (int) getpid (), myhostname, termcmd);
  fflush (NULL);
  FILE *pfil = popen (termcmd, "w");
  if (!pfil)
    {
      fprintf (stderr, "%s: failed to popen terminating command %s (%s)\n",
	       progname, termcmd, strerror (errno));
      exit (EXIT_FAILURE);
    }
  // then use popen on the terminatingscript command
  // and write all the generated C file names to that pipe
  for (int ix = 0; ix < maxcnt; ix++)
    {
      char nambuf[NAME_BUFLEN + 4];
      memset (nambuf, 0, sizeof (nambuf));
      compute_name_for_index (nambuf, ix);
      fputs (nambuf, pfil);
      putc ('\n', pfil);
      if (ix % 16 == 0)
	{
	  fflush (pfil);
	  if (ix % 256 == 0)
	    usleep (5000);	/// give a chance to the script to run more
	};
    }
  fflush (pfil);
  usleep (8000);
  int piperr = pclose (pfil);
  if (piperr)
    {
      if (WIFEXITED (piperr))
	fprintf (stderr, "%s: failing terminating command %s -exited %d\n",
		 progname, termcmd, WEXITSTATUS (piperr));
      else if (WIFSIGNALED (piperr))
	{
	  int signum = WTERMSIG (piperr);
	  if (WCOREDUMP (piperr))
	    fprintf (stderr,
		     "%s: failing terminating command %s -got signal#%d (%s) and core dumped\n",
		     progname, termcmd, signum, strsignal (signum));
	  else
	    fprintf (stderr,
		     "%s: failing terminating command %s -got signal#%d (%s)\n",
		     progname, termcmd, signum, strsignal (signum));
	}
      fflush (stderr);
      exit (EXIT_FAILURE);
    }
  double endelapsedclock = my_clock (CLOCK_MONOTONIC);
  double endcpuclock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  terminating_elapsed_clock = endelapsedclock;
  terminating_cpu_clock = endcpuclock;
  printf
    ("%s: (pid %d on %s) done terminating command %s in %.3f elapsed, %.3f cpu sec\n",
     progname, (int) getpid (), myhostname, termcmd,
     endelapsedclock - startelapsedclock, endcpuclock - startcpuclock);
  fflush (NULL);
}				/* end run_terminating_script */




int
main (int argc, char **argv)
{
  long nbcalls = 0;
  progname = argv[0];
  gethostname (myhostname, sizeof (myhostname) - 1);
  start_elapsed_clock = my_clock (CLOCK_MONOTONIC);
  start_cpu_clock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  if (argc > 1 && !strcmp (argv[1], "--help"))
    show_help ();
  if (argc > 1 && !strcmp (argv[1], "--version"))
    show_version ();
  get_options (argc, argv);
  if (didcleanup)
    return 0;
  generate_all_c_files ();
  if (!fakerun)
    {
      compile_all_plugins ();
      dlopen_all_plugins ();
      nbcalls = do_the_random_calls_to_dlsymed_functions ();
    };
  if (terminatingscript)
    {
      if (nbcalls > 0)
	{
	  static char callenv[64];
	  snprintf (callenv, sizeof (callenv), "MANYDL_NBCALLS=%ld", nbcalls);
	  putenv (callenv);
	};
      run_terminating_script ();
    };
  printf ("%s: (git %s built on %s) pid %d ending on %s (%s:%d)\n",
	  progname, MANYDL_GIT, __DATE__ "@" __TIME__, (int) getpid (),
	  myhostname, __FILE__, __LINE__);
  printf
    ("%s: generated %d plugins of mean size %d in %.3f elapsed, %.3f cpu seconds\n",
     progname, maxcnt, meansize, generate_elapsed_clock - start_elapsed_clock,
     generate_cpu_clock - start_cpu_clock);
  if (!isnan (compile_elapsed_clock) && !isnan (compile_cpu_clock))
    printf
      ("%s: compiled %d plugins of mean size %d in %.3f elapsed, %.3f cpu seconds\n",
       progname, maxcnt, meansize,
       compile_elapsed_clock - generate_elapsed_clock,
       compile_cpu_clock - generate_cpu_clock);
  if (!isnan (dlopen_elapsed_clock) && !isnan (dlopen_cpu_clock))
    printf
      ("%s: dlopened %d plugins of mean size %d in %.3f elapsed, %.3f cpu seconds\n",
       progname, maxcnt, meansize,
       dlopen_elapsed_clock - compile_elapsed_clock,
       compile_cpu_clock - dlopen_cpu_clock);
  if (!isnan (compute_elapsed_clock) && !isnan (compute_cpu_clock))
    printf
      ("%s: computed %ld calls with %d plugins of mean size %d in %.3f elapsed, %.3f cpu seconds\n",
       progname, nbcalls, maxcnt, meansize,
       compute_elapsed_clock - dlopen_elapsed_clock,
       compute_cpu_clock - dlopen_cpu_clock);
  if (!isnan (terminating_elapsed_clock) && !isnan (terminating_cpu_clock))
    printf
      ("%s: terminating script %s with %d plugins of mean size %d in %.3f elapsed, %.3f cpu seconds\n",
       progname, terminatingscript, maxcnt, meansize,
       terminating_elapsed_clock - dlopen_elapsed_clock,
       terminating_cpu_clock - dlopen_cpu_clock);

  // we don't bother to dlclose
  return 0;
}				/* end of main */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make manydl" ;;
 ** End: ;;
 ****************/

/// end of manydl.c
