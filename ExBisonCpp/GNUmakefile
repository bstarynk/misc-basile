# file misc-basile/ExBisonCpp/GNUmakefile
# ©2023 CEA and Basile Starynkevitch <basile@starynkevitch.net> and
# <basile.starynkevitch@cea.fr>
BISONCPP= bisonc++
CXX= g++
CXXFLAGS= -Wall -Wextra -O -g
RM= rm -vf
ASTYLE= astyle
.PHONY: all clean indent

all: mytest

clean:
	$(RM) *.o *.a *~ *.orig mytest

mytest: testbisoncp.o maintestb.o
	$(CXX) $(CFLAGS)  testbisoncp.o maintestb.o -o mytest
indent:
	$(ASTYLE) -v  --style=gnu  maintestb.cc
