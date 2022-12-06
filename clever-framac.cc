// file misc-basile/clever-framac.cc
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Author(s):
 *      Basile Starynkevitch <basile@starynkevitch.net>
 */
//  © Copyright 2022 Commissariat à l'Energie Atomique

/*************************
 * License:
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#include <fstream>
#include <iostream>
#include <unistd.h>
#include <getopt.h>

/*** see getopt(3) man page mentionning
 *      extern char *optarg;
 *      extern int optind, opterr, optopt;
 ***/

const char*progname;
int verbose_opt;
int version_opt;
int framac_opt;


enum clever_flags_en
{
    no_flags=0,
    help_flag='h',
    verbose_flag='V',
    framac_flag='F',
    version_flag=1000,
};

const struct option long_clever_options[] =
{
    {"verbose", no_argument, &verbose_opt, verbose_flag},
    {"version", no_argument, &version_opt, version_flag},
    {"framac", required_argument, &framac_opt, framac_flag},
    {0,0,0,0}
};

void
parse_program_arguments(int argc, char**argv)
{
    int c=0;
    int option_index= -1;
#warning incomplete parse_program_arguments in clever-framac.cc
    do
        {
            option_index = -1;
            c = getopt_long(argc, argv,
                            "VhF:",
                            long_clever_options, &option_index);
            if (c<0)
                break;
            if (long_clever_options[option_index].flag != nullptr)
                continue;
        }
    while (c>=0);
    if (version_opt)
        {
            printf("%s: version git %s built %s at %s\n",
                   progname, GIT_ID, __DATE__, __TIME__);
        }
} // end parse_program_arguments

int
main(int argc, char*argv[])
{
    progname = argv[0];
    parse_program_arguments(argc, argv);
} // end main

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./build-clever-framac.sh" ;;
 ** End: ;;
 ****************/
