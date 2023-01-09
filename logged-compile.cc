// SPDX-License-Identifier: GPL-3.0-or-later
/* file logged-compile.cc from https://github.com/bstarynk/misc-basile/

   a wrapper with logging of GCC or Clang compilation
   (for g++ or gcc or clang or clang++ on Linux)

    ©  Copyright Basile Starynkevitch and CEA 2023
   program released under GNU General Public License

   this is free software; you can redistribute it and/or modify it under
   the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 3, or (at your option) any later
   version.

   this is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.
*/

#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#include <functional>

#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <sysexits.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <argp.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include <sqlite3.h>


char*myprogname;
#ifndef GIT_ID
#error GIT_ID should be compile defined
#endif

#ifndef GCC_EXEC
#define GCC_EXEC "/usr/bin/gcc"
#endif

#ifndef GXX_EXEC
#define GXX_EXEC "/usr/bin/g++"
#endif


const char* argp_program_version = __FILE__ " git " GIT_ID " built " __DATE__;

const char * argp_program_bug_address =
    "Basile Starynkevitch <basile@starynkevitch.net";

const char* mygcc;
const char* mygxx;
const char* mysqlitepath;
const char* mysqliterequest;
sqlite3* mysqlitedb;
bool debug_enabled;
int exitcode;

#define DEBUGLOG_AT(Lin,Log) do {if (debug_enabled) \
      std::clog << "¤¤" <<__FILE__<< ":" << Lin << " " << Log << std::endl; } while(0)
#define DEBUGLOG_AT_BIS(Lin,Log) DEBUGLOG_AT(Lin,Log)
#define DEBUGLOG(Log) DEBUGLOG_AT_BIS(__LINE__,Log)

class Do_Output
{
    std::function<void(std::ostream&)> _outfun;
public:
    Do_Output(std::function<void(std::ostream&)> f) : _outfun(f) {};
    ~Do_Output() = default;
    Do_Output(const Do_Output&) = delete;
    Do_Output(Do_Output&&) = delete;
    void out(std::ostream&out) const
    {
        _outfun(out);
    };
};

std::ostream&
operator << (std::ostream&out, const Do_Output&d)
{
    d.out(out);
    return out;
};

double
get_float_time(clockid_t cid)
{
    struct timespec ts= {0,0};
    if (!clock_gettime(cid, &ts))
        return ts.tv_sec*1.0 + ts.tv_nsec*1.0e-9;
    else
        return NAN;
};				// end get_float_time

enum {
    MYOPT__NONE,
    MYOPT_DEBUG=1000,
    MYOPT_COMPILER,
    MYOPT_SYSLOG,
    MYOPT_SQLITE,
};

struct argp_option my_progoptions[] =
{
    /* ====== debug this program ======= */
    {
        /*name:*/ "debug",
        /*key:*/ MYOPT_DEBUG,
        /*arg:*/ NULL,
        /*flags:*/ 0,
        /*doc:*/ "The real compiler is COMPILER (e.g. /usr/bin/gcc)",
        /*group:*/ 0,
    },
    /* ====== the underlying compiler ======= */
    {
        /*name:*/ "compiler",
        /*key:*/ MYOPT_COMPILER,
        /*arg:*/ "COMPILER",
        /*flags:*/ 0,
        /*doc:*/ "The real compiler is COMPILER (e.g. /usr/bin/gcc)",
        /*group:*/ 0,
    },
    /* ====== log using syslog(3) ======= */
    {
        /*name:*/ "syslog",
        /*key:*/ MYOPT_SYSLOG,
        /*arg:*/ NULL,
        /*flags:*/ 0,
        /*doc:*/ "Log using syslog(3)",
        /*group:*/ 0,
    },
    /* ====== log in sqlite database ======= */
    {
        /*name:*/ "sqlite",
        /*key:*/ MYOPT_SQLITE,
        /*arg:*/ "SQLITE_DB",
        /*flags:*/ 0,
        /*doc:*/ "Log in the SQLITE_DB database for sqlite3",
        /*group:*/ 0,
    },

    /* ======= terminating empty option ======= */
    {   /*name:*/(const char*)0, ///
        /*key:*/0, ///
        /*arg:*/(const char*)0, ///
        /*flags:*/0, ///
        /*doc:*/(const char*)0, ///
        /*group:*/0 ///
    }
};				// end my_progoptions



/* Parse a single option. */
static error_t
my_parse_opt (int key, char *arg, struct argp_state *state)
{
} // end my_parse_opt



const char my_args_doc[] = "compiled files and options";
const char  my_doc[] = "frontend to compiler";

static struct argp my_argp = { my_progoptions, my_parse_opt, my_args_doc, my_doc };

int
main(int argc, char**argv)
{
    myprogname = argv[0];
    if (argc>1 && !strcmp(argv[1], "--debug"))
        debug_enabled = true;
    int firstix = -1;
    int argerr = argp_parse(&my_argp, argc, argv, 0, &firstix, NULL);
    if (argerr > 0)
    {
        fprintf(stderr, "%s: failed to parse program arguments\n",
                myprogname);
        argp_help (&my_argp, stderr, 0, myprogname);
        exit(EXIT_FAILURE);
    };
}


/// end of file logged-compile.cc
