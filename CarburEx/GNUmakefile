## file misc-basile/CarburEx/GNUMakefile

# the Carburetta parser generator from https://carburetta.com/
CARBURETTA= /usr/local/bin/CARBURETTA
GITID=$(shell git log --format=oneline -q -1 | cut -c1-10)
CXX= g++-12
CXXFLAGS= -O -g -Wall -Wextra -std=gnu++14 -DGITID=\"$(GITID)\"


.PHONY: all clean

.SECONDARY: carbex_parser.cc carbex_parser.hh

all: carbex

carbex: carbex_parser.o main.o
	$(LINK.cc) $^ -o $@

carbex_parser.cc carbex_parser.hh: carbex_parser.cbrt
	$(CARBURETTA) $^ --c carbex_parser.cc --h  carbex_parser.hh --x-utf8 

