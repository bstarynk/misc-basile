# Mini Edit JSONRPC protocol

This file `mini-edit-JSONRPC.md` in
https://github.com/bstarynk/misc-basile/ is documenting the
[JSONRPC](https://www.jsonrpc.org/specification) like protocol between
the `fltk-mini-edit` Linux executable (obtained from
[fltk-mini-edit.cc](https://github.com/bstarynk/misc-basile/blob/master/fltk-mini-edit.cc)
on a Linux computer, using script
[./mini-edit-build.sh](https://github.com/bstarynk/misc-basile/blob/master/mini-edit-build.sh)
to build it.

The `fltk-mini-edit` accepts a few arguments. Run `./fltk-mini-edit
--help` command. It may generate a few C++ files at runtime and
compile them as temporary plugins.

In this document `fltk-mini-edit` is considered as a JSONRPC
**server** and communicates with some **client**
(e.g. [RefPerSys](http://refpersys.org/) or
[Bismon](https://github.com/bstarynk/bismon/). By extension to JSONRPC
standard, our `fltk-mini-edit` may send asynchronous JSON events (see
below).

Communication between `fltk-mini-edit` and some other process
(probably [RefPerSys](http://refpersys.org/) related) uses
[fifo(7)](https://man7.org/linux/man-pages/man7/fifo.7.html) named
pipes.

Each JSONRPC message should be ended by two newlines character
(e.g. `"\n\n"` in C notation) or by one formfeed character
(e.g. `'\f'` in C notation). *This extends traditional JSONRPC conventions.*


## JSON asynchronous events

In addition to standard JSONRPC2 messages, the `fltk-mini-edit`
program may send **asynchronous event messages**. These are JSON
objects (ended by two newlines character (e.g. `"\n\n"` in C notation), and have the following mandatory JSON fields:

* `"json_event"` : `"1.O"      (to identify the protocol)
* `"event_number" : *a-unique-integer* (unique to the JSON communication channel; e.g. FIFO)
* `"event_unique_rank" : *globally-unique-integer*
* `"event_name" : *a-string*
* `"event_time" : *a floating-point-number*
* `"event_data" : arbitrary JSON value.

## API to extend the protocol (C compatible)

Use (in the process running `fltk-mini-edit`...) the functions
`my_command_register` and related `my_command_register_plain`,
`my_command_register_data1`, `my_command_register_data2` to add new
JSONRPC method names.

## core JSONRPC methods

The JSONRPC server is our `mini-edit` program.

### method `compileplugin`

This method gets C++ code lines in the JSONRPC request, to be
aggregated into a temporary plugin, which should be `dlopen`-ed. Once
loaded, some initialization C routine is run.

The RPC call should have a unique `id`, and the following JSON fields

* `"method" : "compileplugin"`
* `"prefix" :` *small-name-prefix*
* `"id" :` *required unique id integer*
* `"codelines" :` JSON array of strings, one per C++ line of the plugin

**Optionally**, it may have

* `"initializer" : ` JSON-string naming an initializing `extern "C"`
  routine. called with the given prefix and the given id. Once the
  plugin has been compiled and `dlopen`-ed, the `dlsym` is used to
  retrieve that initializing function.

The JSONRPC `result` has the following JSON fields

* `"compilation_pid"` : *pid-number*
* `"temporary_code"` : *file-name*

Once the temporary plugin has been successfully compiled and `dlopen`-ed, 