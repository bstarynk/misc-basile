// file redis-scan.c from https://github.com/bstarynk/misc-basile/
// it scan all the keys in a REDIS database and prints them on stdout.
// it is mostly an exercise to understand how SCAN works with HIREDIS

/** Copyright (C)  2015  Basile Starynkevitch
    redis-scan is showing how to get all the keys in a REDIS database
    using the SCAN command.  See http://redis.io/ for more about REDIS.

    redis-scan is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3, or (at your option)
    any later version.

    redis-scan is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with redis-scan; see the file COPYING3.   If not see
    <http://www.gnu.org/licenses/>.
***/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <hiredis/hiredis.h>

const char *reply_types[] = {
  [0] NULL,
  [REDIS_REPLY_STRING] "STRING",
  [REDIS_REPLY_ARRAY] "ARRAY",
  [REDIS_REPLY_INTEGER] "INTEGER",
  [REDIS_REPLY_NIL] "NIL",
  [REDIS_REPLY_STATUS] "STATUS",
  [REDIS_REPLY_ERROR] "ERROR",
  NULL
};

#define logprintf(Fmt,...) do { fprintf(stderr, "%s:%d: " Fmt, __FILE__, __LINE__, \
					##__VA_ARGS__); } while(0)
int
main (int argc, char **argv)
{
  const char *servhostname = "localhost";
  int port = 6379;
  redisContext *redc = NULL;
  redisReply *reply = NULL;
  if (argc > 1 && !strcmp (argv[1], "--help"))
    {
      printf ("usage: %s [servhostname [port]]\n", argv[0]);
      printf ("\t default servhostname: %s\n", servhostname);
      printf ("\t default port: %d\n", port);
      printf
	("a program to scan all the keys from a REDIS database (which is filled with a few keys before)\n");
      exit (EXIT_SUCCESS);
    }
  else if (argc > 1 && !strcmp (argv[1], "--version"))
    {
      printf ("no version info for %s built %s\n",
	      argv[0], __FILE__ " " __DATE__ "@" __TIME__);
      exit (EXIT_FAILURE);
    }
  if (argc > 1)
    servhostname = argv[1];
  if (argc > 2)
    port = atoi (argv[2]);
  struct timeval timeout = { 1, 500000 };	// 1.5 seconds
  redc = redisConnectWithTimeout (servhostname, port, timeout);
  if (redc == NULL || redc->err)
    {
      if (redc)
	{
	  logprintf ("Connection error to %s:%d : %s\n", servhostname, port,
		     redc->errstr);
	  redisFree (redc);
	}
      else
	{
	  logprintf
	    ("Connection to %s:%d: can't allocate redis context (%m)\n",
	     servhostname, port);
	}
      exit (EXIT_FAILURE);
    }
  logprintf ("Connected to REDIS at %s:%d\n", servhostname, port);
  /* PING server */
  reply = redisCommand (redc, "PING");
  logprintf ("PING: %s\n", reply ? (reply->str) : "**none**");
  freeReplyObject (reply);
  /* Set a few keys */
  reply = redisCommand (redc, "SET %s %s", "foo", "hello world");
  logprintf ("SET foo: %s\n", reply ? (reply->str) : "**none**");
  freeReplyObject (reply);
  /* we set $CLIENTPID before $CLIENTHOSTNAME, even if their
     alphanumerical order is the other way round... */
  reply = redisCommand (redc, "SET $CLIENTPID %d", (int) getpid ());
  logprintf ("SET $CLIENTPID %d: %s\n", (int) getpid (),
	     reply ? (reply->str) : "**none**");
  freeReplyObject (reply);
  {
    char myhostname[80];
    memset (myhostname, 0, sizeof (myhostname));
    gethostname (myhostname, sizeof (myhostname) - 1);
    reply = redisCommand (redc, "SET $CLIENTHOSTNAME %s", myhostname);
    logprintf ("SET $CLIENTHOSTNAME %s: %s\n", myhostname,
	       reply ? (reply->str) : "**none**");
    freeReplyObject (reply);
  }
  long count = 0;
  long cursor = 0;
  do
    {
      reply = redisCommand (redc, "SCAN %ld", cursor);
      int type = reply ? (reply->type) : 0;
      logprintf ("SCAN %ld: type %d (%s)\n", cursor, type,
		 (type > 0
		  && type <
		  sizeof (reply_types) /
		  sizeof (reply_types[0])) ? (reply_types[type] ? : "??") :
		 "???");
      if (type == REDIS_REPLY_STRING || type == REDIS_REPLY_ERROR)
	logprintf ("SCAN reply str %s\n", reply->str);
      else if (type == REDIS_REPLY_ARRAY)
	{
	  size_t nbelem = reply->elements;
	  logprintf ("SCAN reply array size %zd\n", nbelem);
	  for (size_t ix = 0; ix < nbelem; ix++)
	    {
	      struct redisReply *replelem = reply->element[ix];
	      assert (replelem != NULL);
	      int eltype = replelem->type;
	      logprintf ("SCAN reply [%zd] type %d (%s)\n",
			 ix, eltype,
			 (eltype > 0
			  && eltype <
			  sizeof (reply_types) /
			  sizeof (reply_types[0])) ? (reply_types[eltype] ? :
						      "??") : "???");
	    }
	}
      freeReplyObject (reply);
      count++;
    }
  while (cursor > 0);
  logprintf ("did %ld SCAN loops\n", count);
  logprintf ("done %s in %.4f CPU seconds\n", argv[0], 1.0e-6 * clock ());
  return EXIT_SUCCESS;
}				/* end main */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "gcc -o redis-scan -Wall -rdynamic -O -g redis-scan.c $(pkg-config --cflags --libs hiredis)" ;;
 ** End: ;;
 ****************/
