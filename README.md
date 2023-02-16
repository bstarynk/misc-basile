# misc-basile
Miscellaneous stuff, mostly single-file tiny programs

Email me to
[basile@starynkevitch.net](mailto:basile@starynkevitch.net) for
feedback and questions. See
[starynkevitch.net/Basile](http://starynkevitch.net/Basile/) for more
about me, with [this](http://starynkevitch.net/Basile/index_en.html)
being in English. My pet free software project is
[RefPerSys](http://refpersys.org/).

Some of these files are coded at work, so copyrighted by my employer
[CEA](https://www.cea.fr/). My office email is there (before
nov. 2023, when I reach retirement)
[basile.starynkevitch@cea.fr](mailto:basile.starynkevitch@cea.fr).

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
  bugs by sending periodically an arbitrary 
  [signal](https://man7.org/linux/man-pages/man7/signal.7.html) (such as `SIGSEGV` or
  `SIGABRT`) to the Linux process (or processes) running that command.

* `microbenchlist.c`  is a useless microbenchmark on linked lists
  use `gcc -Wall -O2 -march=native microbenchlist.c -o microbenchlist`
  to compile it.

* `makeprimes.c` uses the very clever BSD `/usr/games/primes` program
  and extract some primes from the stream of primes producing it.


* `clever-framac.cc` is a clever C++ wrapper for
  [Frama-C](https://frama-c.com/) static source code analyzer. It uses
  [GNU guile](https://www.gnu.org/software/guile/) version 3, so the
  `guile-3.0-dev` Debian package.

* `sync-periodically.c` runs periodically the
  [sync(2)](http://man7.org/linux/man-pages/man2/sync.2.html), is
  GPLv3+ licensed (so without warranty), and accepts both `--version`
  and `--help` program arguments. It might be useful (to avoid losing
  a lot of data in kernel file buffers) on power cuts, and don't
  consume lot of resources. Please glance inside the source code. Your
  `/etc/crontab` might have a line like

```
@reboot sync /usr/local/bin/sync-periodically --log-period=600 --sync-period=3  --daemon --pid-file=/var/run/sync-periodically.pid
```

and we wrote
  (for `systemd`) some `sync-periodically.service`


* `qfontdialog-example.cc` is a tiny improvement over [this `QFontDialog` example](http://www.codebind.com/cpp-tutorial/qt-tutorial/qt-tutorials-for-beginners-qfontdialog-example/)

* `HelloWorld/` contains a small set of files and its
  [README](HelloWorld/README.md), for a tutorial about GNU make (done
  on the phone). Perhaps the GPLv3+ license does not fit for such a
  trivial work.

* `onionwebsocket.c` is a slighty improved example of websockets from
  [libonion](https://www.coralbits.com/libonion/). Most of the code is
  not mine.

* `foldexample.cc` is interesting, since it shows how recent C++ compilers are capable of very deep optimizations

* `fox-tinyed.cc` is for learning the [FOX](https://fox-toolkit.org/) toolkit - a tiny editor (perhaps incomplete).

* `fltk-mini-edit.cc` is for learning the
  [FLTK](https://fltk-toolkit.org/) toolkit - a tiny editor (perhaps
  incomplete). It accepts some
  [JSONRPC](https://www.jsonrpc.org/specification) inspired protocol,
  documented in file
  [mini-edit-JSONRPC.md](mini-edit-JSONRPC.md).

* `logged-gcc.cc` is a (GPLv3 licensed) wrapper (coded in C++) around
  compilation commands by [GCC](http://gcc.gnu.org/) to log them (and
  their time) with
  [syslog(3)](https://man7.org/linux/man-pages/man3/syslog.3.html)
  and/or some [Sqlite](http://sqlite.org/) database. You will compile
  it using `compile-logged-gcc.sh`.  See also
  [this](https://unix.stackexchange.com/questions/605505/how-to-log-compilation-commands-on-linux-with-gcc). It
  could need improvements in start of 2023.

## Using `logged-gcc`

You first need to compile `logged-gcc.cc` with the
`compile-logged-gcc.sh` shell script. You might want to edit that
script. It produces a `logged-gcc` executable which you could put into
your `$HOME/bin/` directory.

You then should change your [`$PATH`
variable](https://en.wikipedia.org/wiki/PATH_(variable)) in such a way
that `$HOME/bin/` is in front of the directory containing your system
`gcc`, usually `/usr/bin/`. You might have something like `export
PATH=$HOME/bin:/usr/bin:/bin` in some shell initialization file
(e.g. `$HOME/.bashrc` or `$HOME/.zshrc` for [zsh](http://zsh.org/)
users).

You could run `logged-gcc --help` to get some help, and `logged-gcc
--version` for version information.

You then add *symbolic links* with `ln -sv $HOME/bin/logged-gcc $HOME/bin/gcc` and  `ln -sv $HOME/bin/logged-gcc $HOME/bin/g++`

You could set environment variables `$LOGGED_GCC` to
e.g. `/usr/bin/gcc-10` and `$LOGGED_GXX` to
e.g. `/usr/bin/g++-10`. See also
[environ(7)](https://man7.org/linux/man-pages/man7/environ.7.html).

If you want to use `logged-gcc` with just
[syslog(3)](https://man7.org/linux/man-pages/man3/syslog.3.html), you
don't need to do anything more.

If you want to use `logged-gcc` with some *Sqlite* database such as
`/tmp/logged-gcc.sqlite`, you need first to initialize it using
`logged-gcc --sqlite=/tmp/logged-gcc.sqlite` (before any
`/tmp/logged-gcc.sqlite` file exists), and then set the environment
variable `$LOGGED_SQLITE` to `/tmp/logged-gcc.sqlite`. Only successful
[GCC](http://gcc.gnu.org/) compilations go into that database.
