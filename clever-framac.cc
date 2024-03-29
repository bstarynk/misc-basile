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
#include <atomic>
#include <memory>

#include <stdarg.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <getopt.h>

// GNU guile Scheme interpreter. See
// https://www.gnu.org/software/guile/manual/html_node/Linking-Programs-With-Guile.html
#include "libguile.h"

#ifndef FULL_CLEVER_FRAMAC_SOURCE
#error compilation command should define FULL_CLEVER_FRAMAC_SOURCE
#endif //FULL_CLEVER_FRAMAC_SOURCE

extern "C" const char my_full_clever_framac_source[];
const char my_full_clever_framac_source[] = FULL_CLEVER_FRAMAC_SOURCE;

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
bool guile_did_run_framac;
char* sourcelist_path;
bool do_list_framac_plugins;
typedef std::vector<std::string> my_vector_of_strings_t;
my_vector_of_strings_t my_prepro_options;
my_vector_of_strings_t my_framac_options;
my_vector_of_strings_t  my_guile_files;
extern "C" std::atomic<my_vector_of_strings_t*> my_framargvec_ptr;

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

///// declaration of our Guile primitives, conventionally named
///// myscm_<cname>

extern "C" {
  //°Guile (get_nb_prepro_options)
  SCM myscm_get_nb_prepro_options(void);

  //°Guile (get_nth_prepro_option <int>)
  SCM myscm_get_nth_prepro_option(SCM nguile);

  //°Guile (add_prepro_option <string>)
  SCM myscm_add_prepro_option(SCM arg);

  //°Guile (get_real_frama_c_path)
  SCM myscm_get_real_frama_c_path(void);

  //°Guile (get_prepro_argvec)
  SCM myscm_get_prepro_argvec(void);

  //°Guile (reset_frama_c_argvec)
  SCM myscm_reset_frama_c_argvec(void);

// Guile primitive to query the current preprocessor arguments as a
// Guile vector
  //°Guile (get_prepro_argvec)
  SCM myscm_get_prepro_argvec(void);
// Guile primitive to query the current Frama-C arguments as a Guile vector
  //°Guile (get_frama_c_argvec)
  SCM myscm_get_frama_c_argvec(void);

  /// variadic Guile primitive
  SCM myscm_run_frama_c(SCM first, ...);
};

enum source_type
{
  srcty_NONE,
  srcty_c,
  srcty_cpp
};				// end source_type

SCM myscm_symb_c;
SCM myscm_symb_cpp;
SCM myscm_symb_core;
#define CLEVERFRAMAC_LASTSIG 64
SCM myscm_symb_signal[CLEVERFRAMAC_LASTSIG];


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
  SCM guile_type() const
  {
    switch (srcf_type)
      {
      case srcty_c:
        return myscm_symb_c;
      case srcty_cpp:
        return myscm_symb_cpp;
      default:
        return scm_from_bool(false);
      };
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
         "\t                              # if it starts with @ it is a list of files\n"
         "\t ... <source files>           # analyzed sources are C *.c ...\n"
         "\t                              # ... or C++ files *.cc\n"
         " (preprocessing options are passed to Frama-C)\n"
         "\n See https://frama-c.com/ for details on Frama-C ...\n"
         "Our gitid is %s (file %s compiled %s at %s)\n"
         "\n",
         progname, framacexe, GIT_ID, __FILE__, __DATE__, __TIME__);
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
  for (std::string curprepopt : my_prepro_options)
    std::cout << ' ' << curprepopt << std::endl;
  int nbguile = (int)my_guile_files.size();
  if (nbguile>0)
    {
      std::cout << progname << " with " << nbguile << " guile script files:" << std::endl;
      for (std::string curguilfil : my_guile_files)
        std::cout << " " << curguilfil << std::endl;
    }
  else
    std::cout << progname << " without any guile script." << std::endl;
  int nbsrc = (int) my_srcfiles.size();
  if (nbsrc > 0)
    {
      int nbc = 0;
      int nbcpp = 0;
      for (Source_file& cursrc : my_srcfiles)
        {
          if (cursrc.type() == srcty_c) nbc++;
          else if (cursrc.type() == srcty_cpp) nbcpp++;
          else CFR_FATAL("source file " << cursrc.path() << " has invalid type.");
        }
      std::cout << progname << " with " << nbc << " C files and " << nbcpp << " C++ files to analyze:" << std::endl;
      for (Source_file& cursrc : my_srcfiles)
        std::cout << ' ' << cursrc.path() << "\t" << cursrc.type_cname() << std::endl;
    }
  else
    std::cout << progname << " without analyzed sources." << std::endl;
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



