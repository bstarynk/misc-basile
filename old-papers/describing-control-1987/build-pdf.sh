#!/bin/bash
# generate the git tag 
gittag=$(git log --format=oneline -1 --abbrev=16 --abbrev-commit -q|cut -d' ' -f1)
git log --format=oneline -1 --abbrev=16 --abbrev-commit -q | awk '{printf "\\newcommand{\\gitcommit}[0]{%s}\n", $1}' > generated-git-commit.tex

# run LaTeX first time
pdflatex -halt-on-error  desc-ctrl-1987 < /dev/null

# run bibtex for the bibliography
bibtex desc-ctrl-1987 < /dev/null

# run LaTeX the second time
pdflatex -halt-on-error  desc-ctrl-1987 < /dev/null

# run bibtex again
bibtex desc-ctrl-1987 < /dev/null

# run LaTeX the last time
pdflatex -halt-on-error  desc-ctrl-1987 < /dev/null
