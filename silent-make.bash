#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# © Copyright (C) 2025 Basile Starynkevitch
#
# file ~/scripts/silent-make running GNU make silently but keeping
# the output on your $HOME/tmp/_silent-make.out
#
# also on github.com/bstarynk/misc-basile/silent-make.bash
#
#
# script licensed under GNU public licence v3+
# see https://www.gnu.org/licenses/#GPL
# so NO WARRANTY
make "$@" >& $HOME/tmp/_silent-make.out
#### end of silent-make script
