// file misc-basile/clever-framac.cc
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Author(s):
 *      Basile Starynkevitch <basile@starynkevitch.net>
 *      or (before november 2023) <basile.starynkevitch@cea.fr>
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

// GNU guile Scheme interpreter. See
// https://www.gnu.org/software/guile/manual/html_node/Linking-Programs-With-Guile.html
#include "libguile.h"
#include "libguile/init.h"

/*** see getopt(3) man page mentionning
 *      extern char *optarg;
 *      extern int optind, opterr, optopt;
 ***/

char myhost[80];
const char*progname;
const char*framacexe = "/usr/bin/frama-c";
const char*realframac;
void compute_real_framac(void);
bool is_verbose;
bool guile_has_been_initialized;
char* sourcelist_path;
bool do_list_framac_plugins;
std::vector<std::string> my_prepro_options;
std::vector<std::string> my_framac_options;
std::vector<std::string> my_guile_files;
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
    const char*type_cname() const
    {
        switch (srcf_type)
            {
            case srcty_NONE:
                return "*none*";
            case srcty_c:
                return "C";
            case srcty_cpp:
                return "C++";
            default:
                CFR_FATAL("source " << path() << " has invalid type #"
                          << (int)srcf_type);
            }
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


SCM get_my_guile_environment_at(const char*csrc, int clineno);
#define GET_MY_GUILE_ENVIRONMENT() get_my_guile_environment_at(__FILE__,__LINE__)

/// add a GNU guile script file
void add_guile_script(const char*guilepath);

/// evaluate a GNU guile expression
void do_evaluate_guile(const char*guiletext);

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
    guilescript_flag='G',
    cppdef_flag='D',
    cppundef_flag='U',
    cppincl_flag='I',
    version_flag=1000,
    listplugins_flag,
    evalguile_flag,
    printinfo_flag,
};

const struct option long_clever_options[] =
{
    /// --verbose to increase verbosity
    {
        .name= "verbose",
        .has_arg= no_argument,
        .flag= nullptr,
        .val= verbose_flag // -V
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
        .val=help_flag // -h
    },
    /// --framac=<Frama-C-executable>
    {
        .name="framac",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=framacexe_flag // -F...
    },
    {
        .name="argframac",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=argframac_flag // -a...
    },
    {
        .name="negframac",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=negframac_flag // -n...
    },
    {
        .name="sources",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=sourcelist_flag // -s...
    },
    {
        .name="cppdefine",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=cppdef_flag // -D...
    },
    {
        .name="cppundef",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=cppundef_flag // -U...
    },
    {
        .name="list-plugins",
        .has_arg=no_argument,
        .flag=nullptr,
        .val=listplugins_flag
    },
    {
        .name="guile-script",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=guilescript_flag /// -G<script>
    },
    {
        .name="eval-guile",
        .has_arg=required_argument,
        .flag=nullptr,
        .val=evalguile_flag
    },
    {
        .name="print-info",
        .has_arg=no_argument,
        .flag=nullptr,
        .val=printinfo_flag
    },
    {}
};

