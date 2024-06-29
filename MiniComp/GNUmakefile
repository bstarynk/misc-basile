## file misc-basile/MiniComp/GNUmakefile
## on https://github.com/bstarynk/
## SPDX-License-Identifier: GPLv3+
##  © Copyright Basile Starynkevitch  2024

## Permission is hereby granted, free of charge, to any person
## obtaining a copy of this software and associated documentation
## files (the "Software"), to deal in the Software without
## restriction, including without limitation the rights to use, copy,
## modify, merge, publish, distribute, sublicense, and/or sell copies
## of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be
## included in all copies or substantial portions of the Software.
##
## THE minicomp SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
## KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
## WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
## NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
## BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
## ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
## CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.
##
## minicomp is designed to be run by programs and translating a
## sequence of JSON files into a shared object using the libgccjit
## provided by GNU gcc
##
CC=/usr/bin/gcc
INDENT=/usr/bin/indent
RM=/bin/rm -vf
export GIT_ID=$(shell git log --format=oneline -q -1 | cut -c1-15)
PACKAGES=glib-2.0 jansson
OPTIMFLAGS= -O2 -g
CFLAGS= $(OPTIMFLAGS) -Wall -Wextra -I /usr/local/include/

.PHONY: all clean

all: minicomp-gccjit

minicomp-gccjit: minicomp-gccjit.c minicomp.h |GNUmakefile
	$(CC)  -rdynamic -fPIE -fPIC $(CFLAGS) -DGITID='"$(GIT_ID)"' \
	       -DMD5SUM='"$(shell /bin/md5sum $<)"' \
               -I $(shell $(CC) -print-file-name=include) \
                $(shell pkg-config --cflags $(PACKAGES)) \
	$< $(shell $(CC) -print-file-name=libgccjit.so) \
           $(shell pkg-config --libs $(PACKAGES)) \
                -o $@

clean:
	$(RM) *.o minicomp-gccjit
