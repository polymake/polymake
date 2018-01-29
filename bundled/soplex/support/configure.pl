#  Copyright (c) 1997-2018
#  Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
#  http://www.polymake.org
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

@conf_vars=qw( CXXFLAGS LDFLAGS LIBS );


sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( soplex ) }=();
}


sub usage {
  print STDERR "  --with-soplex=PATH    Directory where SoPlex was built.\n",
               "                        This should contain 'lib/libsoplex.so' and\n",
               "                        the header file soplex.h somewhere below.\n";
}

sub proceed {
   my ($options)=@_;
   my ($path, $version, $gmp);
   if (defined ($path=$options->{soplex})) {
      if (my $soplex_h=`find $path -type f -name soplex.h`) {
	 my ($incdir)= $soplex_h =~ $Polymake::directory_of_cmd_re;
	 $CXXFLAGS = "-I$incdir";
      } else {
	 die "invalid Soplex location $path: header file soplex.h not found anywhere below\n";
      }
      my $libdir=Polymake::Configure::get_libdir($path, "soplex");
      $LDFLAGS = "-L$libdir -Wl,-rpath,$libdir";
   }

   # example code from src/example.cpp
   my $error=Polymake::Configure::build_test_program(<<'---', LIBS => "-lsoplex -lgmp -lz", CXXFLAGS => "$CXXFLAGS", LDFLAGS => "$LDFLAGS");
#include <cstddef>
#include <iostream>
#include "soplex.h"
#include <gmp.h>

using namespace soplex;

int main() {
   SoPlex soplex;
   soplex.setIntParam(soplex::SoPlex::IntParam::VERBOSITY, SoPlex::VERBOSITY_ERROR);
   soplex.setRealParam(soplex::SoPlex::RealParam::FEASTOL, 0.0);
   soplex.setRealParam(soplex::SoPlex::RealParam::OPTTOL, 0.0);
   soplex.setIntParam(soplex::SoPlex::IntParam::SOLVEMODE, SoPlex::SOLVEMODE_RATIONAL);
   soplex.setIntParam(soplex::SoPlex::IntParam::SYNCMODE, SoPlex::SYNCMODE_AUTO);

   soplex.setIntParam(soplex::SoPlex::IntParam::OBJSENSE, SoPlex::OBJSENSE_MINIMIZE);
   DSVector col(0);
   soplex.addColReal(LPCol(2.0, col, infinity, 15.0));
   soplex.addColReal(LPCol(3.0, col, infinity, 20.0));
   DSVector row1(2);
   row1.add(0, 1.0);
   row1.add(1, 5.0);
   soplex.addRowReal(LPRow(100.0, row1, infinity));

   SPxSolver::Status status = soplex.solve();

   if(status != SPxSolver::OPTIMAL) {
      std::cout << "Failed to solve example-LP" << std::endl;
      return 1;
   }
   soplex.setIntParam(soplex::SoPlex::IntParam::VERBOSITY, SoPlex::VERBOSITY_NORMAL);
   soplex.printVersion();
   return 0;
}
---
   if ($?==0) {
      my $output=Polymake::Configure::run_test_program();
      if ($?) {
         die "Could not run a test program checking for SoPlex.\n",
             "The complete error log follows:\n\n$output\n",
             "Please investigate the reasons and fix the installation.\n";
      } else {
         ($version) = $output =~ /SoPlex version ([\d.]+)/;
         if (Polymake::Configure::v_cmp($version, "2.2.0") < 0) {
            die "SoPlex version is $version. Minimal required version is 2.2.0\n",
                "Full version string:\n$output\n\n";
         }
         ($gmp) = $output =~ /\[rational: ([-\d\w\s.]+)\]/;
         if (!$gmp =~ /GMP [-\w\d.]+/) {
            die "Please make sure SoPlex was compiled with 'GMP=true'.\n",
                "Full version string:\n$output\n\n";
         }
      }
   } else {
      die "Could not compile a test program checking for SoPlex.\n",
          "Please make sure SoPlex was compiled with 'GMP=true' and 'SHARED=true'\n",
          "and specify its location using --with-soplex=PATH.\n",
          "The complete error log follows:\n\n$error\n";
   }
   $LIBS="-lsoplex -lz";
   return "$version [$gmp] @ $path";
}