// Guile primitive to query the number of preprocessor options
SCM
myscm_get_nb_prepro_options(void)
{
  return scm_from_signed_integer(my_prepro_options.size());
} // end myscm_get_nb_prepro_options

// Guile primitive to query the Nth preprocessor options or else #f
// in Guile (get_nth_prepro_option <number>)
SCM
myscm_get_nth_prepro_option(SCM nguile)
{
  if (scm_is_integer (nguile))
    {
      int n = scm_to_int(nguile);
      int nbprepro = (int)  my_prepro_options.size();
      if (n<0)
        n += nbprepro;
      if (n >= 0 && n<nbprepro)
        return scm_from_utf8_string(my_prepro_options[n].c_str());
    }
  return scm_from_bool(false);
} // end myscm_get_nth_prepro_option

SCM
myscm_add_prepro_option(SCM arg)
{
  if (scm_is_string (arg))
    {
      char*s = scm_to_utf8_string(arg);
      std::string str(s);
      my_prepro_options.push_back(str);
      return arg;
    }
  else
    return scm_from_bool(false);
} // end myscm_add_prepro_option

// Guile primitive to query the real Frama-C path as a string
SCM
myscm_get_real_frama_c_path(void)
{
  if (!realframac)
    compute_real_framac();
  return scm_from_utf8_string(realframac);
} // end myscm_get_real_framac_path


SCM
myscm_reset_frama_c_argvec(void)
{
  my_framac_options.clear();
  my_framac_options.push_back(realframac);
  return myscm_get_frama_c_argvec();
} // end myscm_reset_frama_c_argvec

// Guile primitive to query the current Frama-C arguments as a Guile vector
SCM
myscm_get_frama_c_argvec(void)
{
  SCM resvec = nullptr;
  if (!realframac)
    compute_real_framac();
  if (my_framac_options.empty())
    my_framac_options.push_back(realframac);
  int siz = my_framac_options.size();
  resvec = scm_c_make_vector(siz, SCM_UNSPECIFIED);
  for (int i=0; i<siz; i++)
    {
      scm_c_vector_set_x(resvec, i, scm_from_utf8_string(my_framac_options[i].c_str()));
    }
  return resvec;
} // end myscm_get_frama_c_argvec

// Guile primitive to query the current preprocessor arguments as a Guile vector
SCM
myscm_get_prepro_argvec(void)
{
  SCM resvec = nullptr;
  int siz = my_prepro_options.size();
  resvec = scm_c_make_vector(siz, SCM_UNSPECIFIED);
  for (int i=0; i<siz; i++)
    {
      scm_c_vector_set_x(resvec, i, scm_from_utf8_string(my_prepro_options[i].c_str()));
    }
  return resvec;
} // end myscm_get_prepro_argvec


