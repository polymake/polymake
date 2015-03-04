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

@make_vars=qw( CXXflags LDflags Libs );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( ppl ) }=();
}

sub usage {
   print STDERR "  --with-ppl=PATH  installation path of ppl\n";
}

sub proceed {
   my ($options)=@_;
   my $ppl_path;
   my $ppl_version;

   if (defined ($ppl_path=$options->{ppl})) {
      my $ppl_inc="$ppl_path/include";
      my $ppl_lib=Polymake::Configure::get_libdir($ppl_path, "ppl");
      if (-f "$ppl_inc/ppl.hh" && -f "$ppl_lib/libppl.$Config::Config{so}") {
         $CXXflags="-I$ppl_inc";
         $LDflags="-L$ppl_lib -Wl,-rpath,$ppl_lib";
      } else {
         die "Invalid installation location of libppl: header file ppl.hh and/or library libppl.$Config::Config{so} not found\n";
      }
   }
   my $error=Polymake::Configure::build_test_program(<<"---", Libs => "-lppl -lgmp", CXXflags => "$CXXflags", LDflags => "$LDflags");
#include <cstddef>
#include "ppl.hh"
#include <iostream>
int main() {
   Parma_Polyhedra_Library::C_Polyhedron dummy;
   std::cout << PPL_VERSION_MAJOR << "." << PPL_VERSION_MINOR << "." << PPL_VERSION_REVISION;
   return 0;
}
---
   if ($?==0) {
      my $output=Polymake::Configure::run_test_program();
      if ($?) {
         die "Could not run a test program checking for libppl.\n",
             "The complete error log follows:\n\n$output\n",
             "Please investigate the reasons and fix the installation.\n";
      } else {
         $ppl_version=$output;
         if(Polymake::Configure::v_cmp($ppl_version, "0.11.2") < 0) {
            die "PPL version is $ppl_version. Minimal required version is 0.11.2\n"
         }
      }
   } else {
      die "Could not compile a test program checking for libppl.\n",
          "The most probable reasons are that the library is installed at a non-standard location,\n",
          "is not configured to build a shared module, or missing at all.\n",
          "The complete error log follows:\n\n$error\n",
          "Please install the library and specify its location using --with-ppl option, if needed.\n",
          "Please remember to enable shared modules when configuring the libppl!\n";
   }

   $Libs="-lppl";
   return "$ppl_version @ ".($ppl_path//"system");
}
