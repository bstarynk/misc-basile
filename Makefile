## file misc-basile/Makefile
## on https://github.com/bstarynk/

.PHONY: all clean indent analyze-framac framac-bwc framac-sync-periodically framac-manydl framac-half

FRAMAC=/usr/bin/frama-c
FRAMALIBC=/usr/share/frama-c/libc/
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


analyze-framac:  framac-bwc framac-sync-periodically framac-manydl framac-half

framac-bwc:
	$(FRAMAC)  -cpp-command '$(CC) -C -E -I $(FRAMALIBC) -I. -I/usr/include -x c'  -eva -eva-verbose 2  bwc.c

framac-sync-periodically:
	$(FRAMAC)  -cpp-command '$(CC) -C -E -I$(FRAMALIBC)  -I. -I/usr/include -x c'  -eva  -eva-verbose 2 sync-periodically.c

framac-manydl:
	$(FRAMAC)  -cpp-command '$(CC) -C -E -I $(FRAMALIBC)  -I. -I/usr/include -x c'  -eva  -eva-verbose 2 $(FRAMALIBC)/string.c manydl.c

framac-half:
	$(FRAMAC) -cpp-command '$(CC) -C -E -I /usr/share/frama-c/libc/ -I. -I/usr/include -x c'  -eva  -eva-verbose 2  half.c
