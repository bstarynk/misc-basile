// SPDX-License-Identifier: GPL-3.0-or-later
/* file logged-gcc.cc from https://github.com/bstarynk/misc-basile/

   a wrapper with logging of GCC compilation (for g++ or gcc on Linux)

   © Copyright Basile Starynkevitch 2020
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

#include <unistd.h>
#include <syslog.h>
#include <sysexits.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include <sqlite3.h>

#ifndef GCC_EXEC
#define GCC_EXEC "/usr/bin/gcc-10"
#endif

#ifndef GXX_EXEC
#define GXX_EXEC "/usr/bin/g++-10"
#endif

const char* mygcc;
const char* mygxx;
const char* mysqlitepath;
const char* mysqliterequest;
sqlite3* mysqlitedb;

double get_float_time(clockid_t cid)
{
  struct timespec ts= {0,0};
  if (!clock_gettime(cid, &ts))
    return ts.tv_sec*1.0 + ts.tv_nsec*1.0e-9;
  else
    return NAN;
}

std::vector<const char*>
parse_logged_program_options(int &argc, char**argv,
                             std::string& cmdstr,
                             int& nbgccargs)
{
  std::vector<const char*> argvec;
  cmdstr.reserve(1+((argc*20+100)|0xff));
  argvec.reserve(argc+10);
  argvec.push_back(argv[0]);
  nbgccargs = 0;
  for (int ix=0; ix<argc; ix++)
    {
      if (ix>0) cmdstr += ' ';
      cmdstr += argv[ix];
    };
  ///
  for (int ix=1; ix<argc; ix++)
    {
      if (!strncmp(argv[ix],"--gcc=", strlen ("--gcc=")))
        {
          mygcc=argv[ix]+strlen("--gcc=");
          continue;
        }
      else if (!strncmp(argv[ix],"--gxx=", strlen ("--gxx=")))
        {
          mygxx=argv[ix]+strlen("--gxx=");
          continue;
        }
      else if (!strncmp(argv[ix],"--sqlite=", strlen ("--sqlite=")))
        {
          mysqlitepath=argv[ix]+strlen("--sqlite=");
          continue;
        }
      else if (!strncmp(argv[ix],"--dosql=", strlen ("--dosql=")))
        {
          if (mysqliterequest != nullptr)
            {
              std::clog << argv[0] << " can get only one --dosql=<request> but got " << mysqliterequest
                        << " and " << argv[ix] << std::endl;
              exit (EXIT_FAILURE);
            };
          mysqliterequest=argv[ix]+strlen("--dosql=");
          continue;
        }
      else if (!strcmp(argv[ix], "--help"))
        {
          std::clog << argv[0]
                    << " is a logging wrapper to gcc or g++ compilers for Linux." << std::endl
                    << "  (see http://gcc.gnu.org/ and file " __FILE__ " on https://github.com/bstarynk/misc-basile/ for more)" << std::endl;
          std::clog << "A GPLv3+ licensed free software, see https://www.gnu.org/licenses/ for more" << std::endl;
          std::clog << " by Basile Starynkevitch (see http://starynkevitch.net/Basile/ email <basile@starynkevitch.net>), France" << std::endl;
          std::clog << " Acceptable options include:" << std::endl
                    << " --gcc=<some-executable> #e.g. --gcc=/usr/bin/gcc-9, overridding $LOGGED_GCC" << std::endl
                    << " --g++=<some-executable> #e.g. --g++=/usr/bin/g++-9, overridding $LOGGED_GXX" << std::endl
                    << " --sqlite=<some-sqlite-file> #e.g. --sqlite=$HOME/tmp/loggedgcc.sqlite, overridding $LOGGED_SQLITE" << std::endl
                    << " --dosql=<some-sqlite-request> #e.g. --dosql='SELECT * FROM tb_sourcepath' for advanced users." << std::endl
                    <<  "followed by program options passed to the GCC compiler..." << std::endl;
          std::clog << " Relevant environment variables are $LOGGED_GCC and $LOGGED_GXX for the compilers (when --gcc=... or --g++=... is not given)." << std::endl
                    << " When provided, the $LOGGED_CFLAGS may contain space-separated initial program options passed just after the C compiler $LOGGED_GCC." << std::endl
                    << " When provided, the $LOGGED_CXXFLAGS may contain space-separated initial program options passed just after the C++ compiler $LOGGED_GXX." << std::endl
                    << " When provided, the $LOGGED_LINKFLAGS may contain space-separated final program options passed just after the C or C++ compiler above." << std::endl;
          std::clog << " When provided, the $LOGGED_SQLITE should give some initialized Sqlite database, with a previous run " << argv[0] << " --sqlite=<sqlite-database-file>" << std::endl;
          continue;
        }
      else if (!strcmp(argv[ix], "--version")
               && strstr(argv[0], "logged"))
        {
          std::clog << argv[0] << " version " << GITID
                    <<  " of " << __FILE__ << " compiled on "
                    << __DATE__ "@" __TIME__ << std::endl
                    << "See https://github.com/bstarynk/misc-basile/ for more"
                    << std::endl
                    << " (a logging wrapper to GCC compiler on Linux)" << std::endl
                    << " so pass --help to get some help and usage." << std::endl
                    << " by Basile Starynkevitch (see http://starynkevitch.net/Basile/ email <basile@starynkevitch.net>), France" << std::endl;
          syslog(LOG_INFO, "version %s of %s from https://github.com/bstarynk/misc-basile/ compiled on %s@%s running %s",
                 GITID, argv[0], __DATE__, __TIME__, cmdstr.c_str());
          exit(EXIT_SUCCESS);
        }
      else
        {
          nbgccargs++;
          argvec.push_back(argv[ix]);
        }
    }
  return argvec;
} // end parse_logged_program_options


/// return a serial in tb_sourcepath or 0 on failure
std::int64_t
register_sqlite_source_data(const char*realpath, const char*md5, long mtime, long size)
{
  char*msgerr = nullptr;
  std::int64_t serialid= 0;
  char *sqlreq= nullptr;
  sqlite3_str* str = sqlite3_str_new(mysqlitedb);
  sqlite3_str_appendf(str, "BEGIN TRANSACTION;");
  sqlite3_str_appendf(str, "INSERT OR IGNORE INTO tb_sourcepath(srcp_realpath) VALUES(%q);", realpath);
  sqlreq = sqlite3_str_value(str);
  int r1 = sqlite3_exec(mysqlitedb, sqlreq, nullptr, nullptr, &msgerr);
  if (r1 != SQLITE_OK)
    {
      syslog(LOG_ALERT, "register_sqlite_source_data (l%d) %s failure #%d: %s", __LINE__,
             sqlreq, r1, msgerr?msgerr:"???");
      return 0;
    };
  serialid = sqlite3_last_insert_rowid(mysqlitedb);
  sqlite3_str_reset(str);
  sqlreq = nullptr;
  sqlite3_str_appendf(str, "INSERT OR REPLACE INTO tb_sourcedata(srcd_path_serial, srcd_path_mtime, srcd_path_md5, srcd_path_size)"
                      " VALUES (%lld, %ld, %q, %ld);\n"
                      "END TRANSACTION;",
                      serialid, mtime, md5, size);
  sqlreq = sqlite3_str_value(str);
  int r2 = sqlite3_exec(mysqlitedb, sqlreq, nullptr, nullptr, &msgerr);
  if (r2 != SQLITE_OK)
    {
      syslog(LOG_ALERT, "register_sqlite_source_data (l%d)  %s failure #%d: %s", __LINE__,
             sqlreq, r2,  msgerr?msgerr:"???");
      return 0;
    };
  char*fbuf = sqlite3_str_finish(str);
  sqlite3_free(fbuf);
  return serialid;
} // end register_sqlite_source_data

void
register_sqlite_compilation (std::int64_t firstserial, const char*firstmd5, const char*progstr,
			     time_t startime, double elapsedtime,
			     double usertime, double systime, long maxrss, long pageflt)
{
  assert(mysqlitedb);
  assert(firstserial>0);
  assert(firstmd5!=nullptr);
  assert(progstr!=nullptr);
  assert(startime>0);
  char *sqlreq= nullptr;
  char*msgerr=nullptr;
  sqlite3_str* str = sqlite3_str_new(mysqlitedb);
  sqlite3_str_appendf(str, "BEGIN TRANSACTION;");
  sqlite3_str_appendf(str, "INSERT INTO tb_successful_compilation\n"
		      " (compil_firstsrc_id, compil_firstsrc_md5, compil_command,\n"
		      "  compil_start_time, compil_elapsed_time, compil_usercpu_time, compil_syscpu_time,\n"
		      "  compil_page_faults, compil_max_rss)\n"
		      " VALUES(%lld, %ld, %q, %q, %ld, %f, %f, %f, %ld, %d);\n",
		      firstserial, firstmd5, progstr, startime, elapsedtime, usertime, systime, pageflt, maxrss);
  sqlite3_str_appendf(str, "END TRANSACTION;\n");
  sqlreq = sqlite3_str_value(str);
  int r1 = sqlite3_exec(mysqlitedb, sqlreq, nullptr, nullptr, &msgerr);
  if (r1 != SQLITE_OK)
    {
      syslog(LOG_ALERT, "register_sqlite_compilation (l%d) %s failure #%d: %s", __LINE__,
             sqlreq, r1, msgerr?msgerr:"???");
    };
} // end register_sqlite_compilation

/// return a serial in sqlite database, or else 0, or -1 on failure
std::int64_t
show_md5_mtime(const char*path, time_t mtime, char*firstmd5)
{
  assert (path != nullptr && path[0] != (char)0);
  assert (mtime != 0);
  FILE* fil = fopen(path, "rm");
  if (!fil)
    {
      syslog(LOG_WARNING, "show_md5_mtime cannot open %s - %m", path);
      return -1;
    };
  MD5_CTX ctx;
  memset (&ctx, 0, sizeof(ctx));
  if (!MD5_Init(&ctx))
    {
      syslog(LOG_WARNING, "show_md5_mtime failed to MD5_Init @%p for %s - %m", (void*)&ctx, path);
      return -1;
    };
  char buf[1024];
  unsigned char md5dig[MD5_DIGEST_LENGTH+4];
  char md5buf[2*sizeof(md5dig)];
  memset (md5dig, 0, sizeof(md5dig));
  memset (md5buf, 0, sizeof(md5buf));
  long off = 0;
  do
    {
      memset(buf, 0, sizeof(buf));
      off= ftell(fil);
      ssize_t nb = fread(buf, 1, sizeof(buf), fil);
      long newoff = ftell(fil);
      if (nb < 0)
        {
          syslog(LOG_ALERT, "show_md5_mtime failed to fread %s at offset %ld - %m",
                 path, off);
          return -1;
        };
      if (nb==0)
	break;
      if (!MD5_Update(&ctx, buf, nb))
        {
          syslog(LOG_ALERT, "show_md5_mtime failed to MD5_Update %s at offset %ld - %m",
                 path, off);
          return -1;
        };
      off = newoff;
    }
  while (!feof(fil));
  fclose(fil);
  if (!MD5_Final(md5dig, &ctx))
    {
      syslog(LOG_ALERT, "show_md5_mtime failed to MD5_Final %s at offset %ld - %m",
             path, off);
      return -1;
    };
  for (int ix=0; ix<MD5_DIGEST_LENGTH; ix++)
    snprintf(md5buf+(2*ix), 3, "%02x", (unsigned)md5dig[ix]);
  if (firstmd5)
    strncpy(firstmd5, md5buf, 2*MD5_DIGEST_LENGTH);
  syslog(LOG_INFO, "source file %s has %ld bytes; of md5 %s", path, off, md5buf);
  if (mysqlitedb)
    {
      std::int64_t serial = register_sqlite_source_data(path, md5buf, mtime, off);
      return serial;
    }
  else
    return 0;
} // end show_md5_mtime


/// measure with stat(2) the input source files. Return the serial (for sqlite) of the first one.
std::int64_t
stat_input_files(const std::vector<const char*>&progargvec, char*firstmd5)
{
  std::int64_t firstserial = 0;
  int nbargs = progargvec.size();
  int nbsrcfiles = 0;
  for (int ix=1; ix<nbargs && progargvec[ix]; ix++)
    {
      // skip -o outputfile...
      if (!strcmp (progargvec [ix], "-o"))
        {
          ix++;
          continue;
        }
      else
        {
          /// check for source files like *.c *.i *.S *.C *.cc *.cxx *.cpp
          const char*curarg = progargvec[ix];
          if (curarg [0] == '-')
            continue;
          if (!isalnum(curarg[0]) && curarg[0] != '_')
            continue;
          int lenarg = strlen(curarg);
          if (lenarg<3) continue;
          char lastc = curarg[lenarg-1];
          bool isasrc = false;
          if (curarg[lenarg-2]=='.')
            {
              if (lastc == 'c' || lastc == 'S' || lastc == 'i' || lastc == 'C')
                isasrc = true;
            }
          else if (lenarg>=4 && curarg[lenarg-3]=='.')
            {
              char prevc = curarg[lenarg-2];
              if (lastc == 'c' && prevc == 'c')
                isasrc = true;
            }
          else if (lenarg>=4 && curarg[lenarg-4]=='.')
            {
              if (!strcmp(curarg+lenarg-3, "cxx")
                  || !strcmp(curarg+lenarg-3, "cpp"))
                isasrc = true;
            }
          if (isasrc)
            {
              if (access(curarg, R_OK))
                {
                  syslog(LOG_WARNING, "cannot read access source %s: %m", curarg);
                  continue;
                }
              struct stat st;
              memset(&st, 0, sizeof(st));
              if (stat (curarg, &st))
                syslog(LOG_WARNING, "cannot stat source %s: %m", curarg);
              else if ((st.st_mode & S_IFMT) != S_IFREG)
                syslog(LOG_WARNING, "source %s is not a regular file", curarg);
              else
                {
                  time_t mtim = st.st_mtime;
                  struct tm mtimtm;
                  memset(&mtimtm, 0, sizeof(mtimtm));
                  localtime_r(&mtim, &mtimtm);
                  char mtimbuf[64];
                  memset(mtimbuf, 0, sizeof(mtimbuf));
                  strftime(mtimbuf, sizeof(mtimbuf), "%c", &mtimtm);
                  char*rp = realpath(curarg, nullptr);
                  if (!rp)
                    syslog(LOG_WARNING, "source %s failed realpath", curarg);
                  else
                    {
                      syslog(LOG_INFO, "source %s, real %s, has %ld bytes, modified %s", curarg, rp, (long)st.st_size, mtimbuf);
		      std::int64_t serial = show_md5_mtime(rp, mtim, firstmd5);
		      if (nbsrcfiles++ == 0) 
		        firstserial = serial;
                      free(rp);
                    }
                }
            }
        }
    }
  return firstserial;
} // end stat_input_files



void
fork_log_child_process(const char*cmdname, std::string progcmd, double startelapsedtime, std::vector<const char*>progargvec)
{
  if (progargvec.size() <= 1)
    {
      syslog(LOG_WARNING, "no arguments given to command %s (prog %s)", cmdname, progcmd.c_str());
      return;
    }
  syslog(LOG_INFO,
         "(/%d) starting compilation %s of command %s with %d prog.arg", __LINE__,
         cmdname, progcmd.c_str(), (int)(progargvec.size()));
  char firstmd5[MD5_DIGEST_LENGTH+4];
  memset(firstmd5, 0, sizeof(firstmd5));
  std::int64_t firstserial = stat_input_files(progargvec, firstmd5);
  time_t startime = time(nullptr);
  auto pid = fork();
  if (pid<0)
    {
      syslog(LOG_ALERT, "fork failed for %s - %m", progcmd.c_str());
      exit(EX_OSERR);
    }
  else if (pid==0)
    {
      // child process
      execv(cmdname, (char* const*) (progargvec.data()));
      perror(cmdname);
      syslog(LOG_ALERT, "exec of %s failed for %s - %m", cmdname, progcmd.c_str());
      exit (EX_SOFTWARE);
    }
  else   // father process
    {
      struct rusage rus = {};
      int wst = 0;
      memset (&rus, 0, sizeof(rus));
      /// the below loop is likely to run once
      for(;;)
        {
          auto wpid = wait4(pid, &wst, WUNTRACED, &rus);
          if (wpid == pid)
            break;
          if (wpid < 0)
            {
              syslog(LOG_ALERT,
                     "wait of pid %d for %s failed for %s - %m",
                     (int)pid, cmdname, progcmd.c_str());
              exit(EX_OSERR);
            };
        };
      double endelapsedtime= get_float_time(CLOCK_MONOTONIC);
      double usertime = 1.0*rus.ru_utime.tv_sec + 1.0e-6*rus.ru_utime.tv_usec;
      double systime = 1.0*rus.ru_stime.tv_sec + 1.0e-6*rus.ru_stime.tv_usec;
      long maxrss = rus.ru_maxrss; //kilobytes
      long pageflt = rus.ru_minflt + rus.ru_majflt;
      if (wst==0)
        {
          syslog(LOG_INFO, "%s completed successfully compilation %s in %.4g elapsed seconds, %.4g user, %.4g sys cpu seconds, %ld Kbytes RSS, %ld pages faults (pid %d)",
                 cmdname, progcmd.c_str(), endelapsedtime-startelapsedtime,
                 usertime, systime, maxrss, pageflt,
                 (int)pid);
	  if (mysqlitedb && firstserial>0)
	    register_sqlite_compilation (firstserial, firstmd5, progcmd.c_str(), startime, endelapsedtime-startelapsedtime,
					 usertime, systime, maxrss, pageflt);
        }
      else if (WIFEXITED(wst))
        {
          syslog(LOG_WARNING, "%s failed compilation %s in %.4g elapsed seconds, %.4g user, %.4g sys cpu seconds, %ld Kbytes RSS, %ld pages faults (pid %d, exited %d)",
                 cmdname, progcmd.c_str(), endelapsedtime-startelapsedtime,
                 usertime, systime, maxrss, pageflt, WEXITSTATUS(wst),
                 (int)pid);
        }
      else if (WIFSIGNALED(wst))
        {
          syslog(LOG_ERR, "%s crashed compilation %s in %.4g elapsed seconds, %.4g user, %.4g sys cpu seconds, %ld Kbytes RSS, %ld pages faults (pid %d, signal %d=%s)",
                 cmdname, progcmd.c_str(), endelapsedtime-startelapsedtime,
                 usertime, systime, maxrss, pageflt,
                 (int)pid,
                 WTERMSIG(wst),
                 strsignal(WTERMSIG(wst)));
        }
    }
} // end fork_log_child_process

void
split_flags(std::vector<const char*>&flagvec, const char*flags)
{
  assert (flags != nullptr);
  const char*space=nullptr;
  flagvec.reserve(strlen(flags)/3);
  for (const char* pc = flags; pc; pc = space?space+1:nullptr)
    {
      std::string curarg;
      space = strchr(pc, ' ');
      if (space)
        curarg = std::string(pc, space-pc-1);
      else
        curarg = std::string(pc);
      flagvec.push_back(curarg.c_str());
    }
} // end split_flags

void
do_c_compilation(std::vector<const char*>argvec, std::string cmdstr, const char*linkflags)
{
  double startelapsedtime= get_float_time(CLOCK_MONOTONIC);
  auto cflags= getenv("LOGGED_CFLAGS");
  std::vector<const char*> progargvec;
  std::vector<const char*> cflagvec;
  std::vector<const char*> linkflagvec;
  if (cflags)
    split_flags(cflagvec, cflags);
  if (linkflags)
    split_flags(linkflagvec, linkflags);
  progargvec.reserve(cflagvec.size()
                     + linkflagvec.size() + argvec.size() + 2);
  assert (mygcc != nullptr && mygcc[0] != (char)0);
  progargvec.push_back(mygcc);
  for (auto itcfla : cflagvec)
    progargvec.push_back(itcfla);
  for (int ix=1; ix<(int)argvec.size(); ix++)
    progargvec.push_back(argvec[ix]);
  for (auto itlfla : linkflagvec)
    progargvec.push_back(itlfla);
  std::string progcmd;
  int cnt=0;
  for (auto itarg: progargvec)
    {
      if (cnt>0)
        progcmd.append(" ");
      progcmd.append(itarg);
      cnt++;
    };
  progargvec.push_back(nullptr);
  syslog (LOG_INFO, "%s running C compilation %s for %s",
          argvec[0], progcmd.c_str(), cmdstr.c_str());
  fork_log_child_process(mygcc, progcmd, startelapsedtime, progargvec);
} // end do_c_compilation

void
do_cxx_compilation(std::vector<const char*>argvec, std::string cmdstr,  const char*linkflags)
{
  auto cxxflags= getenv("LOGGED_CXXFLAGS");
  double startelapsedtime= get_float_time(CLOCK_MONOTONIC);
  std::vector<const char*> progargvec;
  std::vector<const char*> cflagvec;
  std::vector<const char*> linkflagvec;
  if (cxxflags)
    split_flags(cflagvec, cxxflags);
  if (linkflags)
    split_flags(linkflagvec, linkflags);
  progargvec.reserve(cflagvec.size()
                     + linkflagvec.size() + argvec.size() + 2);
  assert (mygcc != nullptr && mygcc[0] != (char)0);
  progargvec.push_back(mygcc);
  for (auto itcfla : cflagvec)
    progargvec.push_back(itcfla);
  for (int ix=1; ix<(int)argvec.size(); ix++)
    progargvec.push_back(argvec[ix]);
  for (auto itlfla : linkflagvec)
    progargvec.push_back(itlfla);
  std::string progcmd;
  int cnt = 0;
  for (auto itarg: progargvec)
    {
      if (cnt>0)
        progcmd.append(" ");
      progcmd.append(itarg);
      cnt++;
    }
  progargvec.push_back(nullptr);
  syslog (LOG_INFO, "%s running C++ compilation %s - %s", progargvec[0], progcmd.c_str(),
          cmdstr.c_str());
  fork_log_child_process(mygcc, progcmd, startelapsedtime, progargvec);
} // end do_cxx_compilation


struct Sql_request_data
{
  static int callback(void*data, int nbcol, char**colname, char**colval);
  static constexpr long _rdata_magic_ = 60433327;
  long _rdata_magicnum;
  long _rdata_count;
  const char* _rdata_sql;
public:
  Sql_request_data(const char*sql)
    : _rdata_magicnum(_rdata_magic_), _rdata_count(0), _rdata_sql(sql) {};
  ~Sql_request_data()
  {
    _rdata_magicnum = 0;
    _rdata_count = -1;
    _rdata_sql = nullptr;
  };
  bool valid() const
  {
    return _rdata_magicnum == _rdata_magic_;
  };
  long count() const
  {
    return _rdata_count;
  };
  const char*sql() const
  {
    return _rdata_sql;
  };
};				// end Sql_request_data

int
Sql_request_data::callback(void*data, int nbcol, char**colname, char**colval)
{
  Sql_request_data* thisdata = (Sql_request_data*)data;
  assert (thisdata != nullptr && thisdata->_rdata_magicnum == _rdata_magic_);
  long cnt = thisdata->_rdata_count++;
  FILE* fout = stdout;
  if (cnt == 0)
    {
      // output commented request, line by line
      const char*reqsql = thisdata->sql();
      const char*eol = nullptr;
      for (const char*pc=reqsql; pc && *pc; pc = eol)
        {
          eol = strchr(pc, '\n');
          if (eol)
            {
              fprintf(fout, "#-%*s\n", (int)(eol-pc), pc);
              eol++;
            }
          else
            fprintf(fout, "#-%s\n", pc);
        };
      // output column names
      fputs("#|", fout);
      for (int cix=0; cix<nbcol; cix++)
        {
          if (cix>0)
            putc('\t', fout);
          fputs(colname[cix], fout);
        };
      putc('\n', fout);
    };
  for (int cix=0; cix<nbcol; cix++)
    {
      if (cix>0)
        putc('\t', fout);
      fputs(colval[cix], fout);
    };
  putc('\n', fout);
  fflush(fout);
  return 0;
} // end Sql_request_data::callback

void
run_sqlite_request(const char*sqlreq)
{
  char*msgerr=nullptr;
  Sql_request_data reqdata(sqlreq);
  int r = sqlite3_exec(mysqlitedb,
                       sqlreq,
                       Sql_request_data::callback,
                       &reqdata,
                       &msgerr);
  if (r != SQLITE_OK)
    {
      syslog(LOG_ALERT, "run_sqlite_request (path %s) failure #%d for request %s: %s",
             mysqlitepath, r, sqlreq, msgerr?msgerr:"???");
      exit(EXIT_FAILURE);
    }
  else
    {
      fprintf(stdout, "#- %ld rows\n\n", (long) reqdata.count());
      fflush(stdout);
      syslog(LOG_INFO, "run_sqlite_request did %s with %ld rows", sqlreq,
             reqdata.count());
    }
} // end of run_sqlite_request

void
create_sqlite_database(void)
{
  assert (mysqlitedb);
  assert (mysqlitepath);
  char *msgerr = nullptr;
  {
    const char* inireq1= R"!*(
PRAGMA encoding = 'UTF-8';
BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS tb_sourcepath (
  srcp_serial INTEGER PRIMARY KEY ASC AUTOINCREMENT,
  srcp_realpath VARCHAR(512) NOT NULL UNIQUE,
  srcp_last_compil_id INTEGER NOT NULL,
  srcp_last_compil_time DATETIME NOT NULL
);
)!*";
    int r1 = sqlite3_exec(mysqlitedb,
			  inireq1,
			  nullptr,
			  nullptr,
			  &msgerr);
    if (r1 != SQLITE_OK) {
      syslog(LOG_ALERT, "create_sqlite_database @L%d (path %s) failure #%d : %s\n request was %s", __LINE__,
	     mysqlitepath, r1, msgerr?msgerr:"???", inireq1);
      exit(EXIT_FAILURE);
    };
  }
  //
  {
    const char* inireq2= R"!*(
CREATE UNIQUE INDEX IF NOT EXISTS ix_sourcepath_realpath ON tb_sourcepath (srcp_realpath);
CREATE INDEX IF NOT EXISTS ix_sourcepath_compilid ON tb_sourcepath (srcp_last_compil_id);
CREATE INDEX IF NOT EXISTS ix_sourcepath_compiltime ON tb_sourcepath (srcp_last_compil_time);
)!*";
  
    int r2 = sqlite3_exec(mysqlitedb,
			  inireq2,
			  nullptr,
			  nullptr,
			  &msgerr);
    if (r2 != SQLITE_OK) {
      syslog(LOG_ALERT, "create_sqlite_database @L%d (path %s) failure #%d : %s\n request was %s", __LINE__,
	     mysqlitepath, r2, msgerr?msgerr:"???", inireq2);
      exit(EXIT_FAILURE);
    };
  }
  //
  {
    const char* inireq3= R"!*(
CREATE TABLE IF NOT EXISTS tb_sourcedata (
  srcd_path_serial INTEGER NOT NULL PRIMARY KEY ASC, 
  srcd_path_mtime DATETIME NOT NULL,
  srcd_path_md5 CHAR(32) NOT NULL,
  srcd_path_size INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS ix_sourcedata_serial ON tb_sourcedata(srcd_path_serial);
CREATE INDEX IF NOT EXISTS ix_sourcedata_mtime ON tb_sourcedata(srcd_path_mtime);
)!*";
  
    int r3 = sqlite3_exec(mysqlitedb,
			  inireq3,
			  nullptr,
			  nullptr,
			  &msgerr);
    if (r3 != SQLITE_OK) {
      syslog(LOG_ALERT, "create_sqlite_database @L%d (path %s) failure #%d : %s\n request was %s", __LINE__,
	     mysqlitepath, r3, msgerr?msgerr:"???", inireq3);
      exit(EXIT_FAILURE);
    };
  }

  {
    const char* inireq4= R"!*(
CREATE TABLE IF NOT EXISTS tb_successful_compilation (
  compil_serial INTEGER PRIMARY KEY ASC AUTOINCREMENT,
  compil_firstsrc_id INTEGER NOT NULL,
  compil_firstsrc_md5 CHAR(32) NOT NULL,
  compil_command TEXT NOT NULL,
  compil_start_time DATETIME NOT NULL,
  compil_elapsed_time DOUBLE NOT NULL,
  compil_usercpu_time DOUBLE NOT NULL,
  compil_syscpu_time  DOUBLE NOT NULL,
  compil_page_faults INTEGER NOT NULL,
  compil_max_rss INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS ix_compilation_id ON tb_successful_compilation(compil_firstsrc_id);

END TRANSACTION;
)!*";
  
    int r4 = sqlite3_exec(mysqlitedb,
			  inireq4,
			  nullptr,
			  nullptr,
			  &msgerr);
    if (r4 != SQLITE_OK) {
      syslog(LOG_ALERT, "create_sqlite_database @L%d  (path %s) failure #%d : %s\n request was %s", __LINE__,
	     mysqlitepath, r4, msgerr?msgerr:"???", inireq4);
      exit(EXIT_FAILURE);	   
    }
  }
  syslog(LOG_INFO, "create_sqlite_database initialized database %s", mysqlitepath);
} // end create_sqlite_database

void
initialize_sqlite(void)
{
  assert (mysqlitepath != nullptr);
  bool oldsqlite = !access(mysqlitepath, F_OK);
  int err = sqlite3_open_v2(mysqlitepath,
			    &mysqlitedb,
			    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
			    nullptr);
  if (err != SQLITE_OK) {
    char errbuf[16];
    memset(errbuf, 0, sizeof(errbuf));
    snprintf(errbuf, sizeof(errbuf), "sqlerr#%d", err);
    syslog(LOG_ALERT, "sqlite3_open_v2 failed on %s - %s (%m)",
	   mysqlitepath, mysqlitedb?sqlite3_errmsg(mysqlitedb):errbuf);
    exit(EXIT_FAILURE);
  }
  if (!oldsqlite)
    create_sqlite_database();
  if (mysqliterequest)
    run_sqlite_request(mysqliterequest);
} // end of initialize_sqlite

////////////////////////////////////////////////////////////////
int
main(int argc, char**argv)
{
  if (argc <= 1)
    {
      std::clog << argv[0] << " requires at least one argument. Try "
                << argv[0] << " --help" << std::endl;
      exit(EXIT_FAILURE);
    }
  openlog(argv[0], LOG_PERROR|LOG_PID, LOG_USER);
  std::string argstr;
  if (!mygcc)
    mygcc = getenv("GCC");
  if (!mygcc)
    mygcc = getenv("LOGGED_GCC");
  if (!mygcc)
    mygcc = GCC_EXEC;
  mygxx = getenv("GXX");
  if (!mygxx)
    mygxx= getenv("LOGGED_GXX");
  if (!mygxx)
    mygxx = GXX_EXEC;
  mysqlitepath = getenv("LOGGED_SQLITE");
  bool for_cxx = strstr(argv[0], "++") != nullptr;
  if (argc==2 && !strcmp(argv[1], "--version"))
    {
      openlog(argv[0], LOG_PID, LOG_USER);
      syslog(LOG_INFO, "%s (git %s) running version query: %s --version",
             __FILE__, GITID, argv[0]);
      if (mygcc && !for_cxx)
        {
          syslog(LOG_INFO, "running version for gcc: %s", mygcc);
          argv[0] = (char*)mygcc;
          execv(mygcc, argv);
          perror(mygcc);
          exit(EX_OSERR);
        }
      else if (mygxx && for_cxx)
        {
          syslog(LOG_INFO, "running version for g++: %s", mygxx);
          argv[0] = (char*)mygxx;
          execv(mygxx, argv);
          perror(mygxx);
          exit(EX_OSERR);
        }
      else
        syslog(LOG_INFO, "running native version for %s", argv[0]);
    };
  int nbgccarg=0;
  std::vector<const char*> argvec
    = parse_logged_program_options(argc, argv, argstr, nbgccarg);
  assert (argvec.size() > 0);
  if (!for_cxx && access(mygcc, X_OK))
    {
      syslog (LOG_WARNING, "%s is not executable - %m - for %s", mygcc,
              argstr.c_str());
      const char*defgcc = "/usr/bin/gcc";
      if (access(defgcc, X_OK))
        {
          syslog (LOG_ALERT, "%s is not executable - %m - for %s", defgcc,
                  argstr.c_str());
          exit(EXIT_FAILURE);
        };
      mygcc = defgcc;
    };
  ///
  if (mysqlitepath)
    initialize_sqlite();
  if (for_cxx && access(mygxx, X_OK))
    {
      syslog (LOG_WARNING, "%s is not executable - %m - for %s", mygxx,
              argstr.c_str());
      const char*defgxx = "/usr/bin/g++";
      if (access(defgxx, X_OK))
        {
          syslog (LOG_ALERT, "%s is not executable - %m - for %s", defgxx,
                  argstr.c_str());
          exit(EXIT_FAILURE);
        };
      mygxx = defgxx;
    };
  auto linkflags = getenv("LOGGED_LINKFLAGS");
  if (for_cxx && nbgccarg>0)
    do_cxx_compilation (argvec, argstr, linkflags);
  else if (!for_cxx && nbgccarg>0)
    do_c_compilation (argvec, argstr, linkflags);
} /* end main */

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "./compile-logged-gcc.sh" ;;
 ** End: ;;
 ****************/
