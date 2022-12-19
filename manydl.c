/***
 *  file manydl.c from https://github.com/bstarynk/misc-basile/
 *  SPDX-License-Identifier: MIT
 *  
 *  generating lots of C functions, and dynamically compiling and loading them
 *  this is completely useless, except for testing & benchmarking 
 *  
 *  © Copyright Basile Starynkevitch 2004- 2022
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
#include <dlfcn.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>
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
		   " %c = (%c * %d + tab[%c & %d] - %c) & 0xffffff; tab[%d]++;\n",
		   RANVAR, RANVAR, 1 + DICE (16), RANVAR, MAXTAB - 1, RANVAR,
		   DICE (MAXTAB));
	  if (DICE (8) == 0)
	    fprintf (f, " dynstep++;\n");
	  break;
	case 2:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f,
		   " if (%c > %c + %d) %c++; else %c=(tab[%d] + %c) & 0xffffff;\n",
		   RANVAR, RANVAR, DICE (8), RANVAR, RANVAR, DICE (MAXTAB),
		   RANVAR);
	  if (DICE (8) == 0)
	    fprintf (f, " dynstep++;\n");
	  break;
	case 3:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f, " tab[%c & %d] += %d + ((%c - %c) & 0xf); %c++;\n",
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
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f,
		   " %c = (%c / ((%c & 0xffff) + 2) + %c * %d) & 0xffffff;\n",
		   RANVAR, RANVAR, RANVAR, RANVAR, DICE (10) + 3);
	  break;
	case 8:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f,
		   " %c = (%d + ((%c << (1+ (%c & 0x7))) + %c)) & 0xffffff;\n",
		   RANVAR, DICE (32) + 5, RANVAR, RANVAR, RANVAR);
	  break;
	case 9:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f, " %c = (%c * %d + %d) & 0xffffff;\n",
		   RANVAR, RANVAR, DICE (100) + 7, DICE (200) + 12);
	  break;
	case 10:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f, " tab[%d]++, tab[%c & %d] += (%c & 0xff) + %d;\n",
		   DICE (MAXTAB), RANVAR, MAXTAB - 1, RANVAR, DICE (8) + 2);
	  fprintf (f,
		   " if (tab[%d] %% %d == 0 && dynstep > initdynstep + %d)\n"
		   "     goto end_%s;\n", DICE (MAXTAB), (40 + DICE (50)),
		   meansize + 50 + 20 * DICE (1000), name);
	  break;
	case 11:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f, " %c = %c + %d;\n", RANVAR, RANVAR, (2 + DICE (5)));
	  break;
	case 12:
	  fprintf (f, "// from %d\n", __LINE__);
	  fprintf (f, " %c = %d;\n", RANVAR, 5 + DICE (20));
	  break;
	case 13:
	  {
	    fprintf (f, "// from %d\n", __LINE__);
	    char fvar = RANVAR;
	    fprintf (f,
		     " for (%c &= %d; %c>0; %c--) {dynstep++;tab[%c] += (1+(%c&0x1f));};\n",
		     fvar, MAXTAB - 1, fvar, fvar, fvar, RANVAR);
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
	  };
	  break;
	case 15:
	  {
	    fprintf (f, "// from %d\n", __LINE__);
	    int labrank = DICE (MAXLAB);
	    fprintf (f, " %c = %c + %d;\n", RANVAR, RANVAR, 2 + DICE (5));
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
	fprintf (f, " lab%d: %c++;\n", i, RANVAR);
      if (DICE (4) == 0 || i == MAXLAB / 2)
	{
	  fprintf (f, " tab[(%c &0xffffff) %% %d] += %c;\n", RANVAR, MAXTAB,
		   RANVAR);
	  fprintf (f, " %c += (%c&0xffff);\n", RANVAR, RANVAR);
	  fprintf (f, " dynstep++;\n");
	};
    }
  fprintf (f, " a &= 0xffffff;\n");
  fprintf (f, "end_%s:\n", name);
  fprintf (f, " say_fun_a_b_c_d(\"%s\", a, b, c, d);\n", name);
  fprintf (f, " return a;\n" "} /* end %s of %d instr */\n", name, l);
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
  printf ("\t -n <count>  : set number of generated plugins,"
	  " default is %d\n", maxcnt);
  printf ("\t -s <mean-size>   : set mean size of generated plugins,"
	  " default is %d\n", meansize);
  printf ("\t -j <job>        : number of jobs, passed to make,"
	  " default is %d\n", makenbjobs);
  printf ("\t -m <maker>      : make program, default is %s\n", makeprog);
  printf ("\t -R <randomseed> : seed passed to srand48, default is unique\n");
  printf ("\t -S <p.suffix>  : plugin suffix, default is %s\n", pluginsuffix);

}				/* end of show_help */

void
show_version (void)
{
  printf ("%s git %s on %s at built %s\n", progname, manydl_git, myhostname,
	  __DATE__);
  exit (EXIT_SUCCESS);
}				/* end of show_version */



void
get_options (int argc, char **argv)
{
  bool seeded = false;
  int opt = 0;
  while ((opt = getopt (argc, argv, "hVvn:s:j:m:S:R:")) > 0)
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
  for (int ix = 0; ix < maxcnt; ix++)
    {
      char curname[64];
      memset (curname, 0, sizeof (curname));
      compute_name_for_index (curname, ix);
      generate_file (curname);
    }
  double endelapsedclock = my_clock (CLOCK_MONOTONIC);
  double endcpuclock = my_clock (CLOCK_PROCESS_CPUTIME_ID);
  printf ("%s generated %d C files in %.3f elapsed sec (%.4f / file)\n",
	  progname, maxcnt, endelapsedclock - startelapsedclock,
	  (endelapsedclock - startelapsedclock) / (double) maxcnt);
  printf ("%s generated %d C files in %.3f CPU sec (%.4f / file)\n",
	  progname, maxcnt, endcpuclock - startcpuclock,
	  (endcpuclock - startcpuclock) / (double) maxcnt);
  fflush (NULL);
}				/* end generate_all_c_files */

void
compile_all_plugins (void)
{
  char buildcmd[128];
  snprintf (buildcmd, sizeof (buildcmd),
	    "%s -j%d manydl-plugins", makeprog, makenbjobs);
  fflush (NULL);
  printf ("%s will do: %s\n", progname, buildcmd);
  fflush (NULL);
  int buildcode = system (buildcmd);
  if (buildcode > 0)
    {
      fprintf (stderr, "%s failed to run: %s (%d)\n",
	       progname, buildcmd, buildcode);
    };
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
  fflush (NULL);
}				/* end dlopen_all_plugins */

int
main (int argc, char **argv)
{
  progname = argv[0];
  gethostname (myhostname, sizeof (myhostname) - 1);
  if (argc > 1 && !strcmp (argv[1], "--help"))
    show_help ();
  if (argc > 1 && !strcmp (argv[1], "--version"))
    show_version ();
  get_options (argc, argv);
  generate_all_c_files ();
  compile_all_plugins ();
  dlopen_all_plugins ();
}				/* end of main */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make manydl" ;;
 ** End: ;;
 ****************/

/// end of manydl.c
