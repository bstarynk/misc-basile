#!/bin/bash -x
# SPDX-License-Identifier: GPL-3.0-or-later
# file misc-basile/build-browserfox.sh
# © 2022 copyright CEA & Basile Starynkevitch
CXX=g++
ASTYLE=astyle
ASTYLEFLAGS='--verbose --indent=spaces=2  --style=gnu'
$ASTYLE $ASTYLEFLAGS browserfox.cc
CXXFLAGS='-Wall  -Wextra -Woverloaded-virtual -Wshadow -O -g'
$CXX $CXXFLAGS $(pkg-config --cflags fox17) browserfox.cc $(pkg-config --libs fox17) \
    -o browserfox
