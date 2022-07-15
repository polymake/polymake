#  Copyright (c) 1997-2022
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
   @$allowed_with{ qw( sympol sympol-include sympol-lib ) }=();
}

sub usage {
   print STDERR "  --with-sympol=PATH   Installation path of sympol, if non-standard.\n",
                "                       By default, polymake will try to use a system-wide\n",
                "                       installation or fall back to the bundled sympol\n",
                "                       (bundled/sympol/external/sympol) if it exists.\n",
                "                       To force the bundled version, specify 'bundled' as PATH.\n",
                "  --with-sympol-include=PATH  Path to the folder containing symmetrycomputation.h\n",
                "  --with-sympol-lib=PATH      Path to the folder containing libsympol.{a,so,dylib}\n";

}

sub check_bundled {
   -e "bundled/sympol/external/sympol/sympol/symmetrycomputation.h"
}

sub proceed {
   my ($options)=@_;
   my $sympol_path;
   $UseBundled = 1;

   if (defined ($sympol_path=$options->{sympol}) and $sympol_path ne "bundled") {
      if (-f "$sympol_path/include/sympol/symmetrycomputation.h") {
         $CXXFLAGS = "-I$sympol_path/include";
      }
      my $sympol_lib=Polymake::Configure::get_libdir($sympol_path, "sympol");
      if (-f "$sympol_lib/libsympol.$Config::Config{so}" ) {
         $LDFLAGS = "-L$sympol_lib";
         $LDFLAGS .= " -Wl,-rpath,$sympol_lib" if $sympol_path ne "/usr";
      }
      if (!$CXXFLAGS or !$LDFLAGS) {
         die "Invalid installation location of sympol: header file symmetrycomputation.h and/or library libsympol.$Config::Config{so} not found.\n",
             "You might try to use --with-sympol-include and --with-sympol-lib.\n";

      }
   }
   if (defined (my $sympol_inc=$options->{'sympol-include'})) {
      $CXXFLAGS .=" -I$sympol_inc";
      $sympol_path .= "include: $sympol_inc ";
   }
   if (defined ($sympol_lib=$options->{'sympol-lib'})) {
      $LDFLAGS = " -L$sympol_lib";
      $LDFLAGS .= " -Wl,-rpath,$sympol_lib" if $sympol_lib !~ m{^/usr/lib};
      $sympol_path .= "lib: $sympol_lib";
   }

   if ($Polymake::Bundled::lrs::UseBundled or $Polymake::Bundled::cdd::UseBundled or $Polymake::Configure::ExternalHeaders =~ /permlib/) {
      if (defined($sympol_path) and $sympol_path ne "bundled") {
         die "Using a non-bundled sympol configuration requires non-bundled lrs, cdd and PermLib as well.\n",
             "Please use --with-cdd=PATH, --with-lrs=PATH and --with-permlib=PATH or remove the --with-sympol option.\n";
      }
      $sympol_path = "bundled";
   }

   if ($sympol_path ne "bundled") {
      my $testcode = <<"---";
#include <cstddef>
#include <iostream>
#include <gmpxx.h>
#include <sympol/polyhedron.h>

int main() {
   boost::shared_ptr<sympol::PermutationGroup> pg;
   return 0;
}
---
      my $error=Polymake::Configure::build_test_program($testcode, LIBS => "-lsympol", CXXFLAGS => "$CXXFLAGS", LDFLAGS => "$LDFLAGS");
      if ($?==0) {
         my $message=Polymake::Configure::run_test_program();
         if ($?) {
            check_bundled() and !defined($sympol_path) or 
               die "Could not run a test program checking for sympol library.\n",
                   "The complete error log follows:\n\n$message\n",
                   "Please investigate the reasons and fix the installation.\n";
         } else {
            $UseBundled = 0;
         }
      } else {
         check_bundled() and !defined($sympol_path) or 
            die "Could not compile a test program checking for sympol.\n",
                "The most probable reasons are that the library is installed at a non-standard location,\n",
                "is not configured to build a shared module, or missing at all.\n",
                "The complete error log follows:\n\n$error\n",
                "Please install the library and specify its location using --with-sympol option, if needed.\n",
      }
   } 


   if ($UseBundled) {
      Polymake::Configure::check_gmpxx();

      Polymake::Configure::check_gmpxx_ostream();
   }

   if ($UseBundled) {
      die "bundled sympol requested but it cannot be found" 
         unless check_bundled();
      undef $LIBS;
      $CXXFLAGS = '-I${root}/bundled/sympol/external/sympol';
   } else {
      $LIBS="-lsympol";
   }

   if ($Polymake::Bundled::ppl::LIBS) {
      $CXXFLAGS .= " -DPOLYMAKE_WITH_PPL";
   }

   return $UseBundled ? "bundled" : ($sympol_path//"system");

}
