#!/bin/bash -x
# SPDX-License-Identifier: GPL-3.0-or-later
# file misc-basile/build-browserfox.sh
# © 2022 - 2024 copyright CEA & Basile Starynkevitch
CXX=g++
if git status|grep -q 'nothing to commit' ; then
    endgitid=''
else
    endgitid='+'
fi

GITID="$(git log --format=oneline -q -1 | cut -c1-15)$endgitid"
PACKAGES="fox17 jsoncpp" 
CXXFLAGS=(-Wall  -Wextra -Woverloaded-virtual -Wshadow -O1 -g)

$CXX -O -g -Wall -Wextra  -DGIT_ID=\"$GITID\" $CXXFLAGS $(pkg-config --cflags $PACKAGES) browserfox.cc $(pkg-config --libs $PACKAGES) \
    -o browserfox
