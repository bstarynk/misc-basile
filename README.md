# misc-basile
Miscellaneous stuff, mostly single-file tiny programs

Several single source file programs usually for
GNU/Linux/Debian/x86-64. Their compilation command is generally given
as a comment inside the source code.

* `manydl.c` is a program to show that Linux is capable of `dlopen`-ing
  *many* plugins (typically, several hundred thousands or many
  millions). It works by generating some pseudo-random C file, compiling it
  into a plugin, which is later dlopen-ed, and repeat.

* `redis-scan.c` is a program which scans all the keys in a REDIS database
  (see http://redis.io/ for more) and prints them on stdout.

* `execicar.c` is a shell-like program interpreting commands on pipes, etc.

* `basilemap.ml` is a simple exercise to understand the balanced binary trees
of the Ocaml stdlib/map.ml file, which I might simplify a bit.

* `bwc.c`  is a crude `wc -l` like program using getline; for performance benchmarking.

* `half.c` is a program to stop/cont-inue a command, running it at half load

* `microbenchlist.c`  is a useless microbenchmark on linked lists
  use `gcc -Wall -O2 -march=native microbenchlist.c -o microbenchlist`
  to compile it.

* `HelloWorld/` contains a small set of files and its [README](HelloWorld/README.md), for a tutorial about GNU make (done on the phone). Perhaps the GPLv3+ license does not fit for such a trivial work.
