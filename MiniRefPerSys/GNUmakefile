## file misc-basile/MiniRefPerSys/GNUmakefile
CC=/usr/bin/gcc
RM=/bin/rm -vf
export GIT_ID=$(shell git log --format=oneline -q -1 | cut -c1-15)
HANDWRITTEN_SOURCES:=$(wildcard [a-z]*mrps.c)
GENERATED_SOURCES:=$(wildcard _*.c)
GENERATED_HEADERS=$(wildcard _*.h)
.PHONY: all clean gencfiles load dump indent

all: gencfiles minirefpersys

## tell GNU make to export all variables by default
export

MRPS_GIT_ID:= $(shell ./do-generate-gitid.sh)
MRPS_SHORTGIT_ID:= $(shell ./do-generate-gitid.sh -s)
MRPS_MAKE:= $(MAKE)
MRPS_SQLITE := $(shell /bin/which sqlite3)
MRPS_BISON := /usr/bin/bison
MRPS_INDENT_COMMAND := /usr/bin/indent 
MRPS_HOST := $(shell /bin/hostname -f)
MRPS_ARCH := $(shell /bin/uname -m)
MRPS_OPERSYS := $(shell /bin/uname -o | /bin/sed 1s/[^a-zA-Z0-9_]/_/g )
MRPS_ATSHARP := $(shell printf '@#')
MRPS_HOMETMP := $(shell echo '$$HOME/tmp')
MRPS_SQLITEDB := mini-refpersys.sqlite
MRPS_PACKAGES= gtk4 sqlite3 lightning
#MRPS_OPTIMFLAGS:= -Og

-include $(wildcard _*.mkdep)

MRPS_INCLUDES= $(shell pkg-config --cflags $(MRPS_PACKAGES))

%_mrps.o: %_mrps.c
	$(CC) $(MRPS_INCLUDES) $(MRPS_OPTIMFLAGS) -O -g -MD -MF __$(basename $(@F)).mkdep \
               -DMRPS_THIS_SOURCE=\"$<\" -DMRPS_THIS_BASENAME=\"$(basename $(@F))\"  \
               -DMRPS_SHORTGITID=\"$(MRPS_SHORTGIT_ID)\" \
               -DMRPS_SQLITEDB=\"$(MRPS_SQLITEDB)\" \
               -DMRPS_SQLITE=\"$(MRPS_SQLITE)\" \
	       -c -o $@ $<

%_mrps.so: %_mrps.c
	$(CC) -fPIC -shared $(MRPS_INCLUDES) $(MRPS_OPTIMFLAGS) -O -g -MD -MF __$(basename $(@F)).mkdep \
               -DMRPS_THIS_SOURCE=\"$<\" -DMRPS_THIS_BASENAME=\"$(basename $(@F))\"  \
               -DMRPS_SHORTGITID=\"$(MRPS_SHORTGIT_ID)\" \
               -DMRPS_SQLITEDB=\"$(MRPS_SQLITEDB)\" \
               -DMRPS_SQLITE=\"$(MRPS_SQLITE)\" \
	       -c -o $@ $<

minirefpersys: $(patsubst %.c,%.o,$(HANDWRITTEN_SOURCES)) \
               $(patsubst %.c,%.o,$(GENERATED_SOURCES)) | GNUmakefile $(MRPS_SQLITEDB)
	$(LINK.c) $<  \
                $(shell  pkg-config --libs $(MRPS_PACKAGES)) \
	        -lbacktrace -ldl -o $@ 

clean:
	$(RM) *.o *.so minirefpersys *~ *.orig

dump:
	[ $f $(basename $(MRPS_SQLITEDB)).sql ] || /bin/mv $(basename $(MRPS_SQLITEDB)).sql $(basename $(MRPS_SQLITEDB)).sql~
	$(MRPS_SQLITE) -batch -cmd .dump $(MRPS_SQLITEDB) < /dev/null > $(basename $(MRPS_SQLITEDB)).sql

indent:
	for f in $(HANDWRITTEN_SOURCES) minrefpersys.h ; do \
	    $(MRPS_INDENT_COMMAND) $$f ; \
	done

gencfiles: $(MRPS_SQLITEDB)
