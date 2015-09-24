# misc-basile
Miscellaneous stuff, mostly single-file tiny programs

Several single source files programs. Their compilation command is generally
given as a comment inside the source code.

* `manydl.c` is a program to show that Linux is capable of `dlopen`-ing
  *many* plugins (typically, several hundred thousands or many
  millions). It works by generating some pseudo-random C file, compiling it
  into a plugin, which is later dlopen-ed, and repeat.

* `redis-scan.c` is a program which scans all the keys in a REDIS database
  (see http://redis.io/ for more) and prints them on stdout.
