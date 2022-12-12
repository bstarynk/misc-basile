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
#include <cstring>
#include <unistd.h>
#include <getopt.h>

/*** see getopt(3) man page mentionning
 *      extern char *optarg;
 *      extern int optind, opterr, optopt;
 ***/

char myhost[80];
const char*progname;
const char*framacexe = "/usr/bin/frama-c";
int verbose_opt;
int version_opt;
int framac_opt;
int help_opt;

enum source_type
{
    srcty_NONE,
    srcty_c,
    srcty_cpp
};				// end source_type

class Source_file
{
    std::string srcf_path;
    source_type srcf_type;
#warning should have a dictionary of Source_file-s
public:
    Source_file(const std::string& path, source_type ty=srcty_c)
        : srcf_path(path), srcf_type(ty)
    {
    };
    ~Source_file()
    {
        srcf_path.erase();
        srcf_type = srcty_NONE;
    };
    const std::string& path(void) const
    {
        return srcf_path;
    };
    source_type type() const
    {
        return srcf_type;
    };
    Source_file(Source_file&) = default;
};				// end Source_file

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
    {"help", no_argument, &help_opt, help_flag},
    {"framac", required_argument, &framac_opt, framac_flag},
    {0,0,0,0}
};

void
show_help(void)
{
    printf("%s usage:\n"
           "\t -V|--verbose           # output more messages\n"
           "\t --version              # give version information\n"
           "\t -h|--help              # give this help\n"
           "\t -F|--framac <framac>   # explicit Frama-C to be used\n"
           "\t                        # default is %s\n"
           "\t ... <source files>     # sources are C files *.c\n"
           "\t                        # or C++ files *.cc\n"
           "\n",
           progname, framacexe);
} // end show_help

void
parse_program_arguments(int argc, char**argv)
{
    int c=0;
    int option_index= -1;
    gethostname(myhost, sizeof(myhost)-1);
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
        };
    if (help_opt)
        {
            show_help();
        }
    if (framac_opt)
        {
            framacexe = optarg;
        };
    /* Handle any remaining command line arguments (not options). */
    if (optind < argc)
        {
            if (verbose_opt)
                printf("%s: processing %d arguments\n",
                       progname, argc-optind);
            while (optind < argc)
                {
                    const char*curarg = argv[optind];
                    const char*resolvedpath = NULL;
                    if (access(curarg, R_OK))
                        {
                            perror(curarg);
                            exit(EXIT_FAILURE);
                        };
                    resolvedpath = realpath(curarg, NULL);
                    // so resolvedpath has been malloced
                    if (strlen(resolvedpath) < 4)
                        {
                            fprintf(stderr, "%s: too short argument#%d: %s\n", progname, optind, resolvedpath);
                            exit(EXIT_FAILURE);
                        };
                    for (const char*pc = resolvedpath; *pc; pc++)
                        if (isspace(*pc))
                            {
                                fprintf(stderr, "%s: unexpected space in argument#%d: %s == %s\n", progname, optind, curarg, resolvedpath);
                                exit(EXIT_FAILURE);
                            };
#warning parse_program_arguments should do something with resolvedpath
                    free ((void*)resolvedpath);
                    optind++;
                }
        }
} // end parse_program_arguments


int
main(int argc, char*argv[])
{
    progname = argv[0];
    parse_program_arguments(argc, argv);
    if (verbose_opt)
        printf("%s running verbosely on %s pid %d git %s\n", progname,
               myhost, (int)getpid(), GIT_ID);
} // end main

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./build-clever-framac.sh" ;;
 ** End: ;;
 ****************/
