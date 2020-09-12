#!/bin/bash -x
# file compile-run-as-misc.sh on https://github.com/bstarynk/misc-basile/
# GPLv3+ licensed free software
# © Copyright Basile Starynkevitch 2020 <basile@starynkevitch.net>
# 
export GITID=$(git log -1|awk '/commit/{printf ("%.12s\n", $2); }')
[ -f run-as-misc ] && mv -v run-as-misc run-as-misc~
if [ -z "$RUN_MISC_USER" ]; then
    printf "%s: environment variable RUN_MISC_USER is not set\n" $10 >& /dev/stderr
    exit 1
fi
export RUN_MISC_UID=$(gawk -F: "/$RUN_MISC_USER/{print \$3;}" /etc/passwd)
if [ -z "$RUN_MISC_UID" ]; then
    printf "%s: failed to find %s in /etc/passwd\n" $0 $RUN_MISC_USER >& /dev/stderr
    exit 1
else
    printf "%s: RUN_MISC_UID is %s for RUN_MISC_USER=%s\n" $0 $RUN_MISC_UID $RUN_MISC_USER
fi
if /usr/bin/gcc -o run-as-misc_$$ -DMISC_USER_ID=$RUN_MISC_UID -DGITID=\"$GITID\" -Wall -Wextra -O -g run-as-misc.c ; then
    mv -v run-as-misc_$$ run-as-misc
else
    printf "%s: COMPILATION of %s failed\n" $0 run-as-misc.c >& /dev/stderr
    exit 1
fi

sudo -E /bin/bash -c "chown $RUN_MISC_USER $PWD/run-as-misc && chmod u+s $PWD/run-as-misc"
ls -l run-as-misc
# eof compile-run-as-misc.sh
