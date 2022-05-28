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

Communication between `fltk-mini-edit` and some other process
(probably [RefPerSys](http://refpersys.org/) related) uses
[fifo(7)](https://man7.org/linux/man-pages/man7/fifo.7.html) named
pipes.

Each JSONRPC message should be ended by two newlines character
(e.g. `"\n\n"` in C notation) or by one formfeed character
(e.g. `'\f'` in C notation). *This differs from traditional JSONRPC conventions.*

## API to extend the protocol (C compatible)

Use (in the process running `fltk-mini-edit`...) the functions
`my_command_register` and related `my_command_register_plain`,
`my_command_register_data1`, `my_command_register_data2` to add new
JSONRPC method names.

## core JSONRPC methods

### method `compileplugin`

This method gets C++ code lines in the JSONRPC request, to be
aggregated into a temporary plugin, which should be `dlopen`-ed. Once
loaded, some initialization C routine is run.