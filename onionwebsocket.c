
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

#include <onion/log.h>
#include <onion/onion.h>
#include <onion/websocket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

onion_connection_status websocket_example_cont (void *data,
						onion_websocket * ws,
						ssize_t data_ready_len);

onion_connection_status
websocket_example (void *data, onion_request * req, onion_response * res)
{
  onion_websocket *ws = onion_websocket_new (req, res);
  if (!ws)
    {
      onion_response_write0 (res,
			     "<html><body><h1 id='h1id'>Easy echo</h1>\n"
			     "<pre id=\"chat\"></pre>"
			     " <script>\ninit = function(){\n"
			     "msg=document.getElementById('msg');\n"
			     "msg.focus();\n\n"
			     "ws=new WebSocket('ws://'+window.location.host);\n"
			     "ws.onmessage=function(ev){\n document.getElementById('chat').textContent+=ev.data+'\\n';\n"
			     "};}\n"
			     "window.addEventListener('load', init, false);\n</script>"
			     "<input type=\"text\" id=\"msg\" onchange=\"javascript:ws.send(msg.value); msg.select(); msg.focus();\"/><br/>\n"
			     "<button onclick='ws.close(1000);'>Close connection</button>\n"
			     "<p>To <a href='#h1id'>top</a>.\n"
			     "Try to <i>open in new tab</i> that link</p>\n"
			     "</body></html>");

      return OCS_PROCESSED;
    }

  onion_websocket_printf (ws,
			  "Hello from server. Write something to echo it");
  onion_websocket_set_callback (ws, websocket_example_cont);

  return OCS_WEBSOCKET;
}

onion_connection_status
websocket_example_cont (void *data, onion_websocket * ws,
			ssize_t data_ready_len)
{
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
  onion_websocket_printf (ws, "Echo: %s", tmp);

  ONION_INFO ("Read from websocket: %d: %s", len, tmp);

  return OCS_NEED_MORE_DATA;
}

int
main ()
{
  onion *o = onion_new (O_THREADED);
  onion_set_port(o, ":8087");
  onion_set_hostname(o, "localhost");

  onion_url *urls = onion_root_url (o);

  onion_url_add (urls, "", websocket_example);

  onion_listen (o);

  onion_free (o);
  return 0;
}


/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "gcc -o onionwebsocket -Wall -rdynamic -I/usr/local/include/ -O -g onionwebsocket.c -L /usr/local/lib -lonion" ;;
 ** End: ;;
 ****************/
