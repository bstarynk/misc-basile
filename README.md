# misc-basile
Miscellaneous stuff, mostly single-file tiny programs

Several single source files programs.

* `manydl.c` is a program to show that Linux is capable of dlopen-ing
  many plugins (typically, several hundred thousands or many
  millions). It works by generating some pseudo-random C file, compiling it
  into a plugin, which is later dlopen-ed, and repeat.
