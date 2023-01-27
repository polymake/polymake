#  Copyright (c) 1997-2023
#  Ewgenij Gawrilow, Michael Joswig, and the polymake team
#  Technische Universität Berlin, Germany
#  https://polymake.org
#
#  This program is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation; either version 2, or (at your option) any
#  later version: http://www.gnu.org/licenses/gpl.txt.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#-------------------------------------------------------------------------------

@conf_vars=qw( CXXFLAGS LDFLAGS LIBS INCLUDEDIR );


sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( scip ) }=();
}


sub usage {
  print STDERR "  --with-scip=PATH      Directory where SCIP was built or installed.\n",
               "                        This should contain 'lib/libscip.so' and the\n",
               "                        header file 'scip.h' somewhere below.\n";
}

sub proceed {
   my ($options)=@_;
   my ($path, $version, $gmp);
   if (defined ($path=$options->{scip})) {
      if (my $scip_h=`find $path -type f -name scip.h`) {
         my ($incdir)= $scip_h =~ $Polymake::directory_of_cmd_re;
         $incdir =~ s/\/scip$//;
         $INCLUDEDIR = $incdir;
         $CXXFLAGS = "-I$incdir";
      } else {
         die "invalid SCIP location $path: header file scip.h not found anywhere below\n";
      }
      my $libdir = Polymake::Configure::get_libdir($path, "scip","so");
      $LDFLAGS = "-L$libdir -Wl,-rpath,$libdir";
   }

   $LIBS="-lscip -lz";

   # modified from scip/examples/CallableLibrary/src/circle.c
   my $testcode = <<'---';

#include <stdio.h>

#include "scip/pub_misc.h"
#include "scip/scip.h"
#include "scip/scipdefplugins.h"

/** number of points to enclose by a circle */
static const int npoints = 10;

/** seed for random number generator */
static const unsigned int randseed = 42;

/** sets up problem */
static
SCIP_RETCODE setupProblem(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_RANDNUMGEN*      randnumgen          /**< random number generator */
   )
{
   SCIP_VAR* a;
   SCIP_VAR* b;
   SCIP_VAR* r;
   char name[SCIP_MAXSTRLEN];
   int i;
   /* create empty problem */
   SCIP_CALL( SCIPcreateProbBasic(scip, "circle") );
   /* create variables and add to problem */
   SCIP_CALL( SCIPcreateVarBasic(scip, &a, "a", -SCIPinfinity(scip), SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS) );
   SCIP_CALL( SCIPcreateVarBasic(scip, &b, "b", -SCIPinfinity(scip), SCIPinfinity(scip), 0.0, SCIP_VARTYPE_CONTINUOUS) );
   SCIP_CALL( SCIPcreateVarBasic(scip, &r, "r", 0.0, SCIPinfinity(scip), 1.0, SCIP_VARTYPE_CONTINUOUS) );
   SCIP_CALL( SCIPaddVar(scip, a) );
   SCIP_CALL( SCIPaddVar(scip, b) );
   SCIP_CALL( SCIPaddVar(scip, r) );
   /* create soc constraints, add to problem, and forget about them */
   for( i = 0; i < npoints; ++i )
   {
      SCIP_CONS* cons;
      SCIP_VAR* ab[2];
      SCIP_Real xy[2];
      ab[0] = a;
      ab[1] = b;
      xy[0] = -SCIPrandomGetReal(randnumgen, 1.0, 10.0);
      xy[1] = -SCIPrandomGetReal(randnumgen, 1.0, 10.0);
      (void) SCIPsnprintf(name, SCIP_MAXSTRLEN, "point%d", i);
#if SCIP_VERSION_MAJOR >= 8
      SCIP_CALL( SCIPcreateConsBasicSOCNonlinear(scip, &cons, name, 2, ab, NULL, xy, 0.0, r, 1.0, 0.0) );
#else
      SCIP_CALL( SCIPcreateConsBasicSOC(scip, &cons, name, 2, ab, NULL, xy, 0.0, r, 1.0, 0.0) );
#endif
      SCIP_CALL( SCIPaddCons(scip, cons) );
      SCIP_CALL( SCIPreleaseCons(scip, &cons) );
   }

   /* release variables */
   SCIP_CALL( SCIPreleaseVar(scip, &a) );
   SCIP_CALL( SCIPreleaseVar(scip, &b) );
   SCIP_CALL( SCIPreleaseVar(scip, &r) );

   return SCIP_OKAY;
}

int main(){
   SCIP* scip;
   SCIP_RANDNUMGEN* randnumgen;
   SCIP_CALL( SCIPcreate(&scip) );
   SCIP_CALL( SCIPincludeDefaultPlugins(scip) );
   SCIP_CALL( SCIPcreateRandom(scip, &randnumgen, randseed, TRUE) );
   SCIP_CALL( setupProblem(scip, randnumgen) );
   SCIP_CALL( SCIPprintOrigProblem(scip, NULL, "cip", FALSE) );
   SCIP_CALL( SCIPsolve(scip) );
   if( SCIPgetNSols(scip) > 0 )
   {
      SCIP_CALL( SCIPprintSol(scip, SCIPgetBestSol(scip), NULL, FALSE) );
   }
   SCIPprintVersion(scip, NULL);
   SCIPfreeRandom(scip, &randnumgen);
   SCIP_CALL( SCIPfree(&scip) );

   return 0;
}
---
RETRY:
   my $error=Polymake::Configure::build_test_program($testcode, LIBS => "$LIBS -lgmp", CXXFLAGS => "$CXXFLAGS", LDFLAGS => "$LDFLAGS");
   if ($?==0) {
      my $output=Polymake::Configure::run_test_program();
      if ($?) {
         die "Could not run a test program checking for SCIP.\n",
             "The complete error log follows:\n\n$output\n",
             "Please investigate the reasons and fix the installation.\n";
      } else {
         if ($LIBS !~ /-lscip/) {
            # if using libscip try to build a shared library as well to check
            # for relocation problems, i.e. whether it was built with -fPIC
            $error = Polymake::Configure::build_test_program($testcode, LIBS => "$LIBS -lgmp", CXXFLAGS => "$Polymake::Configure::CsharedFLAGS $CXXFLAGS", LDFLAGS => "$Polymake::Configure::LDsharedFLAGS $LDFLAGS");
            goto FAILED if ($?);
         }

         ($version) = $output =~ /SCIP version ([\d.]+)/;
         if (Polymake::Configure::v_cmp($version, "6.0.0") < 0) {
            die "SCIP version is $version. Minimal required version is 6.0.0\n",
                "Full version string:\n$output\n\n";
         }
      }
   } else {

FAILED:
      die "Could not compile a test program/library checking for SCIP.\n",
          "Please make sure SCIP was either compiled with cmake\n",
          "or with GNU make and 'GMP=true' and 'SHARED=true',\n",
          "and specify its location using --with-scip=PATH.\n",
          "The complete error log follows:\n\n$error\n";
   }

   
   # Check for presence of ZIMPL since it interferes with cdd.
   check_zimpl();
   

   if (defined($Polymake::Configure::GCCversion) && Polymake::Configure::v_cmp($Polymake::Configure::GCCversion, "8.0.0") >= 0) {
      $CXXFLAGS .= " -Wno-class-memaccess";
   }

   return "$version @ $path";
}


sub check_zimpl{
   my $testzimpl = <<'---';
#include "scip/reader_zpl.h"
#include "scip/scip_general.h"

extern "C" {
#include "zimpl/ratlptypes.h"
#include "zimpl/numb.h"
#include "zimpl/elem.h"
#include "zimpl/tuple.h"
#include "zimpl/mme.h"
#include "zimpl/set.h"
int sc(){
   Set* S;
   set_copy(S);
   set_free(S);
   return 0;
}
}
int main(){
   return 0;
}
---
   my $zimplbuild=Polymake::Configure::build_test_program($testzimpl, LIBS => "$LIBS -lgmp", CXXFLAGS => "$CXXFLAGS", LDFLAGS => "$LDFLAGS");
   if( $?==0 ){
      die "SCIP was built with ZIMPL and contains the symbols \"set_copy\" and \"set_free\" in libscip.so.\n",
          "These are used in libcdd as well and hence will cause polymake to run unstably.\n",
          "Verify with \"nm -gC libscip.so | grep set_\".\n",
          "Rebuild SCIP with -DZIMPL=false.\n";
   }
}
