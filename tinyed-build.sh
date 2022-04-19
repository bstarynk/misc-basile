#!/bin/bash -x
# file misc-basile/tinyed-build.sh
CXX=g++
ASTYLE=astyle
ASTYLEFLAGS='--verbose --indent=spaces=2  --style=gnu'
$ASTYLE $ASTYLEFLAGS fox-tinyed.cc
CXXFLAGS='-O -g'
$CXX $CXXFLAGS $(pkg-config --cflags fox17) fox-tinyed.cc $(pkg-config --libs fox17)
