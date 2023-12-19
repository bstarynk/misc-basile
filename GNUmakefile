## file misc-basile/Makefile
## on https://github.com/bstarynk/

.PHONY: all clean indent manydl-plugins analyze-framac framac-bwc framac-sync-periodically framac-manydl framac-half clever-framac valgrind-logged-gcc

FRAMAC=/usr/bin/frama-c
FRAMALIBC=/usr/share/frama-c/libc/
INDENT=/usr/bin/indent
VALGRIND=/usr/bin/valgrind
CC=/usr/bin/gcc
CXX=/usr/bin/g++
ASTYLE=/usr/bin/astyle
RM=/bin/rm -vf
GENG_CC ?= $(CC)
GIT_ID=$(shell git log --format=oneline -q -1 | cut -c1-10)
CFLAGS= -O2 -g -Wall -Wextra -I /usr/local/include/



GENF_CC=$(CC)
GENF_CFLAGS= -O2 -g -fPIC -Wall

all: manydl half bwc sync-periodically clever-framac logged-compile  logged-gcc filipe-shell browserfox onionrefpersys


clean:
	$(RM) *~ *.orig *.o bwc manydl clever-framac half sync-periodically filipe-shell
	$(RM) browserfox fox-tinyed logged-g++ half logged-gcc execicar gtksrc-browser winpersist
	$(RM) genf*.c
	$(RM) logged-gcc_*
## on non Linux, change .so to whatever can be dlopen-esd
	$(RM) genf*.so


## on non Linux systems, change the .so suffix to whatever can be dlopen-ed
manydl-plugins: $(patsubst %.c, %.so, $(wildcard genf*.c))

browserfox: browserfox.cc build-browserfox.sh logged-g++
	./build-browserfox.sh

## this is specific to Linux, change it on other POSIX systems
genf%.so: genf%.c
	time $(GENG_CC) $(GENF_CFLAGS) -shared -o $@ $^

indent:
	for f in $(wildcard *.c) ; do $(INDENT) --gnu-style $$f ; done
	for f in $(wildcard *.cc) ; do $(ASTYLE) --style=gnu $$f ; done

manydl: manydl.c
	$(CC) $(CFLAGS) -DMANYDL_GIT='"$(GIT_ID)"' -rdynamic $^ -lm -ldl -o $@

half: half.c
	$(CC) $(CFLAGS) -DHALF_GIT='"$(GIT_ID)"' $^  -o $@

bwc: bwc.c
	$(CC) $(CFLAGS) -DBWC_GIT='"$(GIT_ID)"' $^  -o $@

sync-periodically: sync-periodically.c
	$(CC) $(CFLAGS) -DSYNPER_GITID='"$(GIT_ID)"' $^  -o $@

filipe-shell: filipe-shell.c
	$(CC) $(CFLAGS) -DFILIPE_GIT='"$(GIT_ID)"' $^  -o $@

logged-gcc: logged-gcc.cc compile-logged-gcc.sh
	./compile-logged-gcc.sh

logged-compile: logged-compile.cc
	$(CXX) $(CFLAGS) $^ -L/usr/local/lib -DGIT_ID='"$(GIT_ID)"' -lsqlite3 -o $@ 

clever-framac: clever-framac.cc build-clever-framac.sh
	./build-clever-framac.sh

analyze-framac:  framac-bwc framac-sync-periodically framac-manydl framac-half

framac-bwc:
	$(FRAMAC)  -cpp-command '$(CC) -C -E -I $(FRAMALIBC) -I. -I/usr/include -x c'  -eva -eva-verbose 2  bwc.c

framac-sync-periodically:
	$(FRAMAC)  -cpp-command '$(CC) -C -E -I$(FRAMALIBC)  -I. -I/usr/include -x c'  -eva  -eva-verbose 2 sync-periodically.c

framac-manydl:
	$(FRAMAC)  -cpp-command '$(CC) -C -E -I $(FRAMALIBC)  -I. -I/usr/include -x c'  -eva  -eva-verbose 2 $(FRAMALIBC)/string.c manydl.c

framac-half:
	$(FRAMAC) -cpp-command '$(CC) -C -E -I /usr/share/frama-c/libc/ -I. -I/usr/include -x c'  -eva  -eva-verbose 2  half.c

valgrind-logged-gcc: logged-gcc sync-periodically.c
	$(VALGRIND) --verbose --leak-check=full ./logged-gcc --debug $(CFLAGS) -DSYNPER_GITID='"$(GIT_ID)"' sync-periodically.c  -o /tmp/sync-periodically

onionrefpersys: onionrefpersys.c | GNUmakefile
	$(CC)  $(CFLAGS) -DGITID='"$(GIT_ID)"' $^ -L/usr/local/lib \
           -Bstatic -lonion_static -Bdynamic -lsystemd -lgcrypt -lgnutls -o $@
