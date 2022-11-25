/* file manydl.c from https://github.com/bstarynk/misc-basile/
   generating lots of C functions, and dynamically compiling and loading them
   this is completely useless, except for testing & benchmarking 
   
   © Copyright Basile Starynkevitch 2004- 2021
   program released under GNU general public license

   this is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   this is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>
#include <time.h>
#include <sys/times.h>
#include <unistd.h>
#include <sys/utsname.h>

#ifdef __FRAMAC__
#define gnu_get_libc_version() "libc-framac"
#define gnu_get_libc_release() "libc-framac-release"
#else
#include <gnu/libc-version.h>
#endif /*__FRAMAC__*/
char *progname;

#define INDENT_PROGRAM "/usr/bin/indent"

extern long dynstep;
extern void say_fun_a_b_c_d (const char *fun, int a, int b, int c, int d);

/* dynstep, tab & say_fun_a_b_c_d are used in generated files */
long dynstep;			/* number of executed lines */
#define MAXTAB 128		/* length of tab should be a power of two */
int tab[MAXTAB];		/* a table */

extern int tab[MAXTAB];

void				/* printing function for generated files */
say_fun_a_b_c_d (const char *fun, int a, int b, int c, int d)
{
  printf ("<%s> a=%d b=%d c=%d d=%d\n", fun, a, b, c, d);
}

double secpertick;
clock_t firstclock;

#define DICE(N) ((int)(lrand48 () % (N)))

/* generate a file containing one single function (named like the
   file, ie function foo in foo.c), using the lrand48 random number
   generator; return the size */

