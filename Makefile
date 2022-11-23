## file misc-basile/Makefile
## on https://github.com/bstarynk/

.PHONY: all clean indent
FRAMAC=/usr/bin/frama-c
INDENT=/usr/bin/indent
CC=/usr/bin/gcc
CXX=/usr/bin/g++
RM=/bin/rm -vf
CFLAGS= -O2 -g -Wall -Wextra -I /usr/local/include/


all: manydl half bwc sync-periodically

manydl: manydl.c
	$(CC) $(CFLAGS) -rdynamic $^ -ldl -o $@

half: half.c
	$(CC) $(CFLAGS) $^  -o $@

bwc: bwc.c
	$(CC) $(CFLAGS) $^  -o $@

sync-periodically: sync-periodically.c
	$(CC) $(CFLAGS) $^  -o $@
