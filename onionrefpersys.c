
/**
  onionrefpersys.c:
  Copyright (C) 2023 Basile Starynkevitch

  This onionrefpersys is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2.0 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GPL licence, if not see
  <http://www.gnu.org/licenses/>
*/


#include <features.h>
#include <getopt.h>
#include <onion/onion.h>
#include <onion/log.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>
#include <stdio.h>
#include <gnu/libc-version.h>

onion *my_onion;

const char *progname;

const char gitid[] = GITID;

bool debug = false;

const char *web_port = "8080";
const char *web_host = "localhost";

const struct option options_arr[] = {
  {.name = (const char *) "debug",	//
   .has_arg = no_argument,	//
   .flag = (int *) NULL,	//
   .val = 'D'},			//
  {.name = (const char *) "help",	//
   .has_arg = no_argument,	//
   .flag = (int *) NULL,	//
   .val = 'h'},			//
  {.name = (const char *) "version",	//
   .has_arg = no_argument,	//
   .flag = (int *) NULL,	//
   .val = 'V'},			//
  {.name = (const char *) "port",	//
   .has_arg = required_argument,	//
   .flag = (int *) NULL,	//
   .val = 'P'},			//
  {.name = (const char *) "host",	//
   .has_arg = required_argument,	//
   .flag = (int *) NULL,	//
   .val = 'H'},			//
  {.name = (const char *) NULL,	//
   .has_arg = no_argument,	//
   .flag = (int *) NULL,	//
   .val = 0}			//
};

void parse_options (int argc, char **argv);

void show_usage (void);

////////////////////////////////////////////////////////////////

/// keep this function consistent with show_usage
void
parse_options (int argc, char **argv)
{
  while (1)
    {
      int option_index = 0;
      int c = 0;
      c = getopt_long (argc, argv, "DP:H:hV", options_arr, &option_index);
      if (c < 0)
	break;
      switch (c)
	{
	case 'D':		// --debug
	  debug = true;
	  break;
	case 'P':
	  web_port = optarg;
	  break;
	case 'h':
	  show_usage ();
	  break;
	case 'V':
	  printf ("%s version git %s built %s\n",
		  progname, gitid, __DATE__ "@" __TIME__);
	  exit (EXIT_SUCCESS);
	  break;
	case 'H':
	  web_host = optarg;
	  break;
	default:
	  break;
	}
    }
}				/* end parse_options */

/// keep the show_usage consistent with parse_options
void
show_usage (void)
{
  printf ("%s usage (a specialized webserver)\n", progname);
  printf ("see %s file (GPLv3+ licensed, no warranty)\n", __FILE__);
  printf ("\t --debug | -D                 # enable debugging\n");
  printf ("\t --version | -V               # show version\n");
  printf ("\t --help | -h                  # show this help\n");
  printf ("\t --port | -P <webport>        # set HTTP port\n");
  printf ("\t --host | -H <webhost>        # set HTTP host\n");
}				/* end show_usage */

int
main (int argc, char **argv)
{
  progname = argv[0];
  parse_options (argc, argv);
  my_onion = onion_new (O_THREADED);
  onion_set_port (my_onion, web_port);
  onion_set_hostname (my_onion, web_host);
}				/* end main */
