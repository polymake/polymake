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

@make_vars=qw( BundledLrs CXXflags LDflags LrsCflags LrsLib );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( lrs ) }=();
}


sub usage {
   print STDERR "  --with-lrs=PATH  Installation path of lrslib, if non-standard.\n",
                "                   Uses the bundled lrslib (bundled/lrs/external/lrs) if it exists\n",
                "                   and either no path or 'bundled' is given as option.\n";
}

sub check_bundled {
   -e "bundled/lrs/external/lrs/lrslib.h"
}

sub proceed {
   my ($options)=@_;
   my $lrs_path;
   my $lrsversion;
   $BundledLrs = "yes";

   if (defined ($lrs_path=$options->{lrs}) and $lrs_path ne "bundled") {
      my $lrs_inc="$lrs_path/include";
      my $lrs_lib=Polymake::Configure::get_libdir($lrs_path, "lrs");
      if (-f "$lrs_inc/lrslib.h" && -f "$lrs_lib/liblrsgmp.$Config::Config{so}" ) {
         $CXXflags="-I$lrs_inc";
         $LDflags="-L$lrs_lib -Wl,-rpath,$lrs_lib";
      } elsif (-f "$lrs_inc/lrslib.h" && -f "$lrs_lib/liblrsgmp.a" ) {
         $CXXflags="-I$lrs_inc";
         $LDflags="-L$lrs_lib";
      } else {
         die "Invalid installation location of lrslib: header file lrslib.h and/or library liblrsgmp.$Config::Config{so} / liblrsgmp.a not found\n";
      }
      $LrsCflags = $CXXflags;
   }

   if ($lrs_path ne "bundled") {
      my $error=Polymake::Configure::build_test_program(<<'---', Libs => "-llrsgmp -lgmp", CXXflags => "$CXXflags", LDflags => "$LDflags");
#include <iostream>
#include <gmp.h>
#define GMP
extern "C" {
   #include <lrslib.h>
}
#undef GMP

int main() {
  lrs_mp_init(0,NULL,NULL);
  std::cout << "version " << VERSION << std::endl;
  return 0;
}
---
      if ($?==0) {
         my $message=Polymake::Configure::run_test_program();
         if ($?) {
            check_bundled() and !defined($lrs_path) or
               die "Could not run a test program checking for lrs library.\n",
                   "The complete error log follows:\n\n$message\n",
                   "Please investigate the reasons and fix the installation.\n";
         } else {
            my ($lrsver) = $message =~ /version v\.([0-9.]+)[a-z]? [0-9.]+/;
            if (Polymake::Configure::v_cmp($lrsver,"5.1") < 0) {
               check_bundled() and !defined($lrs_path) or
                  die "Your lrslib version $lrsver is too old, at least version 5.1 is required.\n";
            } else {
               $BundledLrs = undef;
               $lrsversion = $lrsver;
            }
         }
      } else {
         check_bundled() and !defined($lrs_path) or
            die "Could not compile a test program checking for lrs library.\n",
                "The most probable reasons are that the library is installed at a non-standard location,\n",
                "is not configured to build a shared module, or missing at all.\n",
                "The complete error log follows:\n\n$error\n",
                "Please install the library and specify its location using --with-lrs option, if needed.\n",
                "Please remember to enable shared modules when configuring the lrs library!\n";
      }
   }

   if ($BundledLrs eq "yes") {
      die "bundled lrs requested but it cannot be found" 
         if (! -e "bundled/lrs/external/lrs/lrslib.h");
      $CXXflags='-I$(ExtensionTop)/external/lrs';
      $LrsLib='${BuildDir}/../../bundled/lrs/staticlib/lrs/liblrsgmp%A';
   } else {
      $LrsLib="-llrsgmp";
   }
   $LrsCflags = $CXXflags;

   return $BundledLrs eq "yes" ?
            "bundled" :
            ("$lrsversion @ ".($lrs_path//"system"));

}
