#!/bin/bash -x
# file misc-basile/tinyed-build.sh
CXX=g++-13
#ASTYLE=astyle
#ASTYLEFLAGS='--verbose --indent=spaces=2  --style=gnu'
#$ASTYLE $ASTYLEFLAGS fox-tinyed.cc
PACKAGES="fox17 jsoncpp"
GIT_ID=$(git log --format=oneline -q -1 | cut -c1-14)
CXXFLAGS='-Wall  -Wextra -Woverloaded-virtual -Wshadow -O -g'
$CXX $CXXFLAGS -fPIC -fPIE -DGIT_ID=\"$GIT_ID\" $(pkg-config --cflags $PACKAGES) fox-tinyed.cc $(pkg-config --libs $PACKAGES) -o fox-tinyed
