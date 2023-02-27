#!/bin/bash -x
#   SPDX-License-Identifier: GPL-3.0-or-later
# file compile-logged-gcc.sh on  https://github.com/bstarynk/misc-basile/
# GPLv3+ licensed free software
# © Copyright Basile Starynkevitch 2020 <basile@starynkevitch.net>
export GITID=$(git log -1|awk '/commit/{printf ("%.12s\n", $2); }')
export MYPACKAGES='openssl sqlite3'
[ -f logged-gcc ] && mv -v logged-gcc  logged-gcc~
/usr/bin/g++ -o logged-gcc_$$ -Wall -Wextra -rdynamic \
	     -L /usr/local/lib/ \
	     $(pkg-config --cflags $MYPACKAGES) \
	     -O2 -g -std=gnu++17 -DGITID=\"$GITID\" \
	     logged-gcc.cc \
	     $(pkg-config --libs $MYPACKAGES) -lstdc++ && /bin/mv -v  logged-gcc_$$ logged-gcc

compcode=$?
if [ $compcode -ne 0 ]; then
    exit $compcode
fi


[ -L logged-g++ ] || ln -svf logged-gcc logged-g++
