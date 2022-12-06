#!/bin/bash -x
# SPDX-License-Identifier: GPL-3.0-or-later
# file misc-basile/build-clever-framac.sh
# Author(s):
#     Basile Starynkevitch <basile@starynkevitch.net>
#
# © Copyright 2022 Commissariat à l'Energie Atomique
/usr/bin/astyle -v --style=gnu clever-framac.cc
/usr/bin/g++ -O -g -rdynamic clever-framac.cc -ldl -o clever-framac
