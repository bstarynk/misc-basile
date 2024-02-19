#!/bin/bash -x
CXX=g++
#ASTYLE=astyle
FLTKCONFIG=/usr/local/bin/fltk-config
#ASTYLEFLAGS='--verbose --indent=spaces=2  --style=gnu'
#$ASTYLE $ASTYLEFLAGS fltk-mini-edit.cc
CXXFLAGS='-Wall  -Wextra -Woverloaded-virtual -Wshadow -O1 -g -std=gnu++17'
CXX_SOURCE_DIR=$(/bin/pwd)
GITID=$(git log --format=oneline -q -1 | cut '-d '  -f1 | tr -d '\n' | head -16c)
$CXX $CXXFLAGS $($FLTKCONFIG --cflags) \
     -DGITID=\"$GITID\" \
     -DCXX_SOURCE_DIR=\"$CXX_SOURCE_DIR\" \
     -DCOMPILE_SCRIPT=\"$0\" \
     $(pkg-config --cflags jsoncpp) \
     fltk-mini-edit.cc \
     $($FLTKCONFIG --libs) \
     $(pkg-config --libs jsoncpp) \
     -lbacktrace -lunistring \
   -o fltk-mini-edit
