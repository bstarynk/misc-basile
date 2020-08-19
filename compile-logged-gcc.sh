#!/bin/bash -x
export GITID=$(git log -1|awk '/commit/{printf ("%.12s\n", $2); }')
[ -f logged-gcc ] && mv -v logged-gcc  logged-gcc~
/usr/bin/g++ -o logged-gcc$$ -Wall -Wextra -rdynamic -O -g -std=gnu++17 -DGITID=\"$GITID\" logged-gcc.cc  -lstdc++ && /bin/mv -v  logged-gcc$$ logged-gcc

[ -f logged-g++ ] || ln -sv logged-gcc logged-g++
