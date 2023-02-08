#!/bin/bash -x
# SPDX-License-Identifier: GPL-3.0-or-later
# file misc-basile/build-clever-framac.sh
# Author(s):
#     Basile Starynkevitch <basile@starynkevitch.net>
#
# © Copyright 2022 Commissariat à l'Energie Atomique
GITID=$(git log --format=oneline -q -1 | cut '-d '  -f1 | tr -d '\n' | head -12c)
/usr/bin/astyle -v --style=gnu clever-framac.cc
/usr/bin/g++ -O -g $(guile-config compile) -rdynamic clever-framac.cc -DGIT_ID=\"$GITID\" -ldl $(guile-config link) -o clever-framac
