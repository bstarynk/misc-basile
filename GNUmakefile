## file misc-basile/GNUmakefile
## on https://github.com/bstarynk/

.PHONY: all clean indent manydl-plugins analyze-framac framac-bwc framac-sync-periodically framac-manydl framac-half clever-framac valgrind-logged-gcc minicomp

FRAMAC=/usr/bin/frama-c
FRAMALIBC=/usr/share/frama-c/libc/
INDENT=/usr/bin/indent
VALGRIND=/usr/bin/valgrind
CC=/usr/bin/gcc
CXX=/usr/bin/g++
ASTYLE=/usr/bin/astyle
ASTYLEFLAGS= --style=gnu -s2
RM=/bin/rm -vf
GENG_CC ?= $(CC)
export GIT_ID=$(shell git log --format=oneline -q -1 | cut -c1-15)
CFLAGS= -O2 -g -Wall -Wextra -I /usr/local/include/
CXXFLAGS= -O -g -Wall -Wextra -I /usr/local/include/
GTK4SERV_PACKAGES= gtk4 glib-2.0 gobject-2.0 gio-2.0
GTKMMRPS_PACKAGES= gtkmm-4.0 jsoncpp
Q6REFPERSYS_PACKAGES= Qt6Core Qt6Gui Qt6DBus Qt6Widgets jsoncpp
GUILE_CFLAGS:=$(shell pkg-config --cflags guile-3.0)
GUILE_LIBS:=$(shell  pkg-config --libs guile-3.0)

QT6MOC= /usr/lib/qt6/libexec/moc
GENF_CC=$(CC)
GENF_CFLAGS= -O2 -g -fPIC -Wall

all: manydl half sync-periodically transpiler-refpersys \
     logged-compile logged-gcc filipe-shell browserfox \
     gtk4serv fox-tinyed q6refpersys  gtkmm-refpersys bwc gtksrc-browser \
     process-rgb-color


clean:
	$(RM) *~ *.orig *.o bwc manydl clever-framac half sync-periodically filipe-shell
	$(RM) fltk-mini-edit
	$(RM) browserfox fox-tinyed logged-g++ half logged-gcc execicar gtksrc-browser winpersist
	$(RM) transpiler-refpersys
	$(RM) build-with-guile
	$(RM) _genf*.c
	$(RM) _logged-gcc_*
	$(RM) _q6refpersys*
	$(RM) *.ii
	$(RM) onionrefpersys gtkmm-refpersys q6refpersys
## on non Linux, change .so to whatever can be dlopen-esd
	$(RM) _genf*.so
	$(RM) _pmap*
	$(MAKE) -C MiniComp clean


## on non Linux systems, change the .so suffix to whatever can be dlopen-ed
manydl-plugins: $(patsubst %.c, %.so, $(wildcard _genf*.c))

browserfox: browserfox.cc build-browserfox.sh |GNUmakefile
	./build-browserfox.sh

## this is specific to Linux, related to manydl.c, change it on other
## POSIX systems
_genf%.so: _genf%.c
	time $(GENG_CC) $(GENF_CFLAGS) -shared -o $@ $^

indent:
	for f in $(wildcard *.c) ; do $(INDENT) --gnu-style $$f ; done
	for f in $(wildcard *.cc) ; do $(ASTYLE) $(ASTYLEFLAGS) $$f ; done
	for f in $(wildcard *.hh) ; do $(ASTYLE) $(ASTYLEFLAGS) $$f ; done

manydl: manydl.c
	$(CC) $(CFLAGS) -DMANYDL_GIT='"$(GIT_ID)"' -rdynamic $^ -lm -ldl -o $@

half: half.c
	$(CC) $(CFLAGS) -DHALF_GIT='"$(GIT_ID)"' $^  -o $@

bwc: bwc.c
	$(CC) $(CFLAGS) -DBWC_GIT='"$(GIT_ID)"' $^  -o $@

build-with-guile: build-with-guile.c
	$(CC) $(CFLAGS) -O -g -Wall -DBUILDWITHGUILE_GIT='"$(GIT_ID)"' $(GUILE_CFLAGS) -rdynamic $^ $(GUILE_LIBS) -lm -ldl -o $@

