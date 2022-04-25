#!/bin/bash -x
CXX=g++
ASTYLE=astyle
FLTKCONFIG=/usr/local/bin/fltk-config
ASTYLEFLAGS='--verbose --indent=spaces=2  --style=gnu'
$ASTYLE $ASTYLEFLAGS fltk-mini-edit.cc
CXXFLAGS='-Wall  -Wextra -Woverloaded-virtual -Wshadow -O -g -std=gnu++17'
$CXX $CXXFLAGS $($FLTKCONFIG --cflags)  fltk-mini-edit.cc  $($FLTKCONFIG --libs) -lX11 -lXext -lXinerama -lXcursor -lXrender -lXrandr -lXfixes -lXi -lfreetype -lfontconfig -lXft -lGL -ldl -lpthread -lrt -ljpeg -lpng -ltiff -lz -lGL -o fltk-mini-edit 