void
show_help(void)
{
    printf("%s usage:\n"
           "\t -V|--verbose                 # output more messages\n"
           "\t --version                    # give version information\n"
           "\t                              # also for Frama-C\n"
           "\t -h|--help                    # give this help\n"
           "\t -F|--framac <framac>         # explicit Frama-C to be used\n"
           "\t                              # default is %s\n"
           "\t -G |--guile-script <script>  # Use GNU guile with this script\n"
           "\t --eval-guile=<guile-expr>    # evaluate with GNU guile\n"
           "\t -a|--argframac <arg>         # argument to Frama-C\n"
           "\t -I <incldir>                 # preprocessing include\n"
           "\t -D <definition>              # preprocessing definition\n"
           "\t -U <undefine>                # preprocessing undefine\n"
           "\t --list-plugins               # passed to Frama-C\n"
           "\t --print-info                 # print information\n"
           "\t -l | --sources <slist>       # read list of files (one per line) from <sfile>\n"
           "\t                              # if it starts with ! or | use popen\n"
           "\t                              # if it starts with @ it a list of files\n"
           "\t ... <source files>           # analyzed sources are C *.c ...\n"
           "\t                              # ... or C++ files *.cc\n"
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

extern "C" char**environ;
void
do_print_information(int argc, char**argv)
{
    std::ostringstream outs;
    for (int i=0; i<argc; i++)
        outs << ' ' << argv[i];
    outs<<std::flush;
    if (!realframac)
        compute_real_framac();
    std::cout << "Information about " << progname << " git " << GIT_ID
              << " pid " << (int)getpid() << " on host " << myhost
              << " with " << argc << " program arguments:"
              << std::endl;
    std::cout << outs.str() << std::endl;
    int envsiz=0;
    for (char**penv = environ; *penv; penv++) envsiz++;
    std::cout << progname << " with " << envsiz
              << " environment variables:" << std::endl;
    for (int i=0; i<envsiz; i++)
        std::cout << " " << environ[i] << std::endl;
    std::cout << progname << " using Frama-C on " << realframac << " with " << my_framac_options.size() << " options:" << std::endl;
    for (std::string curfropt : my_framac_options)
        std::cout << ' ' << curfropt;
    std::cout << std::endl;
    std::cout << progname << " with " << my_prepro_options.size() << " preprocessor options:" << std::endl;
#warning do_print_information should print a lot more...
} // end do_print_information

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
                            "VhF:a:l:U:D:I:G:",
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
                    compute_real_framac();
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
                case 'G': // --guile=<scheme-code>
                    add_guile_script(optarg);
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
                    printf("%s: git %s built at %s\n", progname, GIT_ID, __DATE__);
                    compute_real_framac();
                    printf("%s: running %s -version\n", progname, realframac);
                    try_run_framac("-version");
                    putc('\n', stdout);
                    exit(EXIT_SUCCESS);
                    return;
                case listplugins_flag:
                    do_list_framac_plugins=true;
                    continue;
                case evalguile_flag:
                    do_evaluate_guile(optarg);
                    continue;
                case printinfo_flag:
                    do_print_information(argc, argv);
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
add_guile_script(const char*scmpath)
{
    struct stat scmstat = {};
    assert(scmpath != nullptr);
    if (stat(scmpath, &scmstat) < 0)
        {
            fprintf(stderr, "%s: failed to stat Guile script file %s on %s (%s)\n",
                    progname, scmpath, myhost, strerror(errno));
            exit(EXIT_FAILURE);
        };
    if (scmstat.st_mode & S_IFMT != S_IFREG)
        {
            fprintf(stderr, "%s: Guile script %s is not a regular file on %s\n",
                    progname, scmpath, myhost);
            exit(EXIT_FAILURE);
        };
    const char* rp = realpath(scmpath, nullptr);
    std::string guilestr(rp);
    my_guile_files.push_back(guilestr);
    free ((void*)scmpath);
} // end add_guile_script


SCM
get_my_guile_environment_at (const char*cfile, int clineno)
{
    SCM guilenv = scm_interaction_environment ();
    if (guilenv == nullptr)
        CFR_FATAL("get_my_guile_environment_at failed to get GUILE interaction environment at " << cfile << ":" << clineno);
#warning unimplemented get_my_guile_environment_at should extend the environment
    CFR_FATAL("unimplemented get_my_guile_environment_at at " << cfile << ":" << clineno);
} // end get_my_guile_environment_at

void
do_evaluate_guile(const char*guiletext)
{
    SCM curguilenv = nullptr;
    if (!guile_has_been_initialized)
        {
            guile_has_been_initialized=true;
            scm_init_guile();
            curguilenv = GET_MY_GUILE_ENVIRONMENT();
        }
    /// https://lists.gnu.org/archive/html/guile-user/2023-02/msg00019.html
    SCM guilestr = scm_from_locale_string(guiletext);
    if (guilestr == nullptr)
        CFR_FATAL("do_evaluate_guile failed to get GUILE string from " << guiletext);
    SCM guilres = scm_eval_string_in_module (guilestr, curguilenv);
    CFR_FATAL("incomplete do_evaluate_guile " << guiletext);
#warning incomplete do_evaluate_guile (should use guilres)
} // end do_evaluate_guile

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
            fprintf(stderr, "%s: (near %s:%d)  failed to allocate line bufferof %d bytes (%s)\n",
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
compute_real_framac(void)
{
    if (realframac) return;
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
                            CFR_FATAL("asprintf failed in compute_real_framac: "
                                      << strerror(e));
                        };
                    if (!access(dynpathbuf, X_OK))
                        {
                            realframac = dynpathbuf;
                            break;
                        };
                } // end for pc...
        }
} // end compute_real_framac
void
try_run_framac(const char*arg1, const char*arg2,
               const char*arg3, const char*arg4,
               const char*arg5)
{
    assert(framacexe != nullptr);
    if (!realframac)
        compute_real_framac();
    ////
    /// should fork and execve
    if (is_verbose)
        {
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
        };
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
            execl(realframac, realframac, arg1, arg2, arg3, arg4, arg5,
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
    if (is_verbose)
        std::cout << std::endl << progname << " did run " << realframac << std::endl;
    free((void*)realframac);
    std::cout << std::endl << std::flush;
} // end try_run_framac


void inner_guile(void*guileclosure, int argc, char**argv)
{
    CFR_FATAL("unimplemented inner_guile");
#warning unimplemented inner_guile
    // see https://www.gnu.org/software/guile/manual/html_node/A-Sample-Guile-Main-Program.html
}
void
do_use_guile(int argc, char*argv[], int nbguile)
{
#warning unimplemented do_use_guile
    CFR_FATAL("unimplemented do_use_guile for " << nbguile << " Scheme scripts");
    /// perhaps scm_boot_guile is wrong...
    scm_boot_guile (argc, argv, inner_guile, 0);
} // end do_use_guile

int
main(int argc, char*argv[])
{
    progname = argv[0];
    if (argc>1 && (!strcmp(argv[1], "--verbose") || !strcmp(argv[1], "-V")))
        is_verbose = true;
    ///
    parse_program_arguments(argc, argv);
    int nbguile = (int)my_guile_files.size();
    if (is_verbose)
        printf("%s running verbosely on %s pid %d git %s with %d source files\n", progname,
               myhost, (int)getpid(), GIT_ID, (int) my_srcfiles.size());
    if (nbguile>0)
        do_use_guile(argc, argv, nbguile);
    if (do_list_framac_plugins)
        try_run_framac("-plugins");
    if (!realframac)
        compute_real_framac();
    if (is_verbose)
        {
            int nbsrc = (int) my_srcfiles.size();
            printf("%s running verbosely on %s pid %d git %s\n"
                   "... with the following %d source files:\n",
                   progname, myhost, (int)getpid(), GIT_ID, nbsrc);
            for (int sx=0; sx<nbsrc; sx++)
                {
                    printf(" %s [%s]\n", my_srcfiles[sx].path().c_str(),
                           my_srcfiles[sx].type_cname());
                }
            int nbprepro = (int) my_prepro_options.size();
            printf("With %d preprocessor options:\n", nbprepro);
            for (int px=0; px<nbprepro; px++)
                printf(" %s\n", my_prepro_options[px].c_str());
            int nbargs = (int) my_framac_options.size();
            printf("Specified %d options to %s:\n", nbargs, realframac);
            for (int ax=0; ax<nbargs; ax++)
                {
                    printf(" %s\n", my_framac_options[ax].c_str());
                }
            printf("Specified %d Guile scripts to %s:\n", nbguile, realframac);
            for (int gx=0; gx<nbguile; gx++)
                {
                    printf(" %s\n", my_guile_files[gx].c_str());
                }
        }
    std::vector<std::string> framaexecargs;
    framaexecargs.push_back(std::string{realframac});
    int nbargs = (int) my_framac_options.size();
    for (int aix=0; aix<nbargs; aix++)
        {
            framaexecargs.push_back(my_framac_options[aix]);
        };
    int nbprepro = (int) my_prepro_options.size();
    if (nbprepro>0)
        {
            const char*cppenv = getenv("CPP");
            const char*mycpp = cppenv?cppenv:"/usr/bin/cpp";
            std::string cppcmd=mycpp;
            for (int ipx = 0; ipx < my_prepro_options.size(); ipx++)
                {
                    cppcmd += ' ';
                    cppcmd += my_prepro_options[ipx];
                }
            cppcmd += "%1 -o %2";
            framaexecargs.push_back("-cpp-command");
            framaexecargs.push_back(cppcmd);
        };
    if (nbguile>0)
        {
            if (!guile_has_been_initialized)
                {
                    guile_has_been_initialized=true;
                    scm_init_guile ();
                };
#warning unimplemented use Guile here
            CFR_FATAL("not yet implemented Guile scripts handling of "
                      << nbguile << " Scheme scripts");
        }
    int nbsrc =  my_srcfiles.size();
    for (int six = 0; six < nbsrc; six++)
        {
            framaexecargs.push_back(my_srcfiles[six].path());
        };
    int cmdlen = framaexecargs.size();
    char**framargv = //
        (char**)calloc(cmdlen+2, sizeof(char*));
    if (is_verbose)
        {
            printf ("%s will run command with %d arguments:",
                    progname, cmdlen);
            for (int cix = 0; cix < cmdlen; cix++)
                printf(" %s", framaexecargs[cix].c_str());
            putc('\n', stdout);
            fflush(nullptr);
            if (!framargv)
                CFR_FATAL("failed to calloc " << (cmdlen+2) << " pointers:"
                          << strerror(errno));
            for (int i=0; i<cmdlen; i++)
                framargv[i] = (char*) (framaexecargs[i].c_str());

        };
    fflush(nullptr);
    execvp(realframac, framargv);
    // should not be reached, but if it is....
    fprintf(stderr, "%s: execvp %s with %d arguments failed (%s)\n",
            progname, realframac, cmdlen, strerror(errno));
    fflush(nullptr);
    exit(EXIT_FAILURE);
} // end main

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./build-clever-framac.sh" ;;
 ** End: ;;
 ****************/