int
generate_file (const char *name, int meanlen)
{
  char pathsrc[100];
  char pathso[100];
  FILE *f = NULL;
  int l = 0;
  int i = 0;
  int prevjmpix = 0;
#define MAXLAB 8
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
  snprintf (pathso, sizeof (pathso) - 1, "%s.so", name);
  remove (pathso);
  if (meanlen < 20)
    meanlen = 20;
  /* random length of generated function */
  l = meanlen / 2 + DICE (meanlen);
  fprintf (f, "/* generated file %s length %d meanlen %d*/\n", pathsrc, l,
	   meanlen);
  fprintf (f, "extern long dynstep;\n" "extern int tab[%d];\n", MAXTAB);
  fprintf (f,
	   "extern void say_fun_a_b_c_d(const char*fun, int a, int b, int c, int d);\n");
  fprintf (f, "const char gentimestamp_%s[] = __DATE__ \"@\" __TIME__;\n",
	   name);
  fprintf (f, "int %s(int a, int b) {\n", name);
  fputs ("  int c=0, d=1, e=2, f=3, g=4, h=5;\n", f);
#define RANVAR ('a'+DICE(8))
  for (i = 0; i < l; i++)
    {
      switch (DICE (16))
	{
	case 0:
	  fprintf (f, " %c = (%c * %d + %c * %d + %d) & 0xffffff;\n",
		   RANVAR, RANVAR, 2 + DICE (8), RANVAR, 3 + 2 * DICE (8),
		   DICE (32));
	  break;
	case 1:
	  fprintf (f,
		   " %c = (%c * %d + tab[%c & %d] - %c) & 0xffffff; tab[%d]++;\n",
		   RANVAR, RANVAR, 1 + DICE (16), RANVAR, MAXTAB - 1, RANVAR,
		   DICE (MAXTAB));
	  break;
	case 2:
	  ;
	  fprintf (f,
		   " if (%c > %c + %d) %c++; else %c=(tab[%d] + %c) & 0xffffff;\n",
		   RANVAR, RANVAR, DICE (8), RANVAR, RANVAR, DICE (MAXTAB),
		   RANVAR);
	  break;
	case 3:
	  fprintf (f, " tab[%c & %d] += %d + ((%c - %c) & 0xf); %c++;\n",
		   RANVAR, MAXTAB - 1, DICE (8) + 2, RANVAR, RANVAR, RANVAR);
	  break;
	case 4:
	  {
	    char lvar = RANVAR;
	    fprintf (f,
		     " while (%c>0) { dynstep++; %c -= (%c/3) + %d; }; %c=%d+%c;\n",
		     lvar, lvar, lvar, DICE (16) + 10, lvar, DICE (8),
		     RANVAR);
	  }
	  break;
	case 5:
	  fprintf (f, " %c++, %c++;\n", RANVAR, RANVAR);
	  break;
	case 6:
	  {
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
	  fprintf (f,
		   " %c = (%c / ((%c & 0xffff) + 2) + %c * %d) & 0xffffff;\n",
		   RANVAR, RANVAR, RANVAR, RANVAR, DICE (10) + 3);
	  break;
	case 8:
	  fprintf (f,
		   " %c = (%d + ((%c << (1+ (%c & 0x7))) + %c)) & 0xffffff;\n",
		   RANVAR, DICE (32) + 5, RANVAR, RANVAR, RANVAR);
	  break;
	case 9:
	  fprintf (f, " %c = (%c * %d + %d) & 0xffffff;\n",
		   RANVAR, RANVAR, DICE (100) + 7, DICE (200) + 12);
	  break;
	case 10:
	  fprintf (f, " tab[%d]++, tab[%c & %d] += (%c & 0xff) + %d;\n",
		   DICE (MAXTAB), RANVAR, MAXTAB - 1, RANVAR, DICE (8) + 2);
	  break;
	case 11:
	  fprintf (f, " %c = %c;\n", RANVAR, RANVAR);
	  break;
	case 12:
	  fprintf (f, " %c = %d;\n", RANVAR, 5 + DICE (20));
	  break;
	case 13:
	  {
	    char fvar = RANVAR;
	    fprintf (f,
		     " for (%c &= %d; %c>0; %c--) {dynstep++;tab[%c] += (1+(%c&0x1f));};\n",
		     fvar, MAXTAB - 1, fvar, fvar, fvar, RANVAR);
	    break;
	case 14:
	    {
	      char lvar = RANVAR, rvar = RANVAR;
	      if (lvar != rvar)
		fprintf (f, " %c = %c;\n", lvar, rvar);
	      else
		fprintf (f, " %c++;\n", lvar);
	    };
	    break;
	case 15:
	    fprintf (f, " %c = %c + %d;\n", RANVAR, RANVAR, 2 + DICE (5));
	    break;
	  };
	}
    };
  fprintf (f, " dynstep+=%d;\n", i - prevjmpix);
  for (i = 0; i < MAXLAB; i++)
    if (lab[i] == LAB_JUMPED)
      fprintf (f, " lab%d:", i);
  fprintf (f, " a &= 0xffffff;\n");
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
	  getcwd (cwdbuf, sizeof (cwdbuf) - 2);
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
	    "CPU= %.2fu+%.2fs self, %.2fu+%.2fs children; real= %.2f",
	    secpertick * ti.tms_utime, secpertick * ti.tms_stime,
	    secpertick * ti.tms_cutime, secpertick * ti.tms_cstime,
	    secpertick * (clock - firstclock));
  return timbuf;
}