sync-periodically: sync-periodically.c
	$(CC) $(CFLAGS) -DSYNPER_GITID='"$(GIT_ID)"' $^  -o $@

transpiler-refpersys: transpiler-refpersys.cc transpiler-refpersys.hh |GNUmakefile
	$(CXX) $(CXXFLAGS) -DGIT_ID='"$(GIT_ID)"' $(GUILE_CFLAGS) $< -o $@ -lgccpp -lunistring $(GUILE_LIBS) -lgc  -ldl
transpiler-refpersys.ii: transpiler-refpersys.cc transpiler-refpersys.hh |GNUmakefile
	$(CXX) $(CXXFLAGS) -DGIT_ID='"$(GIT_ID)"' $(GUILE_CFLAGS) -C -E $< -o - | /bin/sed -e 's:^#://#:' > $@

filipe-shell: filipe-shell.c
	$(CC) $(CFLAGS) -DFILIPE_GIT='"$(GIT_ID)"' $^  -o $@

logged-gcc: logged-gcc.cc compile-logged-gcc.sh
	./compile-logged-gcc.sh

logged-compile: logged-compile.cc
	$(CXX) $(CFLAGS) $^ -L/usr/local/lib -DGIT_ID='"$(GIT_ID)"' -lsqlite3 -o $@ 

clever-framac: clever-framac.cc |build-clever-framac.sh GNUmakefile
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

### not built by default in sept 2024
onionrefpersys: onionrefpersys.c  GNUmakefile
	$(CC)  $(CFLAGS) -DGITID='"$(GIT_ID)"' $<  -L/usr/local/lib \
           -lonion_static -lsqlite3 -lsystemd -lgcrypt -lgnutls -lgmp -Bdynamic -o $@

gtksrc-browser: gtksrc-browser.c build-gtksrc-browser.sh  |GNUmakefile
	./build-gtksrc-browser.sh

fox-tinyed: fox-tinyed.cc tinyed-build.sh |GNUmakefile
	./tinyed-build.sh

gtk4serv: gtk4serv.c  |GNUmakefile
	$(CC) -rdynamic -fPIE -fPIC $(CFLAGS) -DGITID='"$(GIT_ID)"' \
	$(shell pkg-config --cflags $(GTK4SERV_PACKAGES)) $< \
	$(shell pkg-config --libs $(GTK4SERV_PACKAGES)) -o $@

fltk-mini-edit: fltk-mini-edit.cc mini-edit-build.sh |GNUmakefile
	./mini-edit-build.sh

gtkmm-refpersys: gtkmm-refpersys.cc |GNUmakefile
	$(CXX)  -rdynamic -fPIE -fPIC -g -O $(CXXFLAGS) -DGITID='"$(GIT_ID)"' \
	$(shell pkg-config --cflags $(GTKMMRPS_PACKAGES)) $< \
	$(shell pkg-config --libs $(GTKMMRPS_PACKAGES)) -o $@

q6refpersys: q6refpersys.cc _q6refpersys-moc.cc |GNUmakefile
	$(CXX) -rdynamic -fPIE -fPIC -g -O $(CXXFLAGS) -DGITID='"$(GIT_ID)"' \
	$(shell pkg-config --cflags $(Q6REFPERSYS_PACKAGES)) $< \
	$(shell pkg-config --libs $(Q6REFPERSYS_PACKAGES)) -o $@

_q6refpersys-moc.cc: q6refpersys.cc |GNUmakefile
	$(QT6MOC)  -DGITID='"$(GIT_ID)"'  q6refpersys.cc > $@

q6refpersys.ii: q6refpersys.cc  _q6refpersys-moc.cc |GNUmakefile
	$(CXX) -C -E $(CXXFLAGS) -DGITID='"$(GIT_ID)"' \
	$(shell pkg-config --cflags $(Q6REFPERSYS_PACKAGES)) $< -o - \
	        | /bin/sed s:^#://#: > $@ \

minicomp: |GNUmakefile MiniComp/GNUmakefile $(wildcard MiniComp/*.c MiniComp/*.h)
	$(MAKE) -C MiniComp all
