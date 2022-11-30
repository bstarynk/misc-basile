/* file manydl.c from https://github.com/bstarynk/misc-basile/
   generating lots of C functions, and dynamically compiling and loading them
   this is completely useless, except for testing & benchmarking 
   
   © Copyright Basile Starynkevitch 2004- 2022
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
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/utsname.h>

#ifdef MANYDL_GIT
const char manydl_git[] = MANYDL_GIT;
#else
#define manydl_git "???"
#endif

#ifdef __FRAMAC__
#define gnu_get_libc_version() "libc-framac"
#define gnu_get_libc_release() "libc-framac-release"
#else
#include <gnu/libc-version.h>
#endif /*__FRAMAC__*/
char *progname;

int maxcnt = 100;
int meanlen = 300;

typedef int (*funptr_t) (int, int);	/* type of function pointers */

funptr_t *funarr = NULL;
char **namarr = NULL;		/* array of names */

FILE *timedataf;

char myhostname[64];
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
  fputs ("  int c=0, d=1, e=2, f=3, g=4, h=5, i=6, j=7, k=8, l=a+b;\n", f);
  fputs ("  long initdynstep = dynstep;\n", f);
#define RANVAR ('a'+DICE(12))
  for (i = 0; i < l; i++)
    {
      switch (DICE (16))
	{
	case 0:
	  fprintf (f, " %c = (%c * %d + %c * %d + %d) & 0xffffff;\n",
		   RANVAR, RANVAR, 2 + DICE (8), RANVAR, 3 + 2 * DICE (8),
		   DICE (32));
	  if (DICE (8) == 0)
	    fprintf (f, " dynstep++;\n");
	  break;
	case 1:
	  fprintf (f,
		   " %c = (%c * %d + tab[%c & %d] - %c) & 0xffffff; tab[%d]++;\n",
		   RANVAR, RANVAR, 1 + DICE (16), RANVAR, MAXTAB - 1, RANVAR,
		   DICE (MAXTAB));
	  if (DICE (8) == 0)
	    fprintf (f, " dynstep++;\n");
	  break;
	case 2:
	  fprintf (f,
		   " if (%c > %c + %d) %c++; else %c=(tab[%d] + %c) & 0xffffff;\n",
		   RANVAR, RANVAR, DICE (8), RANVAR, RANVAR, DICE (MAXTAB),
		   RANVAR);
	  if (DICE (8) == 0)
	    fprintf (f, " dynstep++;\n");
	  break;
	case 3:
	  fprintf (f, " tab[%c & %d] += %d + ((%c - %c) & 0xf); %c++;\n",
		   RANVAR, MAXTAB - 1, DICE (8) + 2, RANVAR, RANVAR, RANVAR);
	  if (DICE (8) == 0)
	    fprintf (f, " dynstep++;\n");
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
	  if (DICE (8) == 0)
	    fprintf (f, " %c++, %c++;\n", RANVAR, RANVAR);
	  if (DICE (16) == 0)
	    fprintf (f, " %c += (%c &0xfffff);\n", RANVAR, RANVAR);
	  if (DICE (16) == 0)
	    fprintf (f, " %c -= (1+ %c&0x7ffff);\n", RANVAR, RANVAR);
	  if (DICE (8) == 0)
	    {
	      fprintf (f, " %c++, %c++;\n", RANVAR, RANVAR);
	      fprintf (f, " dynstep++;\n");
	    };
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
	  }
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
	  {
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
}


void
waitdeferred (int rank, double tstart, pid_t pid, int deferredl,
	      const char *cmd)
{
  if (pid == 0 || !cmd)
    return;
  int waitcnt = 0;
  int wstat = 0;
  pid_t wpid = 0;
  struct rusage rusw = { };
  fflush (NULL);
  // if the previous compilation is still running, wait for it.
  do
    {
      memset (&rusw, 0, sizeof (rusw));
      wpid = wait4 (pid, &wstat, WEXITED, &rusw);
      waitcnt++;
    }
  while (wstat == 0 && wpid != pid && waitcnt < 4);
  double tim_end = my_clock (CLOCK_MONOTONIC);
  double usertime =
    1.0 * rusw.ru_utime.tv_sec + 1.0e-6 * rusw.ru_utime.tv_usec;
  double systime =
    1.0 * rusw.ru_stime.tv_sec + 1.0e-6 * rusw.ru_stime.tv_usec;
  if (timedataf)
    fprintf (timedataf,
	     "NM='%s' IX=%d SZ=%d ELAPSED=%.3f UCPU=%.4f SCPU=%.4f\n",
	     namarr[rank], rank, deferredl, tim_end - tstart,
	     1.0 * (usertime) / sysconf (_SC_CLK_TCK),
	     1.0 * (systime) / sysconf (_SC_CLK_TCK));
  printf
    ("deferred %s in pid %d for %d instr: elapsed %.3f, CPU %.4f usr + %.4f sys seconds\n",
     cmd, (int) pid, deferredl, tim_end - tstart, usertime, systime);
}				/* end waitdeferred */


int
main (int argc, char **argv)
{
  progname = argv[0];
  int k = 0, l = 0;
  long suml = 0;
  int r = 0, s = 0;
  long nbcall = 0, n = 0;
  FILE *map = NULL;		/* the /proc/self/maps pseudofile (Linux) */
  char buf[100] = { (char) 0 };	/* buffer for name */
  char cmd[100] = { (char) 0 };	/* buffer for command */
  char defercmd[100] = { (char) 0 };	/* buffer for deferred command */
  int deferredl = 0;
  int deferredix = 0;
  char linbuf[500] = { (char) 0 };	/* line buffer for map */
  char *cc = NULL;		/* the CC command or gcc (from environment) */
  char *cflags = NULL;		/* the CFLAGS option or -O (from environment)  */
  void **hdlarr = NULL;		/* array of dlopened handles */
  struct utsname uts = { };	/* for uname ie system version */
  struct tms t_init = { }, t_generate = { }, t_load = { }, t_run = { };
  clock_t cl_init = 0, cl_generate = 0, cl_load = 0, cl_run = 0;
  double tim_cpu_generate = 0.0, tim_real_generate = 0.0;
  double tim_cpu_load = 0.0, tim_real_load = 0.0;
  double tim_cpu_run = 0.0, tim_real_run = 0.0;
  pid_t deferredpid = 0;
  double tim_deferred = 0;
  if (argc > 1 && !strcmp (argv[1], "--help"))
    {
      printf ("usage: %s [maxcnt [meanlen]]\n"
	      "\t\t (a program generating many plugins for dlopen on Linux)\n"
	      "\t where maxcnt (default %d) is the count of generated functions and plugins.\n"
	      "\t where meanlen (default %d) is related to the mean length of generated functions\n"
	      "\t (a data file, usable by GNU plot, is generated and named data_manydl_<maxcnt>_<meanlen>_p<pid>.dat)\n",
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
  char timedataname[64];
  memset (timedataname, 0, sizeof (timedataname));
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

  snprintf (timedataname, sizeof (timedataname),
	    "data_manydl_%d_%d_p%d.dat", maxcnt, meanlen, (int) getpid ());
  timedataf = fopen (timedataname, "w");
  if (!timedataf)
    {
      perror (timedataname);
      exit (EXIT_FAILURE);
    };
  gethostname (myhostname, sizeof (myhostname) - 1);
  /* ask for system information */
  memset (&uts, 0, sizeof (uts));
  uname (&uts);			/* don't bother checking success */
  {
    char cwdbuf[200];
    memset (cwdbuf, 0, sizeof (cwdbuf));
    if (!getcwd (cwdbuf, sizeof (cwdbuf)))
      {
	perror ("getcwd");
	strcpy (cwdbuf, "./");
      };

    fprintf (timedataf, "# file %s in %s on %s git %s\n", timedataname,
	     cwdbuf, myhostname, manydl_git);
    fflush (timedataf);
    /* print counter, system, & library information */
    printf
      ("Running %s, max.counter %d, mean len %d, glibc version %s release %s\n"
       "on sys %s release %s version %s in %s\n", argv[0], maxcnt, meanlen,
       gnu_get_libc_version (), gnu_get_libc_release (), uts.sysname,
       uts.release, uts.version, cwdbuf);
  }
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
      /* generate a name like genf_C_9 or genf_A_0 and duplicate it */
      memset (buf, 0, sizeof (buf));
      snprintf (buf, sizeof (buf) - 1, "genf_%c_%d",
		"ABCDEFGHIJKLMNOPQ"[k % 16], k / 16);
      namarr[k] = strdup (buf);
      if (!namarr[k])
	{
	  perror ("strdup");
	  exit (EXIT_FAILURE);
	};
      printf ("generating %s; ", namarr[k]);
      fflush (stdout);
      /* generate and compile the file */
      l = generate_file (namarr[k], meanlen);
      fflush (stdout);
      double tstartcompil = my_clock (CLOCK_MONOTONIC);
      memset (cmd, 0, sizeof (cmd));
      snprintf (cmd, sizeof (cmd) - 1,
		"%s -fPIC -shared %s %s.c -o %s.so",
		cc, cflags, namarr[k], namarr[k]);
      if (k % 2 == 0)
	{
	  // immediate compilation 
	  printf ("compiling %4d instructions", l);
	  struct tms my_start_tms = { };
	  struct tms my_end_tms = { };
	  if (times (&my_start_tms) < 0)
	    perror ("times");
	  if (system (cmd))
	    {
	      fprintf (stderr, "\ncompilation %s #%d failed\n", cmd, k + 1);
	      exit (EXIT_FAILURE);
	    };
	  if (times (&my_end_tms) < 0)
	    perror ("times");
	  double tendcompil = my_clock (CLOCK_MONOTONIC);
	  printf (" in %.4f elapsed seconds.", tendcompil - tstartcompil);
	  putchar ('\n');
	  fflush (stdout);
	  if (timedataf)
	    fprintf (timedataf,
		     "NM='%s' IX=%d SZ=%d ELAPSED=%.3f UCPU=%.4f SCPU=%.4f\n",
		     namarr[k], k, l, tendcompil - tstartcompil,
		     1.0 * (my_end_tms.tms_cutime -
			    my_start_tms.tms_cutime) / sysconf (_SC_CLK_TCK),
		     1.0 * (my_end_tms.tms_cstime -
			    my_start_tms.tms_cstime) / sysconf (_SC_CLK_TCK));
	}
      else
	{
	  // deferred compilation
	  printf (" - deferring %s\n", cmd);
	  fflush (NULL);
	  if (deferredpid > 0)
	    {
	      // wait for the previous deferred compilation!
	      waitdeferred (deferredix, tim_deferred, deferredpid, deferredl,
			    defercmd);
	      deferredpid = 0;
	    };
	  fflush (NULL);
	  memset (defercmd, 0, sizeof (defercmd));
	  strncpy (defercmd, cmd, sizeof (defercmd));
	  deferredl = l;
	  deferredix = k;
	  tim_deferred = my_clock (CLOCK_MONOTONIC);
	  deferredpid = fork ();
	  if (deferredpid == 0)
	    {			/* child process */
	      close (STDIN_FILENO);
	      execl ("/bin/sh", "/bin/sh", "-c", cmd, NULL);
	      perror ("execl-sh");
	    }
	  else
	    usleep (1000);
	}
      if ((k + DICE (8)) % 6 == 0)
	sync ();
      suml += l;
      if (k % 64 == 32)
	printf
	  ("°after %d generated & compiled files (%ld instrs) time\n"
	   "° .. %s [sec]\n", k + 1, suml, timestring ());
      if (k % 32 == 0)
	fflush (timedataf);
    };				/* end for k loop */
  putchar ('.');
  putchar ('\n');
  fflush (NULL);
  if (deferredpid > 0)
    {
      usleep (1000);
      waitdeferred (k, tim_deferred, deferredpid, deferredl, defercmd);
      deferredpid = 0;
    };
  fprintf (timedataf, "# end of file %s\n", timedataname);
  fclose (timedataf), timedataf = NULL;
  suml += l;
  if (k % 64 == 32)
    printf
      ("°after %d generated & compiled files (%ld instrs) time\n"
       "° .. %s [sec]\n", k + 1, suml, timestring ());

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
  sleep (1);
  /* now all files are generated, and compiled, each into a shared
     library *.so; we load all the libraries with dlopen */
  printf ("\n generated %ld instructions in %d files\n", suml, k);
#ifdef __FRAMAC__
  extern void initialize_funarr_framac (void);
  initialize_funarr_framac ();
#else /*!__FRAMAC__ */
  {
    FILE *genframac = fopen ("genf_framac.c", "w");
    if (!genframac)
      {
	perror ("genf_framac.c");
	exit (EXIT_FAILURE);
      }
    fprintf (genframac, "/* generated file genf_framac.c for Frama-C */\n");
    fprintf (genframac, "typedef int (*funptr_t) (int, int);\n");
    fprintf (genframac, "extern funptr_t*funarr;\n");
    //
    for (k = 0; k < maxcnt; k++)
      {
	fprintf (genframac, "extern int %s(int, int);\n", namarr[k]);
      };

    fprintf (genframac, "extern void initialize_funarr_framac(void);\n");
    fprintf (genframac, "void initialize_funarr_framac(void)\n{\n");
    for (k = 0; k < maxcnt; k++)
      fprintf (genframac, "  funarr[%d] = %s;\n", k, namarr[k]);
    fprintf (genframac, "} /*end generated initialize_funarr_framac*/\n");
    fflush (genframac);
    fprintf (genframac,
	     "// end of generated file genf_framac.c for Frama-C */\n");
    fclose (genframac);
  };
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
#endif /*__FRAMAC__*/
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
  fprintf (stderr,
	   "dynload time per file: %.3f cpu %.3f real seconds/file\n",
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
	printf ("after %ld calls %s"
#ifdef MANYDL_GIT
		" git " MANYDL_GIT
#endif
		" time \n" " .. [sec]\n", n + 1, timestring ());
    };

  {
    // copying /proc/self/maps to some genf_manydl_p<pid>.map file
    FILE *fselfmap = fopen ("/proc/self/maps", "r");
    if (!fselfmap)
      {
	perror ("/proc/self/maps");
	exit (EXIT_FAILURE);
      };
    char mapname[64];
    memset (mapname, 0, sizeof (mapname));
#ifdef  MANYDL_GIT
    snprintf (mapname, sizeof (mapname),
	      "genfmap_%.s_p%d.map", MANYDL_GIT, (int) getpid ());
#else
    snprintf (mapname, sizeof (mapname), "genfmap_p%d.map", (int) getpid ());
#endif
    FILE *fmapcopy = fopen (mapname, "w");
    if (!fmapcopy)
      {
	perror (mapname);
	exit (EXIT_FAILURE);
      };
    do
      {
	char linbuf[128];
	memset (linbuf, 0, sizeof (linbuf));
	if (!fgets (linbuf, sizeof (linbuf) - 1, fselfmap))
	  break;
	fputs (linbuf, fmapcopy);
      }
    while (!feof (fselfmap));
    fclose (fselfmap);
    fclose (fmapcopy);
    printf ("%s: copied /proc/self/maps to %s\n", progname, mapname);
  }

  memset (&t_run, 0, sizeof (t_run));
  cl_run = times (&t_run);
  tim_cpu_run =
    secpertick * (t_run.tms_cutime + t_run.tms_cstime
		  + t_run.tms_utime + t_run.tms_stime)
    - secpertick * (t_load.tms_cutime + t_load.tms_cstime
		    + t_load.tms_utime + t_load.tms_stime);
  tim_real_run = secpertick * (cl_run - cl_load);
  printf
    ("\n°%s: made %ld calls to %d functions r=%d s=%d dynstep=%ld "
     "tab=%d %d %d %d %d %d %d %d...\n... %s\n",
     progname, nbcall, maxcnt, r, s, dynstep,
     tab[0], tab[1], tab[2], tab[3], tab[4], tab[5], tab[6], tab[7],
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
    ("\n%s: total time for %d files %ld instructions %ld executed steps %ld calls:\n ... %s sec\n source %s\n",
     progname, maxcnt, suml, dynstep, nbcall, timestring (),
     __FILE__ " " __DATE__ "@" __TIME__
#ifdef MANYDL_GIT
     " git " MANYDL_GIT
#endif
    );
  return 0;
}


/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make manydl" ;;
 ** End: ;;
 ****************/
