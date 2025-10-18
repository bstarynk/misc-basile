#!/usr/bin/gmake
## SPDX-License-Identifier: GPL-3.0-or-later
## fichier misc-basile/CoursLinux/GNUmakefile
## cf https://github.com/bstarynk/misc-basile
## 

.PHONY: all clean
export GIT_ID=$(shell git log --format=oneline -q -1 | cut -c1-15)

## fichiers générés - generated LaTeX files
LATEXGENERES= gener-macro.tex

## fichiers inclus - included latex files clinc-*.tex
LATEXINCLUS= $(wildcard clinc-*.tex)

## pour plus tard
-include "cours-linux.mk"

LUALATEX ?= /usr/bin/lualatex
BIBTEX ?= /usr/bin/bibtex
LATEXOPTIONS  ?=  --halt-on-error --shell-escape --interaction=batchmode
BIBTEXOPTIONS ?=  --halt-on-error --shell-escape --interaction=batchmode
RM ?= /bin/rm -vf
MV ?= /bin/mv -f
DATE ?= /usr/bin/date
PRINTF ?= /usr/bin/printf

all: cours-linux-Starynkevitch.pdf

clean:
	$(RM) *.log *.aux *.fls *~ gener*.tex

cours-linux-Starynkevitch.pdf:  clinux.tex gener-macro.tex \
   $(LATEXGENERES) $(LATEXINCLUS) clinux-biblio.bib 
	$(LUALATEX) $(LATEXOPTIONS)  clinux
	$(BIBTEX)   $(BIBTEXOPTIONS)  clinux
	$(LUALATEX) $(LATEXOPTIONS)  clinux
	$(LUALATEX) $(LATEXOPTIONS)  clinux

gener-macro.tex: | GNUmakefile
	if [ -f "$@" ]; then $(MV) $@ $@~ ; fi
	echo "% fichier genéré gener-macro.tex" > $@
	echo '% gitid' "$(GIT_ID)" >> $@
	$(DATE) +"\\newcommand{clbdate}{%c}" >> $@
	$(PRINTF) "\\\\newcommand{clbgitid}{%s}\n" "$(GIT_ID)" >> $@
