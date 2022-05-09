#!/bin/bash -vx
# SPDX-License-Identifier: GPL-3.0-or-later
# file misc-basile/build-gtksrc-browser.sh
# © 2022 copyright CEA & Basile Starynkevitch
CC=gcc
OPTIMFLAGS=-Og
indent -gnu gtksrc-browser.c
$CC $OPTIMFLAGS -Wall -Wextra -g -Wmissing-prototypes \
    $(pkg-config --cflags --libs \
		 gobject-2.0 glib-2.0 gtk+-3.0 gtksourceview-4) \
    -o gtksrc-browser
