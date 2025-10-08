#!/usr/bin/gmake
## SPDX-License-Identifier: GPL-3.0-or-later
## fichier misc-basile/CoursLinux/GNUmakefile
## cf https://github.com/bstarynk/misc-basile
## 

.PHONY: all clean
export GIT_ID=$(shell git log --format=oneline -q -1 | cut -c1-15)

## pour plus tard
-include "cours-linux.mk"

LUALATEX ?= /usr/bin/lualatex

## fichiers générés - generated LaTeX files
LATEXGENERES= gener-macro.tex gener-intro.tex

RM ?= /bin/rm -vf
MV ?= /bin/mv -f
DATE ?= /usr/bin/date
PRINTF ?= /usr/bin/printf

all: cours-linux-Starynkevitch.pdf

clean:
	$(RM) *.log *~ gener*.tex

 cours-linux-Starynkevitch.pdf:  $(LATEXGENERES) clinux.tex

gener-macro.tex:
	$(MV) $@ $@~
	echo "% fichier genéré gener-macro.tex" > $@
	$(DATE) +"\\newcommand{cldate}{%c}" >> $@
	$(PRINTF) "\\newcommand{clgitid}{%s}\n" "$(GITID) >> $@
