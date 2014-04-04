#  Copyright (c) 1997-2014
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
   my $lib_ext=$Config::Config{dlext};
   if ($^O eq "darwin") {
     $lib_ext="dylib";  # on Mac dlext points to bundle, but we need a shared lib, not a module
   }
   my $ppl_path;
   if (defined ($ppl_path=$options->{ppl})) {
      my $ppl_inc="$ppl_path/include";
      my $ppl_lib=Polymake::Configure::get_libdir($ppl_path, "ppl");
      if (-f "$ppl_inc/ppl.hh" && -f "$ppl_lib/libppl.$lib_ext") {
	 $CXXflags="-I$ppl_inc";
	 $LDflags="-L$ppl_lib -Wl,-rpath,$ppl_lib";
      } else {
	 die "Invalid installation location of libppl: header file ppl.hh and/or library libppl.$lib_ext not found\n";
      }

   } else {
      my $error=Polymake::Configure::build_test_program(<<"---", Libs => "-lppl -lgmp");
#include "ppl.hh"
#include <iostream>
int main() {
  Parma_Polyhedra_Library::C_Polyhedron dummy;
  std::cout << PPL_VERSION_MAJOR << "." << PPL_VERSION_MINOR << "." << PPL_VERSION_REVISION;
  return 0;
}
---
      if ($?==0) {
	 $PPL_output=Polymake::Configure::run_test_program();
	 if ($?) {
     $error=$PPL_output;
	    die "Could not run a test program checking for libppl.\n",
	        "The complete error log follows:\n\n$error\n",
		"Please investigate the reasons and fix the installation.\n";
	 } else {
      $PPL_version=$PPL_output;
      if(Polymake::Configure::v_cmp($PPL_version, "1.1") < 0) {
        die "PPL version is $PPL_version. Minimal required version is 1.1\n"
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
   }

   $Libs="-lppl";
   return $ppl_path ? "libppl=$ppl_path" : "";
}
