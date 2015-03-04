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

@make_vars=qw( CXXflags );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( boost ) }=();
}

sub usage {
   print STDERR "  --with-boost=PATH  installation path of boost library, if non-standard\n";
}

sub proceed {
   my ($options)=@_;
   my $boost_path;
   # everything can include permlib headers
   $CXXflags='-I${ExtensionTop}/external/permlib/include';

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

   # check GMP C++ bindings (for sympol)
   my $build_error=Polymake::Configure::build_test_program(<<'---', Libs => "$Polymake::Configure::ARCHFLAGS -lgmpxx -lgmp");
#include <gmpxx.h>
#include <iostream>
int main() {
   mpq_class x(7,3);
   std::cout << __GNU_MP_VERSION << '.' << __GNU_MP_VERSION_MINOR << '.' << __GNU_MP_VERSION_PATCHLEVEL << std::flush;
   return 0;
}
---
   if ($?==0) {
      my $is_version=Polymake::Configure::run_test_program();
      if ($?==0) {
         if (Polymake::Configure::v_cmp($is_version,"4.2.0")<0) {
            die "The GNU Multiprecision Library (GMP) C++ bindings installed at your site are of version $is_version\n",
                "while 4.2.0 is the minimal required version.\n",
                "Since a more recent GMP installation was found, it was probably configured without --enable-cxx .\n";
         }
      } else {
         die "Could not run a test program linked to the C++ version of the GNU Multiprecision Library (GMP).\n",
             "Probably the shared library libgmpxx.$Config::Config{dlext} is missing or of an incompatible machine type.\n";
      }
   } else {
      die "Could not compile a test program checking for C++ bindings of the GNU Multiprecision Library (GMP).\n",
          "The most probable reasons are that a gmpxx package is missing, lacking the developer's subpackage \n",
          "or GMP was configured without C++ support (--enable-cxx).\n",
          "Please refer to the installation instructions at $Wiki/howto/install.\n",
          "The complete error log follows:\n", $build_error;
   }

   return $boost_path ? "boost=$boost_path" : "";
}
