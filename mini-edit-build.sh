#!/bin/bash -x

# SPDX-License-Identifier: GPL-3.0-or-later
# script mini-edit-build.sh
# GPLv3+ licensed.


. "$(dirname "0")/external/shflags/shflags"


# Flag to indicate whether clang should be used instead of g++; shflags creates
# the variable FLAGS_clang from this defnition.
DEFINE_boolean 'clang' false 'use clang as compiler instead of g++', 'c'

# This variable is a special flag provided by shflags to define the short help
# message that is displayed when the --help command line argument (short form
# -h) is passed to this script.
FLAGS_HELP="usage: $0 [flags]"

# Get shflags to evaluate script flags
FLAGS "$@" || exit $?
eval set -- "$FLAGS_ARGV"


# Main script logic
run_script() {
    if [ "$FLAGS_clang" -eq "$FLAGS_TRUE" ]; then
        CXX=clang
    else
        CXX=g++
    fi

    ASTYLE=astyle
    FLTKCONFIG=/usr/local/bin/fltk-config
    ASTYLEFLAGS='--verbose --indent=spaces=2  --style=gnu'
    $ASTYLE $ASTYLEFLAGS fltk-mini-edit.cc
    CXXFLAGS='-Wall  -Wextra -Woverloaded-virtual -Wshadow -O0 -g -std=gnu++17'
    GITID=$(git log --format=oneline -q -1 | cut '-d '  -f1 | tr -d '\n' | head -16c)

    if [ -x fltk-mini-edit ]; then
        /bin/mv -v fltk-mini-edit fltk-mini-edit~
    fi

    if [ $# == 0 ]; then
        $CXX $CXXFLAGS $($FLTKCONFIG --cflags) -DGITID=\"$GITID\" \
	    -DCXX_COMPILER=\"$CXX\" \
	    -DCOMPILE_SCRIPT=\"$0\" \
	    -DCXX_SOURCE_DIR=\"$(/bin/pwd)\" \
	    $(pkg-config --cflags jsoncpp) \
	    -rdynamic \
	    fltk-mini-edit.cc \
	    -L/usr/local/lib \
	    $($FLTKCONFIG --libs) \
	    $(pkg-config --libs jsoncpp) \
	    -lunistring -lX11 -lXext -lXinerama \
	    -lXcursor -lXrender -lXrandr -lXfixes -lXi -lfreetype \
	    -lfontconfig -lXft -lGL -ldl -lpthread -lrt -ljpeg -lpng -ltiff -lz -lGL \
	    -lbacktrace \
	    -o fltk-mini-edit
    else
        $CXX -fPIC -shared $CXXFLAGS $($FLTKCONFIG --cflags) -DGITID=\"$GITID\" \
	    -DCXX_COMPILER=\"$CXX\" \
	    -DCXX_SOURCE_DIR=\"$(/bin/pwd)\" \
	    $(pkg-config --cflags jsoncpp) \
	    $1 -o /tmp/$(basename $1).so
    fi
}


# Run script with passed flags
run_script "$@"
