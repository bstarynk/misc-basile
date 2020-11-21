/* File onion-print.c from https://github.com/bstarynk/misc-basile/
 *
 * A small web server application to print files using lp
   
   © Copyright Basile Starynkevitch 2020
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


/*** This uses libonion from github.com/davidmoreno/onion/ ****/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <argp.h>
#include <ctype.h>

#include <onion/onion.h>
#include <onion/request.h>
#include <onion/response.h>
#include <onion/handler.h>
#include <onion/log.h>
#include <onion/shortcuts.h>

#include <onion/exportlocal.h>
#include <onion/path.h>
#include <onion/static.h>

////////////////////////////////////////////////////////////////
///// parsing program options
enum rps_progoption_en
{
  OP_PROGOPT__NONE = 0,
  OP_PROGOPT_WEB_SERVER_HOST = 'H',
  OP_PROGOPT_WEB_SERVER_PORT = 'P',
  OP_PROGOPT_PRINT_COMMAND = 1000,
  OP_PROGOPT_PRINT_MEGABYTE_LIMIT,
  OP_PROGOPT_VERSION,
};

onion *OP_onion = NULL;
const char *OP_webhost;
const char *OP_dirname;
const char *OP_port;




struct argp_option OP_progoptions[] = {
  /* ======= web server host ======= */
  { /*name: */ "web-host",	///
						/*key: */ OP_PROGOPT_WEB_SERVER_HOST,
						///
				/*arg: */ "WEBHOST",
				///
				/*flags: */ 0,
				///
						/*doc: */ "Run web server host name",
						//
				/*group: */ 0
				///
   },
  /* ======= web server host ======= */
  { /*name: */ "web-port",	///
						/*key: */ OP_PROGOPT_WEB_SERVER_PORT,
						///
				/*arg: */ "WEBPORT",
				///
				/*flags: */ 0,
				///
						/*doc: */ "Run web server port name",
						//
				/*group: */ 0
				///
   },
  /* ======= printing command ======= */
  { /*name: */ "print-command",	///
					/*key: */ OP_PROGOPT_PRINT_COMMAND,
					///
				/*arg: */ "PRINTCOMMAND",
				///
				/*flags: */ 0,
				///
					/*doc: */ "The printing command",
					//
				/*group: */ 0
				///
   },
  /* ======= printed file size limit ======= */
  { /*name: */ "print-limit",	///
					/*key: */ OP_PROGOPT_PRINT_COMMAND,
					///
				/*arg: */ "SIZELIMIT",
				///
				/*flags: */ 0,
				///
								/*doc: */ "The printed file size limit, in megabytes",
								//
				/*group: */ 0
				///
   },
  /* ======= terminating empty option ======= */
  { /*name: */ (const char *) 0,	///
				/*key: */ 0,
				///
				/*arg: */ (const char *) 0,
				///
				/*flags: */ 0,
				///
				/*doc: */ (const char *) 0,
				///
				/*group: */ 0
				///
   }
};				/* end OP_progoptions */


// Parse a single program option, skipping side effects when state is empty.
error_t
OP_parse1opt (int key, char *arg, struct argp_state *state)
{
  bool side_effect = state != NULL;
  switch (key)
    {
      /// --web-host=WEBHOST
    case OP_PROGOPT_WEB_SERVER_HOST:
      OP_webhost = arg;
      return 0;
    }
  return ARGP_ERR_UNKNOWN;
}

void
OP_parse_program_arguments (int argc, char **restrict argv)
{
  errno = 0;
  struct argp_state argstate;
  memset (&argstate, 0, sizeof (argstate));
  static struct argp argparser;
  argparser.options = OP_progoptions;
  argparser.parser = OP_parse1opt;
  argparser.args_doc = " ; # ";
  argparser.doc =
    "onion-printer: A tiny web server to redirect printing of PDF files\n."
    "GPLv3+ licenses.  You should have received a copy of the GNU General Public License\n"
    "along with this program.  If not, see https://www.gnu.org/licenses\n"
    "**NO WARRANTY, not even for FITNESS FOR A PARTICULAR PURPOSE**\n"
    "+++ use at your own risk +++\n" "\n Accepted program options are:\n";
  argparser.children = NULL;
  argparser.help_filter = NULL;
  argparser.argp_domain = NULL;
  if (argp_parse (&argparser, argc, argv, 0, NULL, NULL))
    {
      fprintf (stderr,
	       "%s: failed to parse program arguments. Try %s --help\n",
	       argv[0], argv[0]);
      exit (EXIT_FAILURE);
    }
}				/* end OP_parse_program_arguments */



