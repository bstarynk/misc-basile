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
#include <ostream>
#include <sstream>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <getopt.h>

/*** see getopt(3) man page mentionning
 *      extern char *optarg;
 *      extern int optind, opterr, optopt;
 ***/

char myhost[80];
const char*progname;
const char*framacexe = "/usr/bin/frama-c";
bool is_verbose;
char* sourcelist_path;
bool do_list_framac_plugins;
std::vector<std::string> my_prepro_options;
std::vector<std::string> my_framac_options;
extern "C" [[noreturn]] void cfr_fatal_error_at(const char*fil, int lin);
#define CFR_FATAL_AT(Fil,Lin,Log) do {			\
    std::ostringstream out##Lin;			\
    out##Lin << Log << std::endl;			\
    std::clog << "FATAL ERROR (" << progname << ") "	\
	     << Log << std::endl;			\
    cfr_fatal_error_at(Fil,Lin);			\
  } while(0)

#define CFR_BISFATAL_AT(Fil,Lin,Log) CFR_FATAL_AT((Fil),Lin,Log)

#define CFR_FATAL(Log) CFR_BISFATAL_AT(__FILE__,__LINE__,Log)

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

/// try to run Frama-C with a few arguments
void try_run_framac(const char*arg1, const char*arg2=nullptr,
                    const char*arg3=nullptr, const char*arg4=nullptr,
                    const char*arg5=nullptr);

/// add a source file to analyze
void add_source_file(const char*srcpath);


/// Add a file containing the list of files to analyze;  empty lines
/// and lines starting with an hash # are skipped, per Unix tradition.
/// a line starting with @ means that it is a list of files
/// a line starting with | or ! means to popen to get the list of files.
void add_sources_list(const char*listpath);

//// see https://www.gnu.org/software/libc/manual/html_node/Getopt-Long-Options.html
enum clever_flags_en
{
    no_flags=0,
    help_flag='h',
    verbose_flag='V',
    argframac_flag='a',
    negframac_flag='n',
    framacexe_flag='F',
    sourcelist_flag='s',
    cppdef_flag='D',
    cppundef_flag='U',
    cppincl_flag='I',
    version_flag=1000,
    listplugins_flag,
};

const struct option long_clever_options[] =
{
    /// --verbose to increase verbosity
    {
        .name= "verbose",
        .has_arg= no_argument,
        .flag= nullptr,
        .val= verbose_flag
    },
    /// --version to show the version of this clever-framac and of
    /// the Frama-C tool iteself
    {
        .name="version",
        .has_arg=no_argument,
        .flag=nullptr,
        .val=version_flag
    },
    /// --help to give help and also for Frama-C
    {
        .name="help",
        .has_arg=no_argument,
        .flag=nullptr,
        .val=help_flag
    },
    /// --framac=<Frama-C-executable>
    {
        .name="framac",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=framacexe_flag
    },
    {
        .name="argframac",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=argframac_flag
    },
    {
        .name="negframac",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=negframac_flag
    },
    {
        .name="sources",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=sourcelist_flag
    },
    {
        .name="cppdefine",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=cppdef_flag
    },
    {
        .name="cppundef",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=cppundef_flag
    },
    {
        .name="list-plugins",
        .has_arg=no_argument,
        .flag=nullptr,
        .val=listplugins_flag
    },
    {}
};

