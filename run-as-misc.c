/* file run-as-misc.c from https://github.com/bstarynk/misc-basile/
   SPDX-License-Identifier: GPL-3.0-or-later
   run a given command as some other MISC user

   © Copyright Basile Starynkevitch 2020
   program released under GNU general public license v3+

   this is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   this is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sysexits.h>
#include <errno.h>
#include <pwd.h>

#ifndef MISC_USER_ID
#error MISC_USER_ID should be defined to a uid
#endif /*MISC_USER_ID */

#ifndef GITID
#error GITID should be defined to a string
#endif /*GITID*/
  int
main (int argc, char **argv)
{
  if (argc > 1 && !strcmp (argv[1], "--version"))
    {
      printf ("%s: version git %s built on " __DATE__ " at " __TIME__
	      "\n", argv[0], GITID);
      printf ("\t see " __FILE__
	      " source code on https://github.com/bstarynk/misc-basile/\n");
      printf ("\t this is a GPLv3+ licensed free software - NO WARRANTY\n");
      printf ("\t by Basile Starynkevitch (France)\n");
      printf
	("\t see license on https://www.gnu.org/licenses/gpl-3.0.html\n");
      exit (EXIT_SUCCESS);
    };
  if (argc > 1 && !strcmp (argv[1], "--help"))
    {
      printf ("%s runs the rest of the command as the user of uid %d,\n"
	      "\t... or as the user given by $RUN_MISC_USER environment variable.\n",
	      argv[0], MISC_USER_ID);
      printf ("\t for example, try %s /bin/id\n", argv[0]);
      printf ("\t this is a GPLv3+ licensed free software - NO WARRANTY\n");
      printf
	("\t by Basile Starynkevitch <basile@starynkevitch.net> (France)\n");
      printf
	("\t source code on https://github.com/bstarynk/misc-basile/ file "
	 __FILE__ "\n");
      printf
	("\t see license on https://www.gnu.org/licenses/gpl-3.0.html\n");
      printf ("\t argument --version gives version information.\n");
      printf ("\t argument --help gives this help.\n");
      exit (EXIT_SUCCESS);
    };
  if (argc <= 1)
    {
      fprintf (stderr, "%s requires some argument. Try %s --help\n",
	       argv[0], argv[0]);
      exit (EXIT_FAILURE);
    }
  openlog (argv[0], LOG_PERROR | LOG_PID, LOG_USER);
  uid_t miscuid = MISC_USER_ID;
  const char *miscuser = getenv ("RUN_MISC_USER");
  if (miscuser)
    {
      struct passwd *pwd = getpwnam (miscuser);
      if (!pwd)
	{
	  syslog (LOG_ERR, "%s/%d: getpwnam failed on %s - %m",
		  __FILE__, __LINE__, miscuser);
	  exit (EX_OSERR);
	}
      else
	miscuid = pwd->pw_uid;
    };
  time_t nowt = time (NULL);
  if (seteuid (miscuid))
    {
      syslog (LOG_WARNING, "%s: seteuid(%d) failed at %s - %m", argv[0],
	      miscuid, ctime (&nowt));
      exit (EX_OSERR);
    };
  if (setuid(miscuid))
    {
      syslog (LOG_WARNING, "%s: setuid(%d) failed at %s - %m", argv[0],
	      miscuid, ctime (&nowt));
      exit (EX_OSERR);
    }
  syslog(LOG_INFO, "%s: miscuid=%d, uid#%d, euid#%d", argv[0],
	 miscuid, getuid(), geteuid());
  execvp (argv[1], argv + 1);	/// usually does not return
  int er = errno;
  static char buf[1024];
  char *pc = buf;
  const char *end = buf + sizeof (buf) - 2;
  for (int ix = 1; ix < argc && pc < end; ix++)
    {
      if (ix > 1)
	*(pc++) = ' ';
      snprintf (buf, end - buf, "%s", argv[ix]);
      pc += strlen (buf);
    };
  syslog (LOG_ERR, "%s: execvp failed (%s) for %s\n", argv[0], strerror (er),
	  buf);
  exit (EX_OSERR);
}				/* end main */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./compile-run-as-misc.sh" ;;
 ** End: ;;
 ****************/
