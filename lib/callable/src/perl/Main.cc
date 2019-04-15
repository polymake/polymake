/* Copyright (c) 1997-2018
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/perl/glue.h"
#include "polymake/Main.h"
#include "polymakeBootstrapXS.h"

#include <memory>
#include <signal.h>
#include <gmp.h>

#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>

#ifdef __APPLE__
# include <crt_externs.h>
# define PmGetEnvironPtr _NSGetEnviron()
#else
# define PmGetEnvironPtr &environ
#endif

#define ConfParamAsString(x) MacroTokenAsString(x)

namespace pm { namespace perl {
namespace {

const char globalScope[]="Polymake::Scope";

GV* globalScope_gv=nullptr;

void destroy_perl(pTHXx)
{
   PL_perl_destruct_level = 1;
   perl_destruct(aTHXx);
   perl_free(aTHXx);
   PERL_SYS_TERM();
}

void emergency_cleanup() __attribute__((destructor));
void emergency_cleanup()
{
   if (PL_curinterp) {
#ifdef PERL_IMPLICIT_CONTEXT
      dTHX;
      destroy_perl(aTHX);
#else
      destroy_perl(PL_curinterp);
#endif
   }
}

// Detects whether the application has established non-standard handling for SIGCHLD.
// If so, the handling must be temporarily reset to default within perl parts of polymake,
// because otherwise all calls to system() and qx(...) would deliver error codes.
// The return value of this function is glued into the text of the initial script.
char must_reset_SIGCHLD()
{
   struct sigaction sa;
   if (sigaction(SIGCHLD, 0, &sa)) return '0';  // if something went wrong with signal handler detection, we'd better not touch it at all
   return sa.sa_handler != SIG_DFL || (sa.sa_flags & SA_NOCLDWAIT) ? '1' : '0';
}

std::string read_rel_link(std::string link, bool mandatory=true)
{
   struct stat link_stat;
   if (lstat(link.c_str(), &link_stat) < 0) {
      if (mandatory)
         throw std::runtime_error("polymake::Main - " + link + " is not a symlink");
      return {};
   }

   std::string result(link_stat.st_size+1, '\0');
   if (readlink(link.c_str(), const_cast<char*>(result.c_str()), link_stat.st_size + 1) != link_stat.st_size)
      throw std::runtime_error("polymake::Main - readlink(" + link + ") failed");
   result.erase(link_stat.st_size);

   auto slash=link.rfind('/');
   if (slash == std::string::npos)
      throw std::runtime_error("polymake::Main - path " + link + " does not contain enough folder levels");
   link.erase(slash+1);

   while (result.substr(0,3) == "../") {
      result.erase(0,3);
      slash=link.rfind('/', link.size()-2);
      if (slash == std::string::npos)
         throw std::runtime_error("polymake::Main - path " + link + " does not contain enough folder levels");
      link.erase(slash+1);
   }

   return link+result;
}

// Follow symbolic links created by the installation script
void deduce_install_dirs(std::string lib_path, std::string& install_top, std::string& install_arch)
{
   const char stem_name[]="libpolymake";
   constexpr size_t stem_size=sizeof(stem_name)-1;
   auto basename_start=lib_path.rfind(stem_name);
   if (basename_start == std::string::npos)
      throw std::runtime_error("polymake::Main - weird callable library path " + lib_path);

   lib_path.replace(basename_start + stem_size, 0, "-apps", 5);
   lib_path=read_rel_link(lib_path);
   auto libdir_start=lib_path.rfind("/lib/", std::string::npos, 5);
   if (libdir_start == std::string::npos)
      throw std::runtime_error("polymake::Main - weird apps library path " + lib_path);

   install_arch=lib_path.substr(0, libdir_start);
   install_top=read_rel_link(install_arch+"/shared");
}

#if POLYMAKE_DEBUG
#  define scr_debug0(line) line
#  define scr_debug1 "$DebugLevel=1; $DB::single=1;"
#  define scr_debug2 "sub stop_here { print STDERR \"@_\\n\" if @_ } my $loaded=1;\n"
#else
#  define scr_debug0(line)
#  define scr_debug1 ""
#  define scr_debug2 ""
#endif

const char scr0[]=
scr_debug0("#line " ConfParamAsString(__LINE__) " \"" __FILE__ "\"\n") "package Polymake;\n"
"BEGIN { " scr_debug1 "\n"
"   $InstallTop='",   scr8InstallTop[]="';\n"
"   $InstallArch='",  scr8InstallArch[]="';\n"
"   $Arch='" ConfParamAsString(POLYMAKE_CONF_Arch) "';\n"
"   @BundledExts='" ConfParamAsString(POLYMAKE_CONF_BundledExts) "' =~ /(\\S+)/g;\n"
"}\n"
"use lib \"$InstallTop/perllib\", \"$InstallArch/perlx\"",  scr8useLib[]=";\n"
"use Polymake::Main q{", scr8user_opts[]="},", scr8reset_SIGCHLD[]=";\n"
scr_debug2
"1\n";

#undef addlibs
#undef scr_debug1
#undef scr_debug2
}

Main::Main(const std::string& user_opts, std::string install_top, std::string install_arch)
{
   if (PL_curinterp) return;

   Dl_info dli_polymake, dli_perl;
   void* polyhandle = nullptr;
   int dlreturn;
   if ((dlreturn = dladdr((void*)&destroy_perl, &dli_polymake))) {
      polyhandle = dlopen(dli_polymake.dli_fname, RTLD_LAZY | RTLD_NOLOAD | RTLD_GLOBAL );
   }
   if (!polyhandle) {
      std::cerr << "*** WARNING: Failed to (re-)dlopen libpolymake with RTLD_GLOBAL: " 
                << (dlreturn ? dlerror() : "dladdr failed to give shared library pathname.") << "***\n"
                   "    Application modules might fail to load." << std::endl;
   }

   void* perlhandle = nullptr;
   if ((dlreturn = dladdr((void*)&perl_destruct, &dli_perl))) {
      perlhandle = dlopen(dli_perl.dli_fname, RTLD_LAZY | RTLD_NOLOAD | RTLD_GLOBAL );
   }
   if (!perlhandle) {
      std::cerr << "*** WARNING: Failed to (re-)dlopen libperl with RTLD_GLOBAL: " 
                << (dlreturn ? dlerror() : "dladdr failed to give shared library pathname.") << "***\n"
                   "    Perl modules might fail to load." << std::endl;
   }

   if (install_top.empty() != install_arch.empty())
      throw std::runtime_error("polymake::Main - install_top and install_arch arguments must both be empty or set to valid paths");
   if (install_top.empty())
      deduce_install_dirs(dli_polymake.dli_fname, install_top, install_arch);

   std::string script_arg(scr0);
   script_arg += install_top;
   script_arg += scr8InstallTop;
   script_arg += install_arch;
   script_arg += scr8InstallArch;
#ifdef __APPLE__
   std::string fink_base=read_rel_link(install_arch + "/fink-base", false);
   if (!fink_base.empty())
      script_arg += ", \"" + fink_base + "/lib/perl5\"";
#endif
   script_arg += scr8useLib;
   script_arg += user_opts;
   script_arg += scr8user_opts;
   script_arg += must_reset_SIGCHLD();
   script_arg += scr8reset_SIGCHLD;

   const char* perl_start_args[]={ "perl", "-e", script_arg.c_str(), 0 };
   int argc=sizeof(perl_start_args)/sizeof(perl_start_args[0])-1;
   const char **argv=perl_start_args;
   char *** const env=PmGetEnvironPtr;
   // the casts looking evil here, but fortunately, nothing harmful happens within this macro
   PERL_SYS_INIT3(&argc, (char***)&argv, env);
   pTHXx = perl_alloc();
   PL_perl_destruct_level = 1;
   perl_construct(aTHXx);
   PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
   if (perl_parse(aTHXx_ xs_init, argc, (char**)argv, *env)) {
      destroy_perl(aTHXx);
      PL_curinterp = nullptr;
      throw std::runtime_error("could not initialize the perl interpreter");
   }
#ifdef PERL_IMPLICIT_CONTEXT
   pi=aTHXx;
#endif
   perl_run(aTHXx);

   globalScope_gv=gv_fetchpvn_flags(globalScope, sizeof(globalScope)-1, false, SVt_RV);
}

unsigned int Scope::depth=0;

Scope::~Scope()
{
   if (saved) {
      dTHXa(pm_main->pi);
      if (depth-- != id) {
         // can't throw an exception from a destructor
         std::cerr << "polymake::perl::Scope nesting violation" << std::endl;
         std::terminate();
      }
      sv_unref_flags(GvSV(globalScope_gv), SV_IMMEDIATE_UNREF);
      sv_setsv(GvSV(globalScope_gv), saved);
      SvREFCNT_dec(saved);
   }
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
