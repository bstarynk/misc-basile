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