void
OP_free_onion (int unused)
{
  onion_free (OP_onion);
  exit (0);
}

typedef struct
{
  const char *abspath;
} upload_file_data;

onion_connection_status
upload_file (upload_file_data * data,
	     onion_request * req, onion_response * res)
{
  if (onion_request_get_flags (req) & OR_POST)
    {
      const char *name = onion_request_get_post (req, "file");
      const char *filename = onion_request_get_file (req, "file");

      if (name && filename)
	{
	  char finalname[1024];
	  snprintf (finalname, sizeof (finalname), "%s/%s", data->abspath,
		    name);
	  ONION_DEBUG ("Copying from %s to %s", filename, finalname);

	  onion_shortcut_rename (filename, finalname);
	}
    }
  return OCS_NOT_PROCESSED;	// I just ignore the request, but process over the FILE data
}

/// Footer that allows to upload files.
void
upload_file_footer (onion_response * res, const char *dirname)
{
  onion_response_write0 (res,
			 "<p><table><tr><th>Upload a file: <form method=\"POST\" enctype=\"multipart/form-data\">"
			 "<input type=\"file\" name=\"file\"><input type=\"submit\"><form></th></tr></table>");
  onion_response_write0 (res,
			 "<h2>Onion minimal fileserver. (C) 2010 <a href=\"http://www.coralbits.com\">CoralBits</a>. "
			 "Under <a href=\"http://www.gnu.org/licenses/agpl-3.0.html\">AGPL 3.0.</a> License.</h2>\n");
}

int
show_help ()
{
  printf ("Onion basic fileserver. (C) 2010 Coralbits S.L.\n\n"
	  "Usage: fileserver [options] [directory to serve]\n\n"
	  "Options:\n"
	  "  --pem pemfile   Uses that certificate and key file. Both must be on the same file. Default is at current dir cert.pem.\n"
	  "  --pam pamname   Uses that pam name to allow access. Default login.\n"
	  "  --port N\n"
	  "   -p N           Listens at given port. Default 8080\n"
	  "  --listen ADDRESS\n"
	  "   -l ADDRESS     Listen to that address. It can be a IPv4 or IPv6 IP, or a local host name. Default: 0.0.0.0\n"
	  "  --help\n"
	  "   -h             Shows this help\n"
	  "\n"
	  "fileserver serves an HTML with easy access to current files, and allows to upload new files.\n"
	  "It have SSL encrypting and PAM authentication.\n" "\n");
  return 0;
}

int
main (int argc, char **restrict argv)
{


  OP_parse_program_arguments (argc, argv);

  upload_file_data data = {
    OP_dirname
  };
  onion_handler *root =
    onion_handler_new ((void *) upload_file, (void *) &data, NULL);
  onion_handler *dir =
    onion_handler_export_local_new (argc == 2 ? argv[1] : ".");
  onion_handler_export_local_set_footer (dir, upload_file_footer);
  onion_handler_add (dir,
		     onion_handler_static ("<h1>404 - File not found.</h1>",
					   404));
  onion_handler_add (root, dir);
  OP_onion = onion_new (O_THREADED);

  if (OP_port)
    onion_set_port (OP_onion, OP_port);
  if (OP_webhost)
    onion_set_hostname (OP_onion, OP_webhost);

  signal (SIGINT, OP_free_onion);
  int error = onion_listen (OP_onion);
  if (error)
    {
      perror ("Cant create the server");
    }

  onion_free (OP_onion);

  return 0;
}

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "gcc -o onion-print -Wall -Wextra -I/usr/local/include -O -g onion-print.c -L/usr/local/lib -Bstatic -lonion" ;;
 ** End: ;;
 ****************/
