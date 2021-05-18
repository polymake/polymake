#  Copyright (c) 1997-2021
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

@conf_vars=qw( CXXFLAGS LDFLAGS LIBS );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( ppl ) }=();
}

sub usage {
   print STDERR "  --with-ppl=PATH  installation path of Parma Polyhedral Library\n";
}

sub proceed {
   my ($options)=@_;
   my ($ppl_path, $ppl_version, $ppl_inc, $ppl_lib, $ppl_config);

   if (defined ($ppl_path=$options->{ppl})) {
      $ppl_inc="$ppl_path/include";
      $ppl_lib=Polymake::Configure::get_libdir($ppl_path, "ppl");
   } elsif ($ppl_config=Polymake::Configure::find_program_in_path("ppl-config")) {
      ($ppl_path)= $ppl_config =~ $Polymake::directory_of_cmd_re;
      $ppl_path =~ s|/bin$||;
      if (`$ppl_config --coefficients` !~ /\bmpz\b/) {
	 if ($?) {
	    die "PPL found at $ppl_path seems unusable: ppl-config failed.\n";
	 } else {
	    die "PPL found at $ppl_path is built without GMP support\n";
	 }
      }
      $ppl_inc=`$ppl_config --includedir`;  chomp $ppl_inc;
      $ppl_lib=`$ppl_config --libdir`;      chomp $ppl_lib;
   }
   if ($ppl_inc) {
      if (-f "$ppl_inc/ppl.hh") {
         $CXXFLAGS = "-I$ppl_inc";
      } elsif (glob("$ppl_inc/ppl*.h")) {
	 die "PPL found at $ppl_path is built without C++ interface\n";
      } else {
         die "Invalid installation location of libppl: header file $ppl_inc/ppl.hh does not exist\n";
      }
   }
   if ($ppl_lib) {
      if (-f "$ppl_lib/libppl.$Config::Config{so}") {
         $LDFLAGS = "-L$ppl_lib";
         $LDFLAGS .= " -Wl,-rpath,$ppl_lib" unless $ppl_lib =~ m#^/usr/lib#;
      } elsif ("$ppl_lib/libppl.a") {
	 die "PPL found at $ppl_path is built without shared libraries.\n";
      } else {
         die "Invalid installation location of libppl: library libppl.$Config::Config{so} does not exist\n";
      }
   }
   my $error=Polymake::Configure::build_test_program(<<"---", LIBS => "-lppl -lgmp", CXXFLAGS => "$CXXFLAGS", LDFLAGS => "$LDFLAGS");
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
         if (Polymake::Configure::v_cmp($ppl_version, "1.2") < 0) {
            die "PPL version is $ppl_version. Minimal required version is 1.2\n";
         }
      }
   } else {
      die "Could not compile a test program checking for libppl.\n",
          "The most probable reasons are that the library is installed at a non-standard location,\n",
          "is not configured to build a shared module, or missing at all.\n",
          "The complete error log follows:\n\n$error\n",
          "Please install the library and specify its location using --with-ppl option, if needed.\n",
          "Please remember to enable C++ interface and GMP support when configuring the libppl!\n";
   }

   if (defined($Polymake::Configure::GCCversion) && Polymake::Configure::v_cmp($Polymake::Configure::GCCversion, "8.0.0") >= 0) {
      $CXXFLAGS .= " -Wno-class-memaccess";
   }

   $LIBS="-lppl";
   return "$ppl_version @ ".($ppl_path//"system");
}
