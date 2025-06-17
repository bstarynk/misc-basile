## file misc-basile/CarburEx/GNUMakefile
## SPDX-License-Identifier: GPL-3.0-or-later
## © Copyright (C) by Basile Starynkevitch 2023 - 2025
## program released under GNU General Public License v3+
## on https://github.com/bstarynk/misc-basile/CarburEx

# the Carburetta parser generator from https://carburetta.com/
CARBURETTA= /usr/local/bin/carburetta
CARBURETTA_VERSION:=$(shell $(CARBURETTA) --version | /bin/tr -d '()')
GITID=$(shell git log --format=oneline -q -1 | cut -c1-10)
CXX:= $(shell /usr/bin/which g++)
CXXFLAGS= -O -g -Wall -Wextra -std=gnu++17 -DGITID=\"$(GITID)\" \
          '-DCARBURETTA_VERSION="$(CARBURETTA_VERSION)"' -DCXX="$(CXX)"
ASTYLE?= /usr/bin/astyle
PRINTF?= /usr/bin/printf
ASTYLEFLAGS= --verbose   --style=gnu  --indent=spaces=2
RM= /bin/rm -vf

.PHONY: all clean indent test

.SECONDARY: carbex_parser.cc carbex_parser.hh

all: carbex GNUmakefile

carbex: carbex_parser.o main.o | GNUmakefile
	@$(PRINTF) '\nbuilding %s git %s with %s CXX=%s\n' $@ '$(GITID)' '$(CARBURETTA_VERSION)' $(CXX)
	$(LINK.cc) $^ -o $@ -lgc

main.o: main.cc carbex.hh | GNUmakefile

carbex_parser.o: carbex_parser.cc carbex.hh | GNUmakefile

carbex_parser.cc carbex_parser.hh: carbex_parser.cbrt
	$(CARBURETTA) --c carbex_parser.cc carbex_parser.cbrt

indent:
	$(ASTYLE) $(ASTYLEFLAGS) carbex.hh
	$(ASTYLE) $(ASTYLEFLAGS) main.cc

clean:
	$(RM) *~ *.orig
	$(RM) *.o
	$(RM) carbex_parser.cc carbex_parser.hh
	$(RM) carbex

test: carbex GNUmakefile

### end of file misc-basile/CarburEx/GNUMakefile
