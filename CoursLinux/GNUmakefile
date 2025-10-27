#!/usr/bin/gmake
## SPDX-License-Identifier: GPL-3.0-or-later
## fichier misc-basile/CoursLinux/GNUmakefile
## cf https://github.com/bstarynk/misc-basile
## 
# https://tex.stackexchange.com/a/527167
.PHONY: all clean
export GIT_ID=$(shell git log --format=oneline -q -1 | cut -c1-13)

## fichiers générés - generated LaTeX files
LATEXGENERES= gener-macro.tex

## fichiers inclus - included latex files clinc-*.tex
LATEXINCLUS= $(wildcard clinc-*.tex)

FICHIERS_SVG= $(wildcard *.svg)
FICHIERS_PNG= $(patsubst %.svg, %.png, $(FICHIERS_SVG))

RM ?= /bin/rm -vf
MV ?= /bin/mv -f
## pour plus tard 
-include "cours-linux.mk"

LUALATEX ?= /usr/bin/lualatex
CONVERT ?= /usr/bin/convert

LATEXOPTIONS  ?=  --halt-on-error --shell-escape --interaction=batchmode
BIBTEXOPTIONS ?= 
DATE ?= /usr/bin/date
PRINTF ?= /usr/bin/printf

all: cours-linux-Starynkevitch.pdf cours-shell-Starynkevitch.pdf | GNUmakefile

clean:
	$(RM) *.log *.aux *.fls *~ gener*.tex *.pdf *~ $(FICHIERS_PNG)

cours-linux-Starynkevitch.pdf: clinux.pdf | GNUmakefile
	/bin/cp --verbose  --backup $^ $@

cours-shell-Starynkevitch.pdf: cshell.pdf | GNUmakefile
	/bin/cp --verbose  --backup $^ $@

%.png: %.svg
	$(CONVERT) $^ $@

clinux.pdf: clinux.tex gener-macro.tex $(FICHIERS_PNG) | GNUmakefile \
   $(LATEXGENERES) $(LATEXINCLUS) #clinux-biblio.bib 
	$(LUALATEX) $(LATEXOPTIONS)  clinux
	$(LUALATEX) $(LATEXOPTIONS)  clinux


cshell.pdf: cshell.tex gener-macro.tex $(FICHIERS_PNG) | GNUmakefile \
   $(LATEXGENERES) $(LATEXINCLUS) #clinux-biblio.bib 
	$(LUALATEX) $(LATEXOPTIONS)  cshell
	$(LUALATEX) $(LATEXOPTIONS)  cshell

%.png: %.svg
	$(CONVERT) $^ $@



gener-macro.tex: | GNUmakefile
	if [ -f "$@" ]; then $(MV) $@ $@~ ; fi
	echo "% fichier genéré gener-macro.tex" > $@
	echo '% gitid' "$(GIT_ID)" >> $@
	$(DATE) +"\\newcommand{\\clbdate}{%c}" >> $@
	$(PRINTF) "\\\\newcommand{%sclbgitid}{%s}\n" "\\" "$(GIT_ID)" >> $@
