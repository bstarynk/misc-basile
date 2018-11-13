This is a tiny set of files to help a tutorial by phone on [GNU
make](https://www.gnu.org/software/make/). It is two variants of a
hello world program (one is a single file `helloworld1.c`, another is
a two files `hello2.c` and `world2.c` with a common header
`helloworld2.h`). There are three makefiles: `Makefile1`,
`Makefile2plain` and `Makefile2timestamp` (the later showing a
generated `timestamp2.c` file with auto-dependencies).

Read wikipages on
[make](https://en.wikipedia.org/wiki/Make_(software)) and on
[Makefile](https://en.wikipedia.org/wiki/Makefile)-s. Remember that
the *tab* character is significant in these *Makefile*-s (some editors
may require special configuration for that, but most source editors
are clever enough).

Read of course the [documentation of GNU
make](https://www.gnu.org/software/make/manual/html_node/index.html)
and of [GCC](https://gcc.gnu.org/onlinedocs/gcc/), notably the first
sections of [§3 GCC Command
Options](https://gcc.gnu.org/onlinedocs/gcc/Invoking-GCC.html).

I am not sure if it is correct to use the GPLv3+ license for such a
trivial example.

See discussion on https://opensource.stackexchange.com/q/7597/910

To use the first *Makefile*, type `make -f Makefile1` (it builds the `helloworld1` executable from source `helloworld1.c`).

To clean after that, type `make -f Makefile1 clean`

To use the second *Makefile* type `make -f Makefile2plain` (it builds
the `helloworld2` executable from `hello2.c` and `world2.c`, and they
both include `helloworld2.h`) then later clean the mess with `make -f
Makefile2plain clean`.

To use the third *Makefile* type `make -f Makefile2timestamp` (it
generates a `timestamp2.c` and build the `helloworld2timestamp`; it
also autogenerate make dependencies).

You can symlink one of the `Makefile`-s above to your `Makefile` and
then just run `make` (or `make clean`).