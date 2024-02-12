#!/bin/bash -vx
# SPDX-License-Identifier: GPL-3.0-or-later
# file misc-basile/build-gtksrc-browser.sh
# © 2022 copyright CEA & Basile Starynkevitch

## see also https://wiki.debian.org/HowToGetABacktrace
## see also https://wiki.debian.org/Debuginfod
## consider export DEBUGINFOD_URLS="https://debuginfod.debian.net"
CC=gcc
MYGITID=$(git log -1|head -1|cut -b9-20)
OPTIMFLAGS=-O1
PACKAGES="glib-2.0  gobject-2.0 gio-2.0  gtk+-3.0 pango gtksourceview-4 libcjson"
indent -gnu gtksrc-browser.c
$CC $OPTIMFLAGS -Wall -Wextra -g -Wmissing-prototypes \
    -DGIT_ID=\"$MYGITID\" \
    $(pkg-config --cflags $PACKAGES) \
    gtksrc-browser.c  \
    -L /usr/local/lib/ \
    $(pkg-config --libs $PACKAGES) \
    -o gtksrc-browser
