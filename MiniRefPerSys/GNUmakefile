## file misc-basile/MiniRefPerSys/GNUmakefile
CC=/usr/bin/gcc
RM=/bin/rm -vf
export GIT_ID=$(shell git log --format=oneline -q -1 | cut -c1-15)
HANDWRITTEN_SOURCES=$(wildcard [a-z]*mrps.c)
.PHONY: all clean

all: minirefpersys

## tell GNU make to export all variables by default
export

MRPS_GIT_ID:= $(shell ./do-generate-gitid.sh)
MRPS_SHORTGIT_ID:= $(shell ./do-generate-gitid.sh -s)
MRPS_MAKE:= $(MAKE)
MRPS_BISON := /usr/bin/bison
MRPS_HOST := $(shell /bin/hostname -f)
MRPS_ARCH := $(shell /bin/uname -m)
MRPS_OPERSYS := $(shell /bin/uname -o | /bin/sed 1s/[^a-zA-Z0-9_]/_/g )
MRPS_ATSHARP := $(shell printf '@#')
MRPS_HOMETMP := $(shell echo '$$HOME/tmp')
MRPS_PACKAGES= gtk4 jansson
MRPS_OPTIMFLAGS:= -Og

MRPS_INCLUDES= $(shell pkg-config --cflags $(MRPS_PACKAGES))

%_mrps.o: %_mrps.c
	$(CC) $(MRPS_INCLUDES) $(MRPS_OPTIMFLAGS) -O -g -MD -MF __$(basename $(@F)).mkdep \
               -DMRPS_THIS_SOURCE=\"$<\" -DMRPS_GITID=\"$(MRPS_GIT_ID)\" -DMRPS_THIS_BASENAME=$(basename $(@F))  \
               -DMRPS_SHORTGITID=\"$(MRPS_SHORTGIT_ID)\" \
	       -c -o $@ $^
