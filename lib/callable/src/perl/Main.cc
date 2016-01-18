/* Copyright (c) 1997-2015
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
#include "Bootstrap.h"

#include <memory>
#include <signal.h>
#include <gmp.h>

#include <dlfcn.h>

#ifdef __APPLE__
# include <crt_externs.h>
# define PmGetEnvironPtr _NSGetEnviron()
#else
# define PmGetEnvironPtr &environ
#endif

namespace pm { namespace perl {
namespace {

const char globalScope[]="Polymake::Scope";

GV *globalScope_gv=NULL;

void destroy_perl(pTHXx)
{
   PL_perl_destruct_level = 1;
   perl_destruct(aTHXx);
   perl_free(aTHXx);
   PERL_SYS_TERM();
}

#ifndef PERL_IMPLICIT_CONTEXT
PerlInterpreter *static_perl = NULL;
#endif

void emergency_cleanup() __attribute__((destructor));
void emergency_cleanup()
{
   if (PL_curinterp) {
#ifdef PERL_IMPLICIT_CONTEXT
      dTHX;
      destroy_perl(aTHX);
#else
      destroy_perl(static_perl);
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

#ifdef POLYMAKE_CONF_FinkBase
#  define addlibs ", \"" POLYMAKE_CONF_FinkBase "/lib/perl5\""
#else
#  define addlibs ""
#endif
#if POLYMAKE_DEBUG
#  define scr_debug1 "$DebugLevel=1; $DB::single=1;"
#  define scr_debug2 "sub stop_here { print STDERR \"@_\\n\" if @_ } my $loaded=1;\n"
#else
#  define scr_debug1 ""
#  define scr_debug2 ""
#endif

#define ToString1(x) FirstArgAsString(x)

const char scr0[]=
"#line " ToString1(__LINE__) " \"" __FILE__ "\"\n" "package Polymake;\n"
"BEGIN { " scr_debug1 "\n"
"   $InstallTop='",   scr8InstallTop[]="';\n"
"   $InstallArch='",  scr8InstallArch[]="';\n"
"   $Arch='" POLYMAKE_CONF_Arch "';\n"
"}\n"
"use lib \"$InstallTop/perllib\", \"$InstallArch/perlx\"" addlibs ";\n"
"use Polymake::Main q{", scr8user_opts[]="},", scr8reset_SIGCHLD[]=";\n"
scr_debug2
"1\n";

#undef addlibs
#undef scr_debug1
#undef scr_debug2
#undef ToString1
}

Main::Main(const std::string& user_opts, const std::string& install_top, const std::string& install_arch)
{
   if (PL_curinterp != NULL) return;

   Dl_info dli;
   void *polyhandle = NULL;
   int dlreturn;
   if ((dlreturn = dladdr((void*)&destroy_perl, &dli))) {
      polyhandle = dlopen(dli.dli_fname, RTLD_LAZY | RTLD_NOLOAD | RTLD_GLOBAL );
   }
   if (!polyhandle) {
      std::cerr << "*** WARNING: Failed to (re-)dlopen libpolymake with RTLD_GLOBAL: " 
                << (dlreturn ? dlerror() : "dladdr failed to give shared library pathname.") << "***\n"
                   "    Application modules might fail to load." << std::endl;
   }

   void *perlhandle = NULL;
   if ((dlreturn = dladdr((void*)&perl_destruct, &dli))) {
      perlhandle = dlopen(dli.dli_fname, RTLD_LAZY | RTLD_NOLOAD | RTLD_GLOBAL );
   }
   if (!perlhandle) {
      std::cerr << "*** WARNING: Failed to (re-)dlopen libperl with RTLD_GLOBAL: " 
                << (dlreturn ? dlerror() : "dladdr failed to give shared library pathname.") << "***\n"
                   "    Perl modules might fail to load." << std::endl;
   }

   std::string script_arg(scr0);
   if (install_top.empty())
      script_arg+=POLYMAKE_CONF_InstallTop;
   else
      script_arg+=install_top;
   script_arg+=scr8InstallTop;
   if (install_arch.empty())
      script_arg+=POLYMAKE_CONF_InstallArch;
   else
      script_arg+=install_arch;
   script_arg+=scr8InstallArch;
   script_arg+=user_opts;
   script_arg+=scr8user_opts;
   script_arg+=must_reset_SIGCHLD();
   script_arg+=scr8reset_SIGCHLD;
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
      throw std::runtime_error("could not initialize the perl interpreter");
   }
#ifndef PERL_IMPLICIT_CONTEXT
   static_perl=aTHXx;
#endif
   perl_run(aTHXx);

   globalScope_gv=gv_fetchpvn_flags(globalScope, sizeof(globalScope)-1, false, SVt_RV);
}

unsigned int Scope::depth=0;

Scope::~Scope()
{
   if (saved != NULL) {
      dTHX;
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