static void add_frama_c_guile_arg(std::vector<std::string>& argv, int depth, SCM val);
//std::atomic<std::shared_ptr<std::vector<std::string>>> my_framargvec_ptr;
std::atomic<my_vector_of_strings_t*> my_framargvec_ptr;
/// variadic Guile primitive
/// Guile arguments have been evaluated by called.
SCM myscm_run_frama_c(SCM first, ...)
{
  va_list args;
  int nbargs= 0;
  SCM elt = nullptr;
  SCM result = nullptr;
  std::vector<std::string> framargvec;
  my_vector_of_strings_t* oldptr = my_framargvec_ptr.exchange(&framargvec);
#warning should use my_framargvec_ptr here
  compute_real_framac();
  framargvec.push_back(realframac);
  if (first != SCM_UNDEFINED && !SCM_UNBNDP(first))
    {
      va_start (args, first);
      while ((elt = va_arg(args, SCM)) != nullptr //
             && !SCM_UNBNDP(elt))
        nbargs++;
      va_end (args);
      add_frama_c_guile_arg(framargvec, 0, first);
      va_start (args, first);
      while ((elt = va_arg(args, SCM)) != nullptr //
             && !SCM_UNBNDP(elt))
        add_frama_c_guile_arg(framargvec, 0, elt);
      va_end (args);
    }
  const char**frargv = (const char**) (calloc (framargvec.size() + 1, sizeof(char*)));
  if (!frargv)
    CFR_FATAL("myscm_run_frama_c failed to calloc " << (framargvec.size()+1) << " words: " << strerror(errno));
  int frcntarg = 0;
  for (std::string curargstr : framargvec)
    frargv[frcntarg++] = curargstr.c_str();
  std::cout << std::flush;
  std::cerr << std::flush;
  std::clog << std::flush;
  fflush(nullptr);
#pragma message "the fork of Frama-C is here"
  pid_t pid = fork();
  if (pid<0)
    CFR_FATAL("myscm_run_frama_c failed to fork " << strerror(errno));
  else if (pid==0)   /// child process should run Frama-C
    {
      (void) nice (1);
      // According to Julien Seignole, Frama-C never reads its stdin..., so
      close(STDIN_FILENO);
      int nfd = open("/dev/null", O_RDONLY);
      if (nfd>0 && nfd!= STDIN_FILENO)
        {
          dup2(nfd, STDIN_FILENO);
          close(nfd);
        }
      execv(realframac, (char**)frargv);
      // Should in practice never be reached, but just in case...
      perror(realframac);
      abort();
      exit(EXIT_FAILURE);
    }
  else // pid>0, parent process
    {
      int ws = 0;
      usleep (10*1024);	// pause for ten milliseconds to let Frama-C start
      while (ws=0, errno=0, ((-1==waitpid(pid, &ws, 0)) && errno == EINTR))
        usleep(1024);
      if (ws==0)
        result = scm_from_bool(true);
      else if (WIFEXITED(ws) && WEXITSTATUS(ws) == EXIT_FAILURE)
        result = scm_from_bool(false);
      else if (WIFEXITED(ws) && WEXITSTATUS(ws) > 1)
        result = scm_from_int(WEXITSTATUS(ws));
      else if (WIFSIGNALED(ws))
        {
          bool dumpedcore = WCOREDUMP(ws);
          int nsig = WTERMSIG(ws);
          if (nsig>0 && nsig<CLEVERFRAMAC_LASTSIG && myscm_symb_signal[nsig])
            {
              if (dumpedcore)
                result = scm_cons(myscm_symb_signal[nsig], myscm_symb_core);
              else
                result = myscm_symb_signal[nsig];
            }
          else
            {
              if (dumpedcore)
                result = scm_cons(scm_from_int(nsig), myscm_symb_core);
              else
                result = scm_from_int(nsig);
            }
        }
    };
  free (frargv);
  my_framargvec_ptr.store(oldptr);
  return result;
} // end myscm_run_frama_c

#define MAX_CALL_DEPTH 256

