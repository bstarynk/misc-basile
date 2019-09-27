# misc-basile
Miscellaneous stuff, mostly single-file tiny programs

Several single source file programs usually for
GNU/Linux/Debian/x86-64. Their compilation command is generally given
as a comment inside the source code.

* `manydl.c` is a program to show that Linux is capable of `dlopen`-ing
  *many* plugins (typically, several hundred thousands or many
  millions). It works by generating some pseudo-random C file, compiling it
  into a plugin, which is later dlopen-ed, and repeat.

* `forniklas.c` is a trivial C program generating then using one single plugin
 in C. Read its comments for more details.

* `redis-scan.c` is a program which scans all the keys in a REDIS database
  (see http://redis.io/ for more) and prints them on stdout.

* `execicar.c` is a shell-like program interpreting commands on pipes, etc.

* `basilemap.ml` is a simple exercise to understand the balanced binary trees
of the Ocaml stdlib/map.ml file, which I might simplify a bit.

* `bwc.c`  is a crude `wc -l` like program using getline; for performance benchmarking.

* `half.c` is a program to stop/cont-inue a command, running it at
  half load. It was originally written to overcome a hardware bug on
  an old MSI-270 Turion laptop. It might be useful today to "emulate"
  bugs by sending periodically an arbitrary signal (such as SIGSEGV or
  SIGABRT) to a command.

* `microbenchlist.c`  is a useless microbenchmark on linked lists
  use `gcc -Wall -O2 -march=native microbenchlist.c -o microbenchlist`
  to compile it.

* `makeprimes.c` uses the very clever BSD `/usr/games/primes` program
  and extract some primes from the stream of primes producing it.

* `HelloWorld/` contains a small set of files and its
  [README](HelloWorld/README.md), for a tutorial about GNU make (done
  on the phone). Perhaps the GPLv3+ license does not fit for such a
  trivial work.

* `onionwebsocket.c` is a slighty improved example of websockets from
  [libonion](https://www.coralbits.com/libonion/). Most of the code is
  not mine.

* `foldexample.cc` is interesting, since it shows how recent C++ compilers are capable of very deep optimizations
