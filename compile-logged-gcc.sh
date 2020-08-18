#!/bin/bash -x
export GITID=$(git log -1|awk '/commit/{printf ("%.12s\n", $2); }')
[ -f logged-gcc ] && mv -v logged-gcc  logged-gcc~
g++ -o logged-gcc -Wall -Wextra -rdynamic -O -g -std=gnu++17 -DGITID=\"$GITID\" logged-gcc.cc  -lstdc++
[ -f logged-g++ ] || ln -sv logged-gcc logged-g++
