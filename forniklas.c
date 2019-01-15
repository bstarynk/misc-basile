// file forniklas.c
// in the public domain

/***
This small program for Linux generates the C code of some tiny plugin
containing a single function (of two integer arguments and of integer
result), compiles it as a plugin, loads that plugin and its function,
and apply that dynamically provided function to two given numbers.  It
illustrates that a C program can extend itself on Linux.

 compile that file on Linux/x86-64 as 
    gcc -Wall -g -rdynamic forniklas.c -ldl -o forniklas

 then run it as
    ./forniklas SUM 'return x+y;' 3 5

a more interesting example involves calling the somefun function,
defined in this very forniklas.c file, so:
    ./forniklas FOO 'return somefun(x)-y;' 10 12


it should create some C code in some temporary file defining the SUM
function, compile that code into a plugin, load that plugin, get the
SUM function in it, and invoke that with 3 and 5, then print the
result 8.
***/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <dlfcn.h>

#define MAXFUNAME_LEN 64

// the signature of the generated function
typedef int funsigt (int, int);

// the somefun function is only useful inside the plugin
int
somefun (int u)
{
  int v = 5 * u + 2;
  printf ("somefun u=%d gives v=%d\n", u, v);
  return v;
}				/* end of somefun */

void
emit_c_code (const char *srcpath, const char *funame, const char *fubody)
{
  FILE *out = fopen (srcpath, "w");
  if (!out)
    {
      fprintf (stderr, "failed to fopen %s - %s\n", srcpath,
	       strerror (errno));
      exit (EXIT_FAILURE);
    }
  fprintf (out, "// generated %s file - DONT EDIT\n", srcpath);
  fputs ("#include <stdio.h>\n", out);
  fputs ("extern int somefun(int); // defined in the main program\n\n", out);
  fprintf (out, "int %s (int x, int y) {\n", funame);
  fprintf (out, "  printf(\"start of %s: x=%%d, y=%%d\\n\", x, y);\n",
	   funame);
  fprintf (out, "//inserted body:\n"
	   "%s\n" "//end of inserted body\n", fubody);
  fprintf (out, "} /* end of %s */\n", funame);
  fprintf (out, "/// end of generated %s file\n\n", srcpath);
  fclose (out);
}				/* end emit_c_code */


int
main (int argc, char **argv)
{
  if (argc != 5)
    {
      fprintf (stderr,
	       "%s needs four arguments: %s <funame> <fubodfy> <intx> <inty>\n",
	       argv[0], argv[0]);
      exit (EXIT_FAILURE);
    };
  const char *funame = argv[1];
  const char *fubody = argv[2];
  int xx = atoi (argv[3]);
  int yy = atoi (argv[4]);
  for (const char *pc = funame; *pc; pc++)
    if (!isalpha (*pc))
      {
	fprintf (stderr, "bad function name %s\n", funame);
	exit (EXIT_FAILURE);
      }
  if (strlen (funame) > MAXFUNAME_LEN)
    {
      fprintf (stderr, "too long function name %s\n", funame);
      exit (EXIT_FAILURE);
    }
  char srcpath[2 * MAXFUNAME_LEN];
  snprintf (srcpath, sizeof (srcpath), "/tmp/code_%s.c", funame);
  emit_c_code (srcpath, funame, fubody);
  char pluginpath[2 * MAXFUNAME_LEN];
  snprintf (pluginpath, sizeof (pluginpath), "/tmp/plugin_%s.so", funame);
  char cmdbuf[2 * MAXFUNAME_LEN + 200];
  snprintf (cmdbuf, sizeof (cmdbuf), "gcc -fPIC -shared -Wall -g %s -o %s",
	    srcpath, pluginpath);
  printf ("running %s\n", cmdbuf);
  fflush (NULL);
  int rc = system (cmdbuf);
  if (rc != 0)
    {
      fprintf (stderr, "failed to run %s : got %d\n", cmdbuf, rc);
      exit (EXIT_FAILURE);
    }
  char *pluginh = dlopen (pluginpath, RTLD_NOW | RTLD_GLOBAL);
  if (!pluginh)
    {
      fprintf (stderr, "dlopen %s failed: %s\n", pluginpath, dlerror ());
      exit (EXIT_FAILURE);
    }
  funsigt *funptr = dlsym (pluginh, funame);
  if (!funptr)
    {
      fprintf (stderr, "dlsym %s in %s failed: %s\n", funame, pluginpath,
	       dlerror ());
      exit (EXIT_FAILURE);
    }
  printf ("dlopen-ed %s plugin and got %s function @%p,\n"
	  ".. calling it with xx=%d yy=%d\n",
	  pluginpath, funame, (void *) funptr, xx, yy);
  fflush (NULL);
  int res = (*funptr) (xx, yy);
  printf ("result of %s (%d, %d) is %d\n", funame, xx, yy, res);
  snprintf (cmdbuf, sizeof (cmdbuf), "pmap %d", (int) getpid ());
  fflush (NULL);
  system (cmdbuf);
  printf ("(don't forget to remove generated source %s and plugin %s)\n",
	  srcpath, pluginpath);
  return 0;
}				/* end of main */
