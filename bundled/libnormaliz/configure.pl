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
}


sub usage {

}

sub proceed {
   my ($options)=@_;
   my $message;

# openmp flags are set in the main configure script
   if (defined($Polymake::Configure::CXXflags) && $Polymake::Configure::CXXflags !~ /-fopenmp/) {
      $CXXflags = "-DOPENMP=no";
      $message = "OpenMP support disabled";
   }

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
