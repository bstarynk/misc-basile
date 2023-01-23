// file misc-basile/clever-framac.cc
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Author(s):
 *      Basile Starynkevitch <basile@starynkevitch.net>
 */
//  © Copyright 2022 - 2023 Commissariat à l'Energie Atomique

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
#include <map>
#include <string>
#include <vector>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
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
int argframac_opt;
int help_opt;
int sourcelist_opt;

char* sourcelist_path;

std::vector<std::string> my_prepro_options;

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
    static std::map<std::string, Source_file*> srcf_dict;
public:
    Source_file(const std::string& path, source_type ty=srcty_c)
        : srcf_path(path), srcf_type(ty)
    {
        srcf_dict.insert({path,this});
    };
    Source_file (const Source_file&) = default;
    Source_file(Source_file&) = default;
    ~Source_file()
    {
        srcf_dict.erase(srcf_path);
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
    Source_file& operator = (Source_file&) = default;
};				// end Source_file

std::map<std::string, Source_file*> Source_file::srcf_dict;
std::vector<Source_file> my_srcfiles;


/// add a source file to analyze
void add_source_file(const char*srcpath);


/// Add a file containing the list of files to analyze;  empty lines
/// and lines starting with an hash # are skipped, per Unix tradition.
/// a line starting with @ means that it is a list of files
/// a line starting with | or ! means to popen to get the list of files.
void add_sources_list(const char*listpath);


enum clever_flags_en
{
    no_flags=0,
    help_flag='h',
    verbose_flag='V',
    argframac_flag='a',
    framacexe_flag='F',
    sourcelist_flag='s',
    version_flag=1000,
};

const struct option long_clever_options[] =
{
    {"verbose", no_argument, &verbose_opt, verbose_flag},
    {"version", no_argument, &version_opt, version_flag},
    {"help", no_argument, &help_opt, help_flag},
    {"framac", required_argument, &framac_opt, framacexe_flag},
    {"argframac", required_argument, &argframac_opt, argframac_flag},
    {"sources", required_argument, &sourcelist_opt, sourcelist_flag},
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
           "\t -a|--argframac <arg>   # argument to Frama-C\n"
           "\t -I <incldir>           # preprocessing include\n"
           "\t -D <definition>        # preprocessing definition\n"
           "\t -U <undefine>          # preprocessing undefine\n"
           "\t -l | --sources <slist> # read list of files (one per line) from <sfile>\n"
           "\t                        # if it starts with ! or | use popen\n"
           "\t                        # if it starts with @ it a list of files\n"
           "\t ... <source files>     # sources are files *.c ...\n"
           "\t                        # ... or C++ files *.cc\n"
           " (preprocessing options are passed to Frama-C)\n"
           "\n See https://frama-c.com/ for details on Frama-C ...\n"
           "Our gitid is %s (file %s compiled %s)\n"
           "\n",
           progname, framacexe, GIT_ID, __FILE__, __DATE__);
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
            optarg = nullptr;
            c = getopt_long(argc, argv,
                            "VhF:a:l:U:D:I:",
                            long_clever_options, &option_index);
            if (c<0)
                break;
            if (c=='V')
                verbose_opt=(int)true;
            else if (c=='I' || c=='D' || c=='U')
                {
                    if (!optarg)
                        {
                            fprintf(stderr,
                                    "%s: missing preprocessing option #%d (git %s)\n",
                                    progname, option_index, GIT_ID);
                            exit(EXIT_FAILURE);

                        }
                    else if (!optarg[0])
                        {
                            fprintf(stderr,
                                    "%s: empty preprocessing option #%d (git %s)\n",
                                    progname, option_index, GIT_ID);
                            exit(EXIT_FAILURE);
                        }
                    else
                        {
                            std::string opt;
                            opt.push_back('-');
                            opt.push_back(c);
                            opt.append(std::string{optarg});
                            my_prepro_options.push_back(opt);
                        }
                }
            else if (long_clever_options[option_index].flag != nullptr)
                continue;
            if (version_opt)
                {
                    printf("%s: version git %s built %s at %s\n",
                           progname, GIT_ID, __DATE__, __TIME__);
                };
            if (help_opt)
                {
                    show_help();
                    help_opt = false;
                }
            if (framac_opt)
                {
                    framacexe = optarg;
                    framac_opt = false;
                };
            if (sourcelist_opt)
                {
                    add_sources_list(optarg);
                    sourcelist_opt = false;
                };
        }
    while (c>=0);
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
                    enum source_type styp = srcty_NONE;
                    if (curarg[0] == '-')
                        {
                            fprintf(stderr,
                                    "%s: bad file argument#%d: %s (consider giving ./%s instead)\n",
                                    progname, optind, curarg, curarg);
                            exit(EXIT_FAILURE);
                        }
                    if (access(curarg, R_OK))
                        {
                            perror(curarg);
                            exit(EXIT_FAILURE);
                        };
                    int curlen = strlen(curarg);
                    if (curlen < 4)
                        {
                            fprintf(stderr,
                                    "%s: too short argument#%d: %s\n",
                                    progname, optind, curarg);
                            exit(EXIT_FAILURE);
                        };
                    if (curarg[curlen-2] == '.' && curarg[curlen-1] == 'c')
                        styp = srcty_c;
                    else if (curarg[curlen-3] == '.'
                             && curarg[curlen-2] == 'c' && curarg[curlen-1] == 'c')
                        styp = srcty_cpp;
                    else
                        {
                            fprintf(stderr,
                                    "%s: unexpected argument#%d: %s not ending with .c or .cc\n",
                                    progname, optind, curarg);
                            exit(EXIT_FAILURE);
                        }
                    resolvedpath = realpath(curarg, NULL);
                    // so resolvedpath has been malloced
                    if (strlen(resolvedpath) < 4)
                        {
                            fprintf(stderr,
                                    "%s: too short argument#%d: %s == %s\n",
                                    progname, optind, curarg, resolvedpath);
                            exit(EXIT_FAILURE);
                        };
                    for (const char*pc = resolvedpath; *pc; pc++)
                        if (isspace(*pc))
                            {
                                fprintf(stderr,
                                        "%s: unexpected space in argument#%d: %s == %s\n",
                                        progname, optind, curarg, resolvedpath);
                                exit(EXIT_FAILURE);
                            };
                    Source_file cursrcf(resolvedpath, styp);
                    my_srcfiles.push_back(cursrcf);
                    free ((void*)resolvedpath);
                    optind++;
                }
        }
} // end parse_program_arguments


