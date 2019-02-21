
/**
  onionwebsocket.c: my adaptation of Libonion's
  examples/websockets/websockets.c to learn more about websockets.

  Libonion is on  https://github.com/davidmoreno/onion

  Copyright (C) 2010-2019 David Moreno Montero, Basile Starynkevitch
  and others

  This [library] example is free software; you can redistribute it
  and/or modify it under the terms of, at your choice:

  a. the Apache License Version 2.0.

  b. the GNU General Public License as published by the
  Free Software Foundation; either version 2.0 of the License,
  or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of both licenses, if not see
  <http://www.gnu.org/licenses/> and
  <http://www.apache.org/licenses/LICENSE-2.0>.
*/

#include <features.h>
#include <onion/log.h>
#include <onion/onion.h>
#include <onion/websocket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>
#include <stdio.h>

#ifndef HAVE_GNUTLS
// on Debian, install libcurl4-gnutls-dev & gnutls-dev pacakges before
// building libonion
#error without HAVE_GNUTLS but onion needs it
#endif

onion_connection_status websocket_example_cont (void *data,
						onion_websocket * ws,
						ssize_t data_ready_len);

onion_connection_status
websocket_example (void *data, onion_request * req, onion_response * res)
{
  ONION_INFO ("%s: start req method %s path '%s' fullpath '%s'",
	      __func__,
	      onion_request_methods[onion_request_get_flags (req) & 0xf],
	      onion_request_get_path (req), onion_request_get_fullpath (req));
  onion_websocket *ws = onion_websocket_new (req, res);
  if (!ws)
    {
      time_t nowtim = time (NULL);
      char timbuf[80];
      static atomic_int acnt;
      atomic_fetch_add (&acnt, 1);
      memset (timbuf, 0, sizeof (timbuf));
      strftime (timbuf, sizeof (timbuf), "%c", localtime (&nowtim));
      onion_response_set_header (res, "Content-Type",
				 "text/html; charset=utf-8");
      onion_response_write0 (res,
			     "<html><body><h1 id='h1id'>Easy echo</h1>\n");
      onion_response_printf (res,
			     "<p>Generated <small><tt>%s</tt></small>, count %d, pid %d</p>\n",
			     timbuf, acnt, (int) getpid ());
      onion_response_write0 (res, "<pre id=\"chat\"></pre>"	//
			     " <script>\ninit = function(){\n"	//
			     "console.group('chatinit');\n"	//
			     "msg=document.getElementById('msg');\n"	//
			     "msg.focus();\n\n"	//
			     "ws=new WebSocket('ws://'+window.location.host);\n"	//
			     "console.log('init... msg=', msg, ' ws=', ws, ' window=', window);\n"	//
			     "console.trace();\n"	//
			     "console.groupEnd();\n"	//
			     "ws.onmessage=function(ev){\n"	//
			     "   document.getElementById('chat').textContent+=ev.data+'\\n';\n"	//
			     "};}\n"	//
			     "window.addEventListener('load', init, false);\n</script>\n"	//
			     "<input type=\"text\" id=\"msg\"\n"	//
			     "     onchange=\"javascript:ws.send(msg.value);"	//
			     "     msg.select(); msg.focus();\"/><br/>\n"	//
			     "<button onclick='ws.close(1000);'>Close connection</button>\n"	//
			     "<p>To <a href='#h1id'>top</a>.\n"	//
			     "Try to <i>open in new tab</i> that link</p>\n"	//
			     "</body></html>");	//
      ONION_INFO ("%s: websocket created ws@%p acnt=%d timbuf=%s", __func__,
		  ws, acnt, timbuf);
      fflush (NULL);
      return OCS_PROCESSED;
    }
  else
    ONION_INFO ("%s: got ws@%p", __func__, ws);

  onion_websocket_printf (ws,
			  "Hello from server. Write something to echo it. ws@%p",
			  ws);
  onion_websocket_set_callback (ws, websocket_example_cont);

  return OCS_WEBSOCKET;
}				/* end websocket_example */




onion_connection_status
websocket_example_cont (void *data, onion_websocket * ws,
			ssize_t data_ready_len)
{
  ONION_INFO ("%s: ws@%p data_ready_len=%ld", __func__, ws,
	      (long) data_ready_len);
  char tmp[256];
  if (data_ready_len > sizeof (tmp))
    data_ready_len = sizeof (tmp) - 1;

  int len = onion_websocket_read (ws, tmp, data_ready_len);
  if (len <= 0)
    {
      ONION_ERROR ("Error reading data: %d: %s (%d)", errno, strerror (errno),
		   data_ready_len);
      return OCS_NEED_MORE_DATA;
    }
  tmp[len] = 0;
  onion_websocket_printf (ws, "Echo: %s (ws@%p)", tmp, ws);

  ONION_INFO ("Read from websocket ws@%p: %d: %s", ws, len, tmp);

  return OCS_NEED_MORE_DATA;
}

int
main ()
{
  onion *o = onion_new (O_THREADED);
  onion_set_port (o, "8087");
  onion_set_hostname (o, "localhost");

  onion_url *urls = onion_root_url (o);

  onion_url_add (urls, "", websocket_example);

  onion_listen (o);

  onion_free (o);
  return 0;
}


/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "gcc -o onionwebsocket -Wall -rdynamic -I/usr/local/include/ -O -g -DHAVE_GNUTLS onionwebsocket.c -L /usr/local/lib $(pkg-config --cflags --libs gnutls) -lonion" ;;
 ** End: ;;
 ****************/
