#  Copyright (c) 1997-2015
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

# List of variables used in the Makefiles that must be modified when building this extension.
# The complete list of recognized variables is contained in the module perllib/Polymake/Configure.pm.
# Usually you will have to modify only few of them, most probably CXXflags, LDflags, or Libs.
# Please put just the bare names of the variables here, without '$' prefix.

@make_vars=qw( CXXflags LDflags Libs );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( openmp boost ) }=();
}


sub usage {
   print STDERR "  --without-openmp        deactivate OpenMP support for libnormaliz\n",
                "  --with-boost=PATH  installation path of boost library, if non-standard\n";
}

sub proceed {
   my ($options)=@_;
   my $message = "";

   if (defined($Polymake::Configure::GCCversion) && Polymake::Configure::v_cmp($Polymake::Configure::GCCversion, "4.4") >= 0 && $options->{openmp} ne ".none.") {
      $CXXflags = "-fopenmp";
      $LDflags = "-fopenmp";
   } else {
      $CXXflags = "-DOPENMP=no";
      $message = "OpenMP support disabled";
   }


# libnormaliz uses the boost library
# check for the headers and add the appropriate path to CXXflags if neccessary
# uses the same config option as the extension group (where permlib requires boost)

   my $boost_path;

   if (defined ($boost_path=$options->{boost})) {
      $boost_path .= '/include' if (-d "$boost_path/include/boost");
      if (-f "$boost_path/boost/shared_ptr.hpp") {
         $CXXflags.=" -I$boost_path";
      } else {
         die "Invalid installation location of boost library: header file boost/shared_ptr.hpp not found\n";
      }

   } else {
      my $error=Polymake::Configure::build_test_program(<<'---');
#include <boost/shared_ptr.hpp>
#include <boost/iterator/counting_iterator.hpp>
int main() {
  return 0;
}
---
      if ($?) {
         die "Could not compile a test program checking for boost library.\n",
            "The most probable reasons are that the library is installed at a non-standard location,\n",
            "or missing at all.\n",
            "The complete error log follows:\n$error\n\n",
            "Please install the library and specify its location using --with-boost option, if needed.\n";
      }
   }

# add the boost location to the message printed during configuration
   my $boost = $boost_path ? "boost=$boost_path" : "";
   $message .= $message ? ", $boost" : $boost;

   # check GMP C++ bindings
   my $build_error=Polymake::Configure::build_test_program(<<'---', Libs => "-lgmpxx -lgmp");
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

   $Libs="-lgmpxx";

   return "$message";
}