void
add_source_file(const char*srcpath)
{
    struct stat srcstat = {};
    assert(srcpath != nullptr);
    if (stat(srcpath, &srcstat) < 0)
        {
            fprintf(stderr, "%s: failed to stat %s on %s (%s)\n",
                    progname, srcpath, myhost, strerror(errno));
            exit(EXIT_FAILURE);
        };
    if (srcstat.st_mode & S_IFMT != S_IFREG)
        {
            fprintf(stderr, "%s: source %s is not a regular file on %s\n",
                    progname, srcpath, myhost);
            exit(EXIT_FAILURE);
        };
    const char* rp = realpath(srcpath, nullptr);
    if (!rp || !rp[0])
        {
            fprintf(stderr, "%s: failed to find real path of %s on %s (%s)\n",
                    progname, srcpath, myhost, strerror(errno));
            exit(EXIT_FAILURE);
        };
    int lenrp = strlen(rp);
    if (lenrp < 4)
        {
            fprintf(stderr, "%s: real path of %s on %s is too short: %s\n",
                    progname, srcpath, myhost, rp);
            exit(EXIT_FAILURE);
        };
    if (rp[lenrp-1] == 'c' && rp[lenrp-2] == '.')
        {
            /* add a C file */
            Source_file csrc(rp, srcty_c);
            my_srcfiles.push_back(csrc);
        }
    else if (rp[lenrp-3] == '.'
             && rp[lenrp-2] == 'c' && rp[lenrp-1] == 'c')
        {
            /* add a C++ file */
            Source_file cppsrc(rp, srcty_cpp);
            my_srcfiles.push_back(cppsrc);
        }
    free((void*)rp);
} // end add_source_file

void
add_sources_list(const char*listpath)
{
    char* linbuf = nullptr;
    size_t sizbuf = 0;
    assert(listpath != nullptr);
    bool isapipe = listpath[0] == '!' || listpath[0] == '|';
    FILE *f = isapipe?popen(listpath+1, "r"):fopen(listpath, "r");
    if (!f)
        {
            fprintf(stderr, "%s: failed to add sources using %s %s (%s)\n",
                    progname, isapipe?"pipe":"file",
                    listpath, strerror(errno));
            exit(EXIT_FAILURE);
        };
    const int inisiz=128;
    linbuf = (char*)calloc(1, inisiz);
    if (!linbuf)
        {
            fprintf(stderr, "%d: failed to allocate line buffer (near %s:%d) of %d bytes (%s)\n",
                    progname, __FILE__, __LINE__, inisiz, strerror(errno));
            exit(EXIT_FAILURE);
        }
    sizbuf = (size_t)inisiz;
    do
        {
            memset (linbuf, 0, sizbuf);
            ssize_t rsz = getline(&linbuf, &sizbuf, f);
            if (rsz<0)
                break;
            if (linbuf==nullptr)
                break;
            if (rsz < sizbuf && linbuf[rsz] == '\n')
                linbuf[rsz] = (char)0;
            if (linbuf[0] == '#')
                continue;
            if (linbuf[0] == '@')
                add_sources_list(linbuf+1);
            else
                add_source_file(linbuf);
        }
    while (!feof(f));
    free(linbuf);
    if (isapipe)
        pclose(f);
    else
        fclose(f);
} // end add_sources_list



int
main(int argc, char*argv[])
{
    progname = argv[0];
    if (argc>1 && (!strcmp(argv[1], "--verbose") || !strcmp(argv[1], "-V")))
        verbose_opt = true;
    parse_program_arguments(argc, argv);
    if (verbose_opt)
        printf("%s running verbosely on %s pid %d git %s with %d source files\n", progname,
               myhost, (int)getpid(), GIT_ID, (int) my_srcfiles.size());
#warning should run framac on the collected source files....
} // end main

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./build-clever-framac.sh" ;;
 ** End: ;;
 ****************/
