#!/bin/bash -x
# SPDX-License-Identifier: GPL-3.0-or-later
# file misc-basile/build-browserfox.sh
# © 2022 - 2023 copyright CEA & Basile Starynkevitch
CXX=./logged-g++
if git status|grep -q 'nothing to commit' ; then
    endgitid=''
else
    endgitid='+'
fi

GITID="$(git log --format=oneline -q -1 | cut -c1-10)$endgitid"
ASTYLE=astyle
ASTYLEFLAGS='--verbose --indent=spaces=2  --style=gnu'
$ASTYLE $ASTYLEFLAGS browserfox.cc
CXXFLAGS='-Wall  -Wextra -Woverloaded-virtual -Wshadow -O1 -g -DGIT_ID=\"$(GITID)\"'
$CXX $CXXFLAGS $(pkg-config --cflags fox17) browserfox.cc $(pkg-config --libs fox17) \
    -o browserfox
