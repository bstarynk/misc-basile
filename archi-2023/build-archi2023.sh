#!/bin/bash
# file bismon/talks/Lamsade-21nov2019/build.sh
# in http://github.com/bstarynk/bismon/
for svgfile in *.svg ; do
    svgbase=$(basename $svgfile .svg)
    inkscape --without-gui --export-file=$svgbase.pdf $svgfile
    inkscape --without-gui --export-file=$svgbase.eps $svgfile
done
for dotfile in *.dot ; do
    dotbase=$(basename $dotfile .dot)
    dot -v -Teps -o $dotbase.eps  $dotfile
    dot -v -Tpdf -o $dotbase.pdf  $dotfile
    dot -v -Tsvg -o $dotbase.svg  $dotfile
done
lualatex --shell-escape  --halt-on-error archi2023
lualatex --shell-escape  --halt-on-error archi2023

#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#% Local Variables: ;;
#% compile-command: "./build-archi2023.sh" ;;
#% End: ;;
#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
