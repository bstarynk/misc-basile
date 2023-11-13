// file testclockgetime.c
/// MIT licensed
/// copyright Basile Starynkevitch 2023
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

int
main (int argc, char **argv)
{
  struct timespec tstartreal = { 0, 0 }, tendreal = { 0, 0 };
  struct timespec tstartcpu = { 0, 0 }, tendcpu = { 0, 0 };
  clock_gettime (CLOCK_MONOTONIC, &tstartreal);
  clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &tstartcpu);
  double dstartreal = tstartreal.tv_sec * 1.0 + 1.0e-9 * tstartreal.tv_nsec;
  double dstartcpu = tstartcpu.tv_sec * 1.0 + 1.0e-9 * tstartcpu.tv_nsec;
  long cnt = (argc > 1) ? atol (argv[1]) : 0;
  if (cnt < 1000)
    cnt = 1000;
  long nbt = 0;
  for (long i = 0; i < cnt; i++)
    {
      struct timespec tnow = { 0, 0 };
      clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &tnow);
      if (tnow.tv_nsec < 1000)
	nbt++;
    };
  clock_gettime (CLOCK_MONOTONIC, &tendreal);
  clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &tendcpu);
  double dendreal = tendreal.tv_sec * 1.0 + 1.0e-9 * tendreal.tv_nsec;
  double dendcpu = tendreal.tv_sec * 1.0 + 1.0e-9 * tendcpu.tv_nsec;
  printf ("%s ending in %g real %g cpu sec, nbt=%ld count=%ld\n"
	  "... %g real %g cpu sec/iter\n",
	  argv[0], (dendreal - dstartreal), (dendcpu - dstartcpu), nbt, cnt,
	  (dendreal - dstartreal) / cnt, (dendcpu - dstartcpu) / cnt);
  exit (0);
}

/// eof testfclockgetime.c
