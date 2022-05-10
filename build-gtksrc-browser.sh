#!/bin/bash -vx
# SPDX-License-Identifier: GPL-3.0-or-later
# file misc-basile/build-gtksrc-browser.sh
# © 2022 copyright CEA & Basile Starynkevitch
CC=gcc
OPTIMFLAGS=-Og
indent -gnu gtksrc-browser.c
$CC $OPTIMFLAGS -Wall -Wextra -g -Wmissing-prototypes \
    $(pkg-config --cflags \
	 glib-2.0  gobject-2.0 	gio-2.0  gtk+-3.0 pango gtksourceview-4) \
    gtksrc-browser.c  \
    $(pkg-config --libs \
	 glib-2.0  gobject-2.0 	gio-2.0  gtk+-3.0 pango gtksourceview-4) \
    -o gtksrc-browser