void add_frama_c_guile_arg(std::vector<std::string>& argv, int depth, SCM val)
{
  const int maxargsize = 2048;
  long l = -2;
  SCM curelem = nullptr;
  if (depth > MAX_CALL_DEPTH)
    CFR_FATAL("add_frama_c_guile_arg too deep at " << depth);
  if (SCM_NULL_OR_NIL_P(val))
    return;
  if (argv.size() > maxargsize)
    CFR_FATAL("add_frama_c_guile_arg too many arguments " << argv.size()
              << " at depth of " << depth);
  if (scm_is_integer(val))
    {
      char numbuf[16];
      memset(numbuf, 0, sizeof(numbuf));
      snprintf(numbuf, sizeof(numbuf), "%ld",
               (long) scm_to_int(val));
      argv.push_back(std::string{numbuf});
    }
  else if (scm_is_bool_or_nil(val))
    return;
  else if (scm_is_symbol(val))
    {
      const char*symname =
        scm_to_utf8_string(scm_symbol_to_string(val));
      argv.push_back(std::string{symname});
    }
  else if (scm_is_string(val))
    {
      argv.push_back(std::string{scm_to_utf8_string(val)});
    }
  else if ((l = scm_ilength(val))>=0)
    {
      // val is a proper list
      SCM curpair = val;
      curelem = nullptr;
      while (scm_is_pair(curpair))
        {
          curelem = SCM_CAR(curpair);
          if (!SCM_NULL_OR_NIL_P(curelem))
            add_frama_c_guile_arg(argv, depth+1, curelem);
          curelem = nullptr;
          curpair = SCM_CDR(curpair);
        }
    }
  else if (scm_is_vector(val))
    {
      l = scm_c_vector_length(val);
      curelem = nullptr;
      for (long i=0; i<l; i++)
        {
          curelem= scm_c_vector_ref(val, i);
          if (!SCM_NULL_OR_NIL_P(curelem))
            add_frama_c_guile_arg(argv, depth+1, curelem);
          curelem = nullptr;
        }
    }
  else if (scm_is_true (scm_procedure_p (val)))
    {
      curelem = scm_call_0(val);
      if (!SCM_NULL_OR_NIL_P(curelem))
        add_frama_c_guile_arg(argv, depth+1, curelem);
      curelem = nullptr;
    }
  else if (scm_is_array(val))
    {
      int rk = scm_c_array_rank(val);
      if (rk != 1)
        CFR_FATAL("add_frama_c_guile_arg at depth " << depth
                  << " cannot handle multi-dimensional array of " << rk);
      long len = scm_c_array_length(val);
      for (long i=0; i<len; i++)
        {
          curelem = scm_c_array_ref_1(val, i);
          if (!SCM_NULL_OR_NIL_P(curelem))
            add_frama_c_guile_arg(argv, depth+1, curelem);
          curelem = nullptr;
        }
    }
} // end add_frama_c_guile_arg


