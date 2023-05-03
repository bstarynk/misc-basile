#!/bin/bash -x
# file misc-basile/tinyed-build.sh
CXX=g++-13
ASTYLE=astyle
ASTYLEFLAGS='--verbose --indent=spaces=2  --style=gnu'
$ASTYLE $ASTYLEFLAGS fox-tinyed.cc
CXXFLAGS='-Wall  -Wextra -Woverloaded-virtual -Wshadow -O -g'
$CXX $CXXFLAGS $(pkg-config --cflags fox17) fox-tinyed.cc $(pkg-config --libs fox17) -o fox-tinyed