int
main (int argc, char **argv)
{
  progname = argv[0];
  int maxcnt = 100;
  int meanlen = 300;
  int k = 0, l = 0;
  long suml = 0;
  int r = 0, s = 0;
  long nbcall = 0, n = 0;
  FILE *map = NULL;		/* the /proc/self/maps pseudofile (Linux) */
  char buf[100] = { (char) 0 };	/* buffer for name */
  char cmd[400] = { (char) 0 };	/* buffer for command */
  char linbuf[500] = { (char) 0 };	/* line buffer for map */
  char *cc = NULL;		/* the CC command or gcc (from environment) */
  char *cflags = NULL;		/* the CFLAGS option or -O (from environment)  */
  typedef int (*funptr_t) (int, int);	/* tpye of function pointers */
  void **hdlarr = NULL;		/* array of dlopened handles */
  funptr_t *funarr = NULL;	/* array of function pointers */
  char **namarr = NULL;		/* array of names */
  struct utsname uts = { };	/* for uname ie system version */
  struct tms t_init = { }, t_generate = { }, t_load = { }, t_run = { };
  clock_t cl_init = 0, cl_generate = 0, cl_load = 0, cl_run = 0;
  double tim_cpu_generate = 0.0, tim_real_generate = 0.0;
  double tim_cpu_load = 0.0, tim_real_load = 0.0;
  double tim_cpu_run = 0.0, tim_real_run = 0.0;
  if (argc > 1 && !strcmp (argv[1], "--help"))
    {
      printf ("usage: %s [maxcnt [meanlen]]\n"
	      "\t\t (a program generating many plugins for dlopen on Linux)\n"
	      "\t where maxcnt (default %d) is the count of generated functions and plugins.\n"
	      "\t where meanlen (default %d) is related to the mean length of generated functions",
	      argv[0], maxcnt, meanlen);
      exit (EXIT_SUCCESS);
    }
  else if (argc > 1 && !strcmp (argv[1], "--version"))
    {
      printf ("No version info for %s, %s\n",
	      argv[0], __FILE__ " " __DATE__ "@" __TIME__);
      exit (EXIT_FAILURE);
    }
  /* initialize the clock */
  secpertick = 1.0 / (double) sysconf (_SC_CLK_TCK);
  memset (&t_init, 0, sizeof (t_init));
  firstclock = cl_init = times (&t_init);
  if (nice (2) < 0)
    perror ("nice");
  /* If we have a program argument, it is the number of generated functions */
  if (argc > 1)
    maxcnt = atoi (argv[1]);
  if (maxcnt < 4)
    maxcnt = 4;
  else if (maxcnt > 500000)
    maxcnt = 500000;
  if (argc > 2)
    meanlen = atoi (argv[2]);
  if (meanlen < 50)
    meanlen = 50;
  else if (meanlen > 200000)
    meanlen = 200000;
  /* ask for system information */
  memset (&uts, 0, sizeof (uts));
  uname (&uts);			/* don't bother checking success */
  /* print counter, system, & library information */
  printf
    ("Running %s, max.counter %d, mean len %d, glibc version %s release %s\n"
     "on sys %s release %s version %s\n", argv[0], maxcnt, meanlen,
     gnu_get_libc_version (), gnu_get_libc_release (), uts.sysname,
     uts.release, uts.version);
  /* allocate the array of handles (for dlopen) */
  hdlarr = (void **) calloc (maxcnt + 1, sizeof (*hdlarr));
  if (!hdlarr)
    {
      perror ("allocate hdlarr");
      exit (EXIT_FAILURE);
    };
  /* allocate the array of functions pointers */
  funarr = (funptr_t *) calloc (maxcnt + 1, sizeof (*funarr));
  if (!funarr)
    {
      perror ("allocate funarr");
      exit (EXIT_FAILURE);
    };
  /* allocate the array of function (& file) names */
  namarr = (char **) calloc (maxcnt + 1, sizeof (*funarr));
  if (!namarr)
    {
      perror ("allocate namarr");
      exit (EXIT_FAILURE);
    };
  /* initialise the sum of functions' length */
  suml = 0;
  /* open the memory map */
  map = fopen ("/proc/self/maps", "r");
  if (!map)
    {
      perror ("/proc/self/maps");
      exit (EXIT_FAILURE);
    };
  /* get the compiler - default is gcc */
  cc = getenv ("CC");
  if (!cc)
    cc = "gcc";
  /* get the compileflags - default is -O2 */
  cflags = getenv ("CFLAGS");
  if (!cflags)
    cflags = "-O2";
  time_t nowt = 0;
  time (&nowt);
  char hnam[80];
  memset (&hnam, 0, sizeof (hnam));
  gethostname (hnam, sizeof (hnam) - 2);
  printf
    ("%s: before generation of %d files, mean size %d, with CC=%s and CFLAGS=%s\n"
     "... at %s on %s pid %d\n", argv[0], maxcnt, meanlen, cc, cflags,
     ctime (&nowt), hnam, (int) getpid ());
  fflush (NULL);
  /* the generating and compiling loop */
  for (k = 0; k < maxcnt; k++)
    {
      /* generate a name like genf_C9 or genf_A0 and duplicate it */
      memset (buf, 0, sizeof (buf));
      snprintf (buf, sizeof (buf) - 1, "genf_%c%d", "ABCDEFGHIJ"[k % 10],
		k / 10);
      namarr[k] = strdup (buf);
      if (!namarr[k])
	{
	  perror ("strdup");
	  exit (EXIT_FAILURE);
	};
      printf ("generating %s (#%d); ", namarr[k], k + 1);
      fflush (stdout);
      /* generate and compile the file */
      l = generate_file (namarr[k], meanlen);
      printf ("compiling %d instructions ", l);
      fflush (stdout);
      memset (cmd, 0, sizeof (cmd));
      snprintf (cmd, sizeof (cmd) - 1,
		"%s -fPIC -shared %s %s.c -o %s.so",
		cc, cflags, namarr[k], namarr[k]);
      if (system (cmd))
	{
	  fprintf (stderr, "\ncompilation %s #%d failed\n", cmd, k + 1);
	  exit (EXIT_FAILURE);
	};
      putchar ('.');
      putchar ('\n');
      fflush (stdout);
      suml += l;
      if (k % 32 == 16)
	printf
	  ("after %d generated & compiled files (%ld instrs) time\n .. %s [sec]\n",
	   k + 1, suml, timestring ());
    };
  putchar ('.');
  putchar ('\n');
  fflush (stdout);
  suml += l;
  if (k % 32 == 16)
    printf
      ("after %d generated & compiled files (%ld instrs) time\n .. %s [sec]\n",
       k + 1, suml, timestring ());

  memset (&t_generate, 0, sizeof (t_generate));
  cl_generate = times (&t_generate);
  tim_cpu_generate =
    secpertick * (t_generate.tms_cutime + t_generate.tms_cstime
		  + t_generate.tms_utime + t_generate.tms_stime)
    - secpertick * (t_init.tms_cutime + t_init.tms_cstime
		    + t_init.tms_utime + t_init.tms_stime);
  tim_real_generate = secpertick * (cl_generate - cl_init);
  fprintf (stderr,
	   "\ngenerated & compiled %d files (%ld instr) = mean %.2f instr/file.\n generation time %s\n",
	   maxcnt, suml, (double) suml / maxcnt, timestring ());
  fprintf (stderr, "generation time total: %.2f cpu %.2f real seconds\n",
	   tim_cpu_generate, tim_real_generate);
  fprintf (stderr,
	   "generation time per file: %.3f cpu %.3f real seconds/file\n",
	   tim_cpu_generate / maxcnt, tim_real_generate / maxcnt);
  fprintf (stderr,
	   "generation time per instr: %g cpu %g real milliseconds/instr\n",
	   1.0e3 * tim_cpu_generate / suml, 1.0e3 * tim_real_generate / suml);
  /* now all files are generated, and compiled, each into a shared
     library *.so; we load all the libraries with dlopen */
  printf ("\n generated %ld instructions in %d files\n", suml, k);
  for (k = 0; k < maxcnt; k++)
    {
      printf ("dynloading #%d %s", k + 1, namarr[k]);
      fflush (stdout);
      snprintf (cmd, sizeof (cmd) - 1, "./%s.so", namarr[k]);
      hdlarr[k] = dlopen (cmd, RTLD_NOW);
      /* if the dynamic load failed we display an error message and the
         memory map */
      if (!hdlarr[k])
	{
	  fprintf (stderr, "\n dlopen %s failed %s\n", cmd, dlerror ());
	  /* display the memory map */
	  rewind (map);
	  do
	    {
	      memset (linbuf, 0, sizeof (linbuf));
	      if (!fgets (linbuf, sizeof (linbuf) - 1, map))
		break;
	      fputs (linbuf, stderr);
	    }
	  while (!feof (map));
	  fflush (stderr);
	  exit (EXIT_FAILURE);
	};
      funarr[k] = (funptr_t) dlsym (hdlarr[k], namarr[k]);
      if (!funarr[k])
	fprintf (stderr, "\n dlsym %s failed %s\n", namarr[k], dlerror ());
      printf ("@%p\n", (void *) funarr[k]);
      if (k % 32 == 16)
	printf ("after %d dlopened files time\n .. %s [sec]\n",
		k + 1, timestring ());
      fflush (stdout);
      fflush (stderr);
    };

  memset (&t_load, 0, sizeof (t_load));
  cl_load = times (&t_load);
  tim_cpu_load =
    secpertick * (t_load.tms_cutime + t_load.tms_cstime
		  + t_load.tms_utime + t_load.tms_stime)
    - secpertick * (t_generate.tms_cutime + t_generate.tms_cstime
		    + t_generate.tms_utime + t_generate.tms_stime);
  tim_real_load = secpertick * (cl_load - cl_generate);
  /* good, all the dlopen-ed went ok */
  printf ("\n\n *** %d shared objects have been loaded successfully\n"
	  " .. %s [sec]\n", maxcnt, timestring ());
  fprintf (stdout,
	   "\nloaded  %d shared objects (%ld C instr)\n load time %s\n",
	   maxcnt, suml, timestring ());
  fprintf (stderr, "dynload time total: %.2f cpu %.2f real seconds\n",
	   tim_cpu_load, tim_real_load);
  fprintf (stderr, "dynload time per file: %.3f cpu %.3f real seconds/file\n",
	   tim_cpu_load / maxcnt, tim_real_load / maxcnt);
  fprintf (stderr,
	   "dynload time per instr: %g cpu %g real milliseconds/instr\n",
	   1.0e3 * tim_cpu_load / suml, 1.0e3 * tim_real_load / suml);
  /* display the memory map */
  printf ("\n\n**** memory map of process %ld ****\n", (long) getpid ());
  rewind (map);
  do
    {
      memset (linbuf, 0, sizeof (linbuf));
      if (NULL == fgets (linbuf, sizeof (linbuf) - 1, map))
	break;
      fputs (linbuf, stdout);
    }
  while (!feof (map));
  putchar ('\n');
  fflush (stdout);
  /* call randomly the functions */
  r = 0;
  s = maxcnt;
  nbcall = (maxcnt * meanlen) / 16 + 1000;
  for (n = 0; n < nbcall; n++)
    {
      k = DICE (maxcnt);
      s = DICE (meanlen);
      printf ("calling #%d: ", k);
      fflush (stdout);
      r = (*funarr[k]) (n / 16, s);
      if (n % 64 == 32)
	printf ("after %ld calls time\n .. %s [sec]\n", n + 1, timestring ());
    };

  memset (&t_run, 0, sizeof (t_run));
  cl_run = times (&t_run);
  tim_cpu_run =
    secpertick * (t_run.tms_cutime + t_run.tms_cstime
		  + t_run.tms_utime + t_run.tms_stime)
    - secpertick * (t_load.tms_cutime + t_load.tms_cstime
		    + t_load.tms_utime + t_load.tms_stime);
  tim_real_run = secpertick * (cl_run - cl_load);
  printf
    ("\nmade %ld calls to %d functions r=%d s=%d dynstep=%ld "
     "tab=%d %d %d %d ...\n... %s\n",
     nbcall, maxcnt, r, s, dynstep, tab[0], tab[1], tab[2], tab[3],
     timestring ());
  fprintf (stdout,
	   "\nrun  %ld calls (%ld C steps)=%g steps/call\n run time %s\n",
	   nbcall, dynstep, (double) dynstep / nbcall, timestring ());
  fprintf (stderr,
	   "run time total: %.4f cpu %.4f real seconds (%ld calls, %ld steps)\n",
	   tim_cpu_run, tim_real_run, nbcall, dynstep);
  fprintf (stderr,
	   "run time per call: %.4f cpu %.4f real milliseconds/file\n",
	   1.0e3 * tim_cpu_run / nbcall, 1.0e3 * tim_real_run / nbcall);
  fprintf (stderr, "run time per step: %g cpu %g real microseconds/step\n",
	   1.0e6 * tim_cpu_run / dynstep, 1.0e6 * tim_real_run / dynstep);
  /* cleaning up */
  for (k = 0; k < maxcnt; k++)
    {
      funarr[k] = 0;
      if (hdlarr[k] && dlclose (hdlarr[k]))
	fprintf (stderr, "failed to dlclose %s - %s\n", namarr[k],
		 dlerror ());
      if (namarr[k])
	free (namarr[k]);
      namarr[k] = 0;
    };

  free (funarr);
  free (namarr);
  free (hdlarr);
  printf
    ("\n total time for %d files %ld instructions %ld executed steps %ld calls:\n ... %s sec\n id %s\n",
     maxcnt, suml, dynstep, nbcall, timestring (),
     __FILE__ " " __DATE__ "@" __TIME__);
  return 0;
}


/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "gcc -o manydl -Wall -rdynamic -O -g manydl.c -ldl" ;;
 ** End: ;;
 ****************/
