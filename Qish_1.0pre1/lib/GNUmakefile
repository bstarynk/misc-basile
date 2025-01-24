# file Qish/lib/GNUmakefile
# emacs Time-stamp: <2005 May 08 19h17 CEST {GNUmakefile} Basile STARYNKEVITCH>
#* This library is free software; you can redistribute it and/or
#* modify it under the terms of the GNU Lesser General Public
#* License as published by the Free Software Foundation; either
#* version 2 of the License, or (at your option) any later version.
#*
#* This library is distributed in the hope that it will be useful,
#* but WITHOUT ANY WARRANTY; without even the implied warranty of
#* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
#* Lesser General Public License for more details.
#*
#* You should have received a copy of the GNU Lesser General
#* Public License along with this library; if not, write to the
#* Free Software Foundation, Inc., 59 Temple Place, Suite 330,
#* Boston, MA 02111-1307, USA.
SUFFIXES += .i
RM=rm -vf
MV=mv -f
CP=cp
MKDIR=mkdir -pv
INSTALL=install
ifndef OPTIMFLAGS
OPTIMFLAGS= -O -g3
endif 
ifndef PREFIX
PREFIX=/usr/local
endif
CFLAGS+= -Wall -W -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Winline  $(OPTIMFLAGS)
CPPFLAGS+= -I../include/ -I./
BOEHMGC_INCLUDES= -I/usr/include/gc
BOEHMGC_LIBS= 
SRC:=$(wildcard qi*.c)
OBJS:=$(patsubst %.c, %.o, $(SRC))
OBJS_ROUT:=$(patsubst %.c, %_rout.o, $(SRC))

.PHONY: all clean lib header test install bench benchqish

all: lib test

# to debug macros we may need to generate the .i
%.i: %.c
	$(PREPROCESS.S) -C $^ | grep -v '^#' | indent -st | cat -s > $@
%.o: %.i
	$(COMPILE.c)  $<
%.s: %.i
	$(COMPILE.c) -S $<

%_rout.o: %.c
	$(COMPILE.c) -DQISH_ROUTINE $< -o $@

_libdate.c:
	date "+const char qishlib_date[]=\"%Y %b %d, %T %Z\";%nconst long qishlib_time=%s;" >_libdate.c
	echo "const char qishlib_user[]=\"$(shell whoami)\";" >> _libdate.c
	echo "const char qishlib_srcdir[]=\"$(PWD)\";" >> _libdate.c
	echo "const char qishlib_hostname[]=\"`uname -n`\";" >> _libdate.c
	echo "const char qishlib_system[]=\"`uname -srm`\";" >> _libdate.c
# $Format: "\techo const char \"qishlib_version[]=\\\"$ProjectVersion$\\\";\" >> _libdate.c" $
	echo const char "qishlib_version[]=\"1.9\";" >> _libdate.c
# $Format: "\techo const int qishlib_minorvnum= $ProjectMinorVersion$ ';' >> _libdate.c" $
	echo const int qishlib_minorvnum= 9 ';' >> _libdate.c
# $Format: "\techo const char \"qishlib_majorvers[]= \\\"$ProjectMajorVersion$\\\";\" >> _libdate.c" $
	echo const char "qishlib_majorvers[]= \"1\";" >> _libdate.c
# $Format: "\techo const char \"qishlib_prcsdate[]=\\\"$ProjectDate$;\\\" ;\" >> _libdate.c" $
	echo const char "qishlib_prcsdate[]=\"Tue, 28 Dec 2004 12:08:57 +0100;\" ;" >> _libdate.c
# $Format: "\techo const char \"qishlib_release[]=\\\"$ReleaseVersion$;\\\" ;\" >> _libdate.c" $
	echo const char "qishlib_release[]=\"1.0pre1;\" ;" >> _libdate.c

_libdate.o: _libdate.c
	$(CC) -O -c _libdate.c
	$(MV) _libdate.c _libdate.c~

lib: header libqish.a libqish_rout.a

libqish.a: $(OBJS) _libdate.o
	$(AR) -rvf $@ $^



$(OBJS): ../include/qish.h _qishgen.h
$(OBJS_ROUT): ../include/qish.h _qishgen.h


libqish_rout.a: $(OBJS_ROUT) _libdate.o
	$(AR) -rvf $@ $^

header: _qishgen.h ../include/qish.h 
	$(CP) _qishgen.h ../include

_qishgen.h: genqish
	./genqish > _qishgen.h


genqish: genqish.c
	$(CC) -o $@ $<

clean: 
	$(RM) _qishgen.h genqish *.o *~ core  *.a bench_qish bench_gc bench_malloc

install: lib header
	$(MKDIR) $(PREFIX)/lib/
	$(INSTALL) libqish.a libqish_rout.a $(PREFIX)/lib/
	$(MKDIR) $(PREFIX)/include/
	$(INSTALL) ../include/qish.h $(PREFIX)/include/
	$(INSTALL) _qishgen.h $(PREFIX)/include/

bench: GCBench.c
	$(CC) $(OPTIMFLAGS)  -DGC $(BOEHMGC_INCLUDES) GCBench.c $(BOEHMGC_LIBS)  -lgc -o bench_gc
	$(CC) $(OPTIMFLAGS)  GCBench.c -o bench_malloc
	$(CC) $(OPTIMFLAGS)   -I../include -I../lib -L../lib -DQISH GCBench.c -o bench_qish -L../lib -lqish -ldl
	@printf '\n\n** benching Qish copying GC:\n'
	./bench_qish
	@printf '\n\n** benching Boehm s conservative GC:\n'
	./bench_gc
	@printf '\n\n** benching explicit malloc and free:\n'
	./bench_malloc

benchqish:GCBench.c
	$(CC) $(OPTIMFLAGS) -O -I../include -I../lib -L../lib -DQISH GCBench.c -o bench_qish -L../lib -lqish -ldl
	@printf '\n\n** benching only Qish copying GC:\n'
	./bench_qish
#eof $Id: GNUmakefile 1.22 Thu, 23 Dec 2004 14:34:56 +0100 basile $
