#!/usr/bin/gmake
## SPDX-License-Identifier: GPL-3.0-or-later
## fichier misc-basile/CoursLinux/GNUmakefile
## cf https://github.com/bstarynk/misc-basile
## 
# https://tex.stackexchange.com/a/527167
.PHONY: all clean
export GIT_ID=$(shell git log --format=oneline -q -1 | cut -c1-15)

## fichiers générés - generated LaTeX files
LATEXGENERES= gener-macro.tex

## fichiers inclus - included latex files clinc-*.tex
LATEXINCLUS= $(wildcard clinc-*.tex)

RM ?= /bin/rm -vf
MV ?= /bin/mv -f
## pour plus tard 
-include "cours-linux.mk"

LUALATEX ?= /usr/bin/lualatex
LATEXOPTIONS  ?=  --halt-on-error --shell-escape --interaction=batchmode
BIBTEXOPTIONS ?= 
DATE ?= /usr/bin/date
PRINTF ?= /usr/bin/printf

all: cours-linux-Starynkevitch.pdf
	/usr/bin/evince --preview $^

clean:
	$(RM) *.log *.aux *.fls *~ gener*.tex *.pdf *~

cours-linux-Starynkevitch.pdf: clinux.pdf
	/bin/cp --verbose  --backup $^ $@


clinux.pdf: clinux.tex gener-macro.tex \
   $(LATEXGENERES) $(LATEXINCLUS) #clinux-biblio.bib 
	$(LUALATEX) $(LATEXOPTIONS)  clinux
	$(LUALATEX) $(LATEXOPTIONS)  clinux

gener-macro.tex: | GNUmakefile
	if [ -f "$@" ]; then $(MV) $@ $@~ ; fi
	echo "% fichier genéré gener-macro.tex" > $@
	echo '% gitid' "$(GIT_ID)" >> $@
	$(DATE) +"\\newcommand{clbdate}{%c}" >> $@
	$(PRINTF) "\\\\newcommand{clbgitid}{%s}\n" "$(GIT_ID)" >> $@