void
show_help(void)
{
    printf("%s usage:\n"
           "\t -V|--verbose           # output more messages\n"
           "\t --version              # give version information\n"
           "\t                        # also for Frama-C\n"
           "\t -h|--help              # give this help\n"
           "\t -F|--framac <framac>   # explicit Frama-C to be used\n"
           "\t                        # default is %s\n"
           "\t -a|--argframac <arg>   # argument to Frama-C\n"
           "\t -I <incldir>           # preprocessing include\n"
           "\t -D <definition>        # preprocessing definition\n"
           "\t -U <undefine>          # preprocessing undefine\n"
           "\t --list-plugins         # passed to Frama-C\n"
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
cfr_fatal_error_at(const char*fil, int lin)
{
    std::clog << "FATAL ERROR from " << fil << ":" << lin
              << " in " << progname << " git " << GIT_ID << std::endl;
    abort();
} // end cfr_fatal_error_at

void
parse_program_arguments(int argc, char**argv)
{
    int c=0;
    int option_index= -1;
    gethostname(myhost, sizeof(myhost)-1);
    do
        {
            option_index = -1;
            optarg = nullptr;
            c = getopt_long(argc, argv,
                            "VhF:a:l:U:D:I:",
                            long_clever_options, &option_index);
            if (c<0)
                break;
            switch (c)
                {
                case 'h': // --help
                    show_help();
                    std::cout << std::flush;
                    std::cerr << std::flush;
                    std::clog << std::flush;
                    fflush(nullptr);
                    std::cout << std::endl;
                    std::cout << framacexe << " help:::" << std::endl;
                    try_run_framac("-help");
                    try_run_framac("-kernel-h");
                    exit(EXIT_SUCCESS);
                    return;
                case 'V': // --verbose
                    is_verbose=true;
                    continue;
                case 'a': // --argframac=<option>
                    my_framac_options.push_back(optarg);
                    continue;
                case 'F': // --framac=<framac>
                    framacexe=optarg;
                    continue;
                case 's': // --sources=<listpath>
                    add_sources_list(optarg);
                    continue;
                case 'D': // -DPREPROVAR=<something> passed to preprocessor
                case 'U': // -UPREPROVAR passed to preprocessor
                case 'I': // -I<incldir> passed to preprocessor
                    my_prepro_options.push_back(optarg);
                    continue;
                case version_flag: // --version
                    printf("%s git %s built at %s\n", progname, GIT_ID, __DATE__);
                    try_run_framac("-version");
                    exit(EXIT_SUCCESS);
                    return;
                case listplugins_flag:
                    do_list_framac_plugins=true;
                    continue;
                }
            while (c>=0);
            /* Handle any remaining command line arguments (not options). */
            if (optind < argc)
                {
                    if (verbose_flag)
                        printf("%s: processing %d arguments\n",
                               progname, argc-optind);
                    while (optind < argc)
                        {
                            const char*curarg = argv[optind];
                            const char*resolvedpath = NULL;
                            enum source_type styp = srcty_NONE;
                            if (curarg[0] == '-')
                                {
                                    CFR_FATAL("bad file argument #" << optind
                                              << ": " << curarg
                                              << std::endl
                                              << "... consider giving its absolute path");
                                }
                            if (access(curarg, R_OK))
                                {
                                    int e=errno;
                                    CFR_FATAL("cannot access file argument #" << optind << ": " << curarg
                                              << " . " << strerror(e));
                                };
                            int curlen = strlen(curarg);
                            if (curlen < 4)
                                {
                                    CFR_FATAL("too short file argument #"
                                              << optind << ": " << curarg);
                                };
                            if (curarg[curlen-2] == '.' && curarg[curlen-1] == 'c')
                                styp = srcty_c;
                            else if (curarg[curlen-3] == '.'
                                     && curarg[curlen-2] == 'c' && curarg[curlen-1] == 'c')
                                styp = srcty_cpp;
                            else
                                {
                                    CFR_FATAL("unexpected file argument #"
                                              << optind << ": " << curarg
                                              << std::endl
                                              <<"It should end with .c or .cc");
                                }
                            resolvedpath = realpath(curarg, NULL);
                            // so resolvedpath has been malloced
                            if (strlen(resolvedpath) < 4)
                                {
                                    CFR_FATAL("too short file argument #"
                                              << optind << ": " << curarg
                                              << " resolved to " << resolvedpath);
                                };
                            for (const char*pc = resolvedpath; *pc; pc++)
                                if (isspace(*pc))
                                    {
                                        CFR_FATAL("file argument #"
                                                  << optind << ": " << curarg
                                                  << " resolved to unexpected " << resolvedpath);
                                    };
                            Source_file cursrcf(resolvedpath, styp);
                            my_srcfiles.push_back(cursrcf);
                            free ((void*)resolvedpath);
                            optind++;
                        }
                }
        }
    while(c>0);
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


void
try_run_framac(const char*arg1, const char*arg2,
               const char*arg3, const char*arg4,
               const char*arg5)
{
    const char*realframac=nullptr;
    assert(framacexe != nullptr);
    //// compute into realframac the malloc-ed string of Frama-C executable
    if (strchr(framacexe, '/'))
        {
            realframac= realpath(framacexe, nullptr);
        }
    else
        {
            // find $framacexe in our $PATH
            const char*envpath = getenv("PATH");
            const char*pc=nullptr;
            const char*colon=nullptr;
            int slenframac = strlen(framacexe);
            if (!envpath) /// very unlikely!
                envpath="/bin:/usr/bin:/usr/local/bin";
            for (pc=envpath; pc && *pc; pc=colon?colon+1:nullptr)
                {
                    char* dynpathbuf=nullptr;
                    // https://man7.org/linux/man-pages/man3/asprintf.3.html
                    int cplen = 0;
                    colon=strchr(pc, ':');
                    if (colon)
                        cplen = colon-pc-1;
                    else
                        cplen = strlen(pc);
                    int nbytes= asprintf(&dynpathbuf, "%.*s/%s",
                                         cplen, pc, framacexe);
                    if (nbytes<0 || dynpathbuf==nullptr)
                        {
                            int e = errno;
                            CFR_FATAL("asprintf failed in try_run_framac "
                                      << arg1 << ": " << strerror(e));
                        };
                    if (!access(dynpathbuf, X_OK))
                        {
                            realframac = dynpathbuf;
                            break;
                        };
                } // end for pc...
        }
    ////
    /// should fork and execve
    if (is_verbose)
        std::cout << progname << " is running " << realframac;
    if (arg1)
        {
            std::cout << ' ' << arg1;
            if (arg2)
                {
                    std::cout << ' ' << arg2;
                    if (arg3)
                        {
                            std::cout << ' ' << arg3;
                            if (arg4)
                                {
                                    std::cout << ' ' << arg4;
                                    if (arg5)
                                        {
                                            std::cout << ' ' << arg5;
                                        }
                                }
                        }
                }
        }
    std::cout << std::endl;
    std::cout << std::flush;
    std::cerr << std::flush;
    std::clog << std::flush;
    pid_t childpid = fork();
    if (childpid<0)
        {
            int e= errno;
            CFR_FATAL("try_run_framac " << arg1 << " failed to fork "
                      << strerror(e));
        };
    if (childpid==0)
        {
            // in child
            execl(realframac, arg1, arg2, arg3, arg4, arg5,
                  nullptr);
            /// should almost never happen
            abort();
            return;
        }
    /// in parent
    int wstatus= 0;
    bool okwait=false;
    do
        {
            pid_t wpid = waitpid(childpid, &wstatus, 0);
            int e = errno;
            if (wpid<0)
                {
                    if (e == EINTR)
                        {
                            usleep(50*1000);
                            continue;
                        };
                    CFR_FATAL("try_run_framac " << arg1
                              << " failed to waitpid process " << childpid
                              << " " << strerror(e));
                };
            // here waitpid succeeded
            if (wstatus == 0)
                {
                    free ((void*)realframac);
                    realframac=nullptr;
                    okwait= true;
                    return;
                };
            if (WIFEXITED(wstatus))
                {
                    int ex= WEXITSTATUS(wstatus);
                    if (ex>0)
                        CFR_FATAL("try_run_framac " << arg1
                                  << " running " << realframac
                                  << " process " << childpid
                                  << " exited " << WEXITSTATUS(ex));
                    okwait = true;
                }
            else if (WIFSIGNALED(wstatus))
                {
                    bool dumpedcore = WCOREDUMP(wstatus);
                    int sig = WTERMSIG(wstatus);
                    if (dumpedcore)
                        CFR_FATAL("try_run_framac " << arg1
                                  << " running " << realframac
                                  << " process " << childpid
                                  << " dumped core for signal#" << sig << "="
                                  << strsignal(sig));
                    else
                        CFR_FATAL("try_run_framac " << arg1
                                  << " running " << realframac
                                  << " process " << childpid
                                  << " terminated on signal#" << sig << "="
                                  << strsignal(sig));
                    okwait = true; // probably not reached
                }
        }
    while (okwait);
    free((void*)realframac);
    std::cout << std::endl << std::flush;
} // end try_run_framac

int
main(int argc, char*argv[])
{
    progname = argv[0];
    if (argc>1 && (!strcmp(argv[1], "--verbose") || !strcmp(argv[1], "-V")))
        is_verbose = true;
    parse_program_arguments(argc, argv);
    if (verbose_flag)
        printf("%s running verbosely on %s pid %d git %s with %d source files\n", progname,
               myhost, (int)getpid(), GIT_ID, (int) my_srcfiles.size());
    if (do_list_framac_plugins)
        try_run_framac("-plugins");
#warning should run framac on the collected source files....
} // end main

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./build-clever-framac.sh" ;;
 ** End: ;;
 ****************/