SCM
get_my_guile_environment_at (const char*cfile, int clineno)
{
  /// called from the GET_MY_GUILE_ENVIRONMENT macro....
  SCM guilenv = scm_interaction_environment ();
  if (guilenv == nullptr)
    CFR_FATAL("get_my_guile_environment_at failed to get GUILE interaction environment at " << cfile << ":" << clineno);
  scm_c_define("self_cpp_source",
               scm_from_utf8_string(my_full_clever_framac_source));
  scm_c_define("git_id", scm_from_utf8_string(GIT_ID));
  scm_c_define("c_origin_file", scm_from_utf8_string(cfile));
  scm_c_define("c_origin_line", scm_from_int(clineno));
  myscm_symb_c =  scm_from_utf8_symbol ("c");
  myscm_symb_cpp = scm_from_utf8_symbol("c++");
  myscm_symb_core = scm_from_utf8_symbol("core");
  for (int nsig = 1; nsig < CLEVERFRAMAC_LASTSIG; nsig++)
    {
      const char* absig = sigdescr_np(nsig);
      if (!absig) continue;
      myscm_symb_signal[nsig] = scm_from_utf8_symbol(absig);
    };
  scm_c_define_gsubr("get_nb_prepro_options",
                     /*required#*/0, /*optional#*/0, /*variadic?*/0,
                     (scm_t_subr)myscm_get_nb_prepro_options);
  scm_c_define_gsubr("get_nth_prepro_option",
                     /*required#*/1, /*optional#*/0, /*variadic?*/0,
                     (scm_t_subr)myscm_get_nth_prepro_option);
  scm_c_define_gsubr("add_prepro_option",
                     /*required#*/1, /*optional#*/0, /*variadic?*/0,
                     (scm_t_subr)myscm_add_prepro_option);
  scm_c_define_gsubr("get_prepro_argvec",
                     /*required#*/0, /*optional#*/0, /*variadic?*/0,
                     (scm_t_subr)myscm_get_prepro_argvec);
  scm_c_define_gsubr("get_real_frama_c_path",
                     /*required#*/0, /*optional#*/0, /*variadic?*/0,
                     (scm_t_subr)myscm_get_real_frama_c_path);
  scm_c_define_gsubr("run_frama_c",
                     /*required#*/1, /*optional#*/0, /*variadic?*/1,
                     (scm_t_subr)myscm_run_frama_c);
  scm_c_define_gsubr("get_frama_c_argvec",
                     /*required#*/0, /*optional#*/0, /*variadic?*/0,
                     (scm_t_subr)myscm_get_frama_c_argvec);
  scm_c_define_gsubr("reset_frama_c_argvec",
                     /*required#*/0, /*optional#*/0, /*variadic?*/0,
                     (scm_t_subr)myscm_reset_frama_c_argvec);
#warning unimplemented get_my_guile_environment_at should extend the environment
  std::clog << "incomplete GET_MY_GUILE_ENVIRONMENT from " << cfile << ":" << clineno << std::endl;
  return guilenv;
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
  fflush(nullptr);
  printf("Guile evaluation of expression %s gives:", guiletext);
  fflush(stdout);
  scm_display(guilres, scm_current_output_port());
  fputc('\n', stdout);
  fflush(nullptr);
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


int
main(int argc, char*argv[])
{
  progname = argv[0];
  if (argc>1 && (!strcmp(argv[1], "--verbose") || !strcmp(argv[1], "-V")))
    is_verbose = true;
  ///
  parse_program_arguments(argc, argv);
  if (access(my_full_clever_framac_source, R_OK))
    CFR_FATAL("missing source file for " << __FILE__
              << " in " << my_full_clever_framac_source
              << ":" << strerror(errno));
  int nbguile = (int)my_guile_files.size();
  if (is_verbose)
    printf("%s running verbosely on %s pid %d git %s with %d source files\n", progname,
           myhost, (int)getpid(), GIT_ID, (int) my_srcfiles.size());
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
      SCM curguilenv = nullptr;
      if (!guile_has_been_initialized)
        {
          guile_has_been_initialized=true;
          scm_init_guile ();
          curguilenv = GET_MY_GUILE_ENVIRONMENT();
        };
      for (int gix=0; gix<nbguile; gix++)
        {
          std::string curguilefilepath = my_guile_files[gix];
          if (access(curguilefilepath.c_str(), R_OK))
            CFR_FATAL("cannot access Guile file#" << gix << ": " << curguilefilepath << " - " << strerror(errno));
          fflush(nullptr);
          SCM guilecurload = scm_c_primitive_load(curguilefilepath.c_str());
          fflush(nullptr);
          printf("Guile loading of script#%d: %s gave ", gix, curguilefilepath.c_str());
          scm_display(guilecurload, scm_current_output_port());
          fflush(nullptr);
          fputc('\n', stdout);
          fflush(stdout);
        }
    }
  if (guile_did_run_framac)
    {
      if (is_verbose)
        printf ("%s has run %s thru Guile scripts or expressions.\n", progname, realframac);
      exit(EXIT_SUCCESS);
    };

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
