#!/bin/bash -x
export GITID=$(git log -1|awk '/commit/{printf ("%.12s\n", $2); }')
export MYPACKAGES='openssl sqlite3'
[ -f logged-gcc ] && mv -v logged-gcc  logged-gcc~
/usr/bin/g++ -o logged-gcc_$$ -Wall -Wextra -rdynamic \
	     $(pkg-config --cflags $MYPACKAGES) \
	     -O -g -std=gnu++17 -DGITID=\"$GITID\" \
	     logged-gcc.cc \
	     $(pkg-config --libs $MYPACKAGES) -lstdc++ && /bin/mv -v  logged-gcc_$$ logged-gcc

[ -f logged-g++ ] || ln -sv logged-gcc logged-g++
