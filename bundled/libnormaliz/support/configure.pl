#  Copyright (c) 1997-2020
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

@conf_vars=qw( UseBundled CXXFLAGS LDFLAGS LIBS );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( libnormaliz ) }=();
}


sub usage {
   print STDERR "  --with-libnormaliz=PATH  Installation path of libnormaliz, if non-standard.\n",
                "                   By default, polymake will try to use a system-wide\n",
                "                   installation or fall back to the bundled libnormaliz\n",
                "                   (bundled/libnormaliz/external/libnormaliz) if it exists.\n",
                "                   To force the bundled version, specify 'bundled' as PATH.\n";
}

sub check_bundled {
   -e "bundled/libnormaliz/external/libnormaliz/libnormaliz/libnormaliz.h"
}

sub proceed {
   my ($options)=@_;
   my $nmz_path;
   my $nmz_version;
   my $libs = "-lnormaliz";
   $UseBundled=1;

   # check GMP C++ bindings
   my $build_error=Polymake::Configure::build_test_program(<<'---', LIBS => "-lgmpxx -lgmp");
#include <cstddef>
#include <gmpxx.h>
int main() {
   mpz_class z(7);
   mpz_class y(z-z);
   return y.get_si();
}
---

   if ($?!=0) {
      die "Could not compile a test program checking for the GNU Multiprecision Library C++ bindings.\n",
          "The libnormaliz extension needs gmpxx.h and -lgmpxx installed together with GMP.\n",
          "The complete error log follows:\n", $build_error;
   }

   if (defined ($nmz_path=$options->{libnormaliz}) and $nmz_path ne "bundled") {
      my $nmz_inc="$nmz_path/include";
      my $nmz_lib=Polymake::Configure::get_libdir($nmz_path, "normaliz");
      if (-f "$nmz_inc/libnormaliz/libnormaliz.h" && -f "$nmz_lib/libnormaliz.$Config::Config{so}" ) {
         $CXXFLAGS = "-I$nmz_inc";
         $LDFLAGS = "-L$nmz_lib -Wl,-rpath,$nmz_lib";
      } else {
         die "Invalid installation location of libnormaliz header file libnormaliz/libnormaliz.h and/or library libnormaliz.$Config::Config{so} not found\n";
      }
   }

   if ($nmz_path ne "bundled") {
      # check normaliz configuration first:
      my $error=Polymake::Configure::build_test_program(<<'---', LIBS => "", CXXFLAGS => "$CXXFLAGS", LDFLAGS => "$LDFLAGS");
#include <cstddef>
#include <fstream>
#include <iostream>
#include <libnormaliz/nmz_config.h>

int main (int argc, char *argv[])
{
#ifdef ENFNORMALIZ
   std::cout << " -leantic";
#endif
#ifdef NMZ_FLINT
   std::cout << " -lflint";
#endif
   std::cout << std::endl;
   return 0;
}
---
      if ($?) {
         check_bundled() and !defined($nmz_path) or
            die "Could not compile a test program checking for libnormaliz configuration.\n",
                "The complete error log follows:\n\n$error\n",
                "Please investigate the reasons and fix the installation.\n";
      } else {
         chomp(my $extralibs=Polymake::Configure::run_test_program());

         if ($?) {
            check_bundled() and !defined($nmz_path) or
               die "Could not compile a test program checking for libnormaliz configuration.\n",
                   "The complete error log follows:\n\n$extralibs\n",
                   "Please investigate the reasons and fix the installation.\n";
         } else {
            $libs = "$libs $extralibs";
         }
      }

      $error=Polymake::Configure::build_test_program(<<'---', LIBS => "$libs -lgmpxx -lgmp", CXXFLAGS => "$CXXFLAGS", LDFLAGS => "$LDFLAGS");

#include <cstddef>
#include <vector>
#include <fstream>
#include <iostream>
#include <gmpxx.h>
#include <libnormaliz/libnormaliz.h>
#include <libnormaliz/cone.h>
#include <libnormaliz/vector_operations.h>
#include <libnormaliz/cone_property.h>
#include <libnormaliz/integer.h>
using namespace std;
using namespace libnormaliz;
typedef mpz_class Integer;

int main (int argc, char *argv[])
{
   vector<vector<Integer> > rays {{1,0},{1,2}};
   Cone<Integer> nmzcone(Type::cone,rays);
   if (nmzcone.getNrHilbertBasis() != 3)
      throw std::runtime_error("libnormaliz failed to compute hilbert basis!");
   cout << "version " << NMZ_VERSION_MAJOR << "." << NMZ_VERSION_MINOR << "." << NMZ_VERSION_PATCH << endl;
   return 0;
}
---
      if ($?==0) {
         my $message=Polymake::Configure::run_test_program();
         if ($?) {
            check_bundled() and !defined($nmz_path) or
               die "Could not run a test program checking for libnormaliz.\n",
                   "The complete error log follows:\n\n$message\n",
                   "Please investigate the reasons and fix the installation.\n";
         } else {
            ($nmz_version) = $message =~ /version ([0-9]\.[0-9]\.[0-9])/;
            my $minversion = "3.6.0";
            if (Polymake::Configure::v_cmp($nmz_version,$minversion) >= 0 && $nmz_version ne "3.7.0") {
               $UseBundled = 0;
            } else {
               check_bundled() and !defined($nmz_path) or
                  die "Your libnormaliz version $nmz_version is not supported, at least version $minversion is required and\nversion 3.7.0 is broken due to missing configuration flags.\nPlease install a newer version or use the bundled version by omitting\n--with-libnormaliz during configuration.\n";
            }
         }
      } else {
         check_bundled() and !defined($nmz_path) or
            die "Could not compile a test program checking for libnormaliz.\n",
                "The most probable reasons are that the library is installed at a non-standard location,\n",
                "is not configured to build a shared module, or missing at all.\n",
                "Also make sure that libnormaliz was built with the same C++ library as polymake,",
                "especially if the errors below show missing symbols containing std::__1::vector or std::vector.",
                "The complete error log follows:\n\n$error\n",
                "Please install the library and specify its location using --with-libnormaliz option, if needed.\n";
      }
   }
   if ($UseBundled) {
      die "bundled libnormaliz requested but it cannot be found"
         if (!check_bundled());
      undef $LIBS;
      $CXXFLAGS='-I${root}/bundled/libnormaliz/external/libnormaliz -DBUNDLED_LIBNORMALIZ';
      $message = "bundled";

      # openmp flags are set in the main configure script
      if (defined($Polymake::Configure::CXXFLAGS) && $Polymake::Configure::CXXFLAGS !~ /-fopenmp/) {
         $CXXFLAGS .= " -DOPENMP=no";
         $message .= " [OpenMP support disabled]";
      }
   } else {
      $LIBS="$libs";
      $message = "$nmz_version @ ".($nmz_path//"system")
   }

   $LIBS.=" -lgmpxx";

   return "$message";
}
