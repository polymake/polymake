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

@make_vars=qw( BundledCdd CXXflags LDflags CddCflags CddLib );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( cdd cdd-include cdd-lib) }=();
}


sub usage {
   print STDERR "  --with-cdd=PATH  Installation path of cddlib, if non-standard.\n",
                "                   Without this option polymake will try to use a systemwide\n",
                "                   installation or fall back to the bundled cddlib \n",
                "                   (bundled/cdd/external/cdd) if it exists.\n",
                "                   To force the bundled version 'bundled' can be given as option.\n",
                "  --with-cdd-include=PATH  Path to the folder containing cdd.h.  \n",
                "  --with-cdd-lib=PATH      Path to the folder containing libcddgmp.{a,so,dylib} \n";
}

sub check_bundled {
   -e "bundled/cdd/external/cdd/lib-src-gmp/cdd.h"
}

sub proceed {
   my ($options)=@_;
   my $cdd_path;
   my $cddversion;
   $BundledCdd = "yes";

   if (defined ($cdd_path=$options->{cdd}) and $cdd_path ne "bundled") {
      my $cdd_inc="$cdd_path/include";
      if (-f "$cdd_inc/cdd.h") {
         $CXXflags = "-I$cdd_inc";
      } elsif (-f "$cdd_inc/cdd/cdd.h") {
         # This is for debian where the includes are moved to a subdirectory
         $CXXflags = "-I$cdd_inc/cdd";
      }
      my $cdd_lib=Polymake::Configure::get_libdir($cdd_path, "cddgmp");
      if (-f "$cdd_lib/libcddgmp.$Config::Config{so}" ) {
         $LDflags = "-L$cdd_lib";
         $LDflags .= " -Wl,-rpath,$cdd_lib" unless ($cdd_path eq "/usr");
      }
      if (!$CXXflags or !$LDflags) {
         die "Invalid installation location of cddlib: header file cdd.h and/or library libcddgmp.$Config::Config{so} not found.\n",
             "You might try to use --with-cdd-include and --with-cdd-lib.\n";
      }
   }

   if (defined (my $cdd_inc=$options->{'cdd-include'})) {
      $CXXflags="-I$cdd_inc";
      $cdd_path .= "include: $cdd_inc ";
   }
   if (defined ($cdd_lib=$options->{'cdd-lib'})) {
      $LDflags = "-L$cdd_lib -Wl,-rpath,$cdd_lib";
      $cdd_path .= "lib: $cdd_lib";
   }

   
   if ($cdd_path ne "bundled") {
      my $testcode = <<"---";
#include <iostream>
#include <gmp.h>

#define GMPRATIONAL
extern "C" {
   #include <setoper.h>
   #include <cdd.h>
}

int main() {
   std::cout << "dd " << dd_DDVERSION << std::endl;
   std::cout << "ddf " << ddf_DDVERSION << std::endl;
   dd_set_global_constants();
   dd_free_global_constants();
   ddf_set_global_constants();
   ddf_free_global_constants();
   return 0;
}
---
      my $error=Polymake::Configure::build_test_program($testcode, Libs => "-lcddgmp -lgmp", CXXflags => "$CXXflags", LDflags => "$LDflags");
      if ($? != 0) {
         # if this failed then we try to determine whether the includes from cdd are in a subfolder
         # (like on debian)
         (my $newcode = $testcode) =~ s{setoper\.h|cdd\.h}{cdd/$&}g;
         $error = Polymake::Configure::build_test_program($newcode, CXXflags => "$CXXflags", Libs => "-lcddgmp -lgmp", LDflags => "$LDflags");
         if ($? == 0) {
            # run just the preprocessor and parse the output to find the path of cdd.h
            open my $source, "echo '$newcode' | $Polymake::Configure::CXX $CXXflags -xc++ -E - 2>/dev/null |"
               or die "This looks like debian with cdd.h in a cdd subfolder but we could not\n",
                      "run the preprocessor to find the cdd include path '$Polymake::Configure::CXX $CXXflags -xc++ -E -': $!\n",
                      "You can try specifying --with-cdd-include and --with-cdd-lib";
            while (<$source>) {
               if (m{\# \d+ "(\S+)/cdd\.h"}) {
                  $CXXflags .= " -I$1";
                  last;
               }
            }
            close $source;
         }
      }
      $error=Polymake::Configure::build_test_program($testcode, Libs => "-lcddgmp -lgmp", CXXflags => "$CXXflags", LDflags => "$LDflags");
      if ($?==0) {
         my $message=Polymake::Configure::run_test_program();
         if ($?) {
            check_bundled() and !defined($cdd_path) or 
               die "Could not run a test program checking for cdd library.\n",
                   "The complete error log follows:\n\n$message\n",
                   "Please investigate the reasons and fix the installation.\n";
         } else {
            my ($ddver,$ddfver) = $message =~ /dd Version ([0-9a-z.]+) .*\nddf Version ([0-9a-z.]+) /;
            if ($ddver ne $ddfver) {
               check_bundled() and !defined($cdd_path) or 
                  die "Your cddlib installation does not seem to contain the floating-point arithmetic version.\n",
                      "Reported versions: dd_DDVERSION=$ddver and ddf_DDVERSION=$ddfver\n";
            } elsif (($ddver cmp "0.94f") >= 0) {
               $BundledCdd = undef;
               $cddversion = $ddver;
            } else {
               check_bundled() and !defined($cdd_path) or
                  die "Your cddlib version $ddver is too old, at least version 0.94f is required.\n";
            }
         }
      } else {
         check_bundled() and !defined($cdd_path) or 
            die "Could not compile a test program checking for cdd library.\n",
                "The most probable reasons are that the library is installed at a non-standard location,\n",
                "is not configured to build a shared module, or missing at all.\n",
                "The complete error log follows:\n\n$error\n",
                "Please install the library and specify its location using --with-cdd option, if needed.\n",
                "Please remember to enable shared modules when configuring the cdd library!\n";
      }
   }

   if ($BundledCdd eq "yes") {
      die "bundled cdd requested but it cannot be found" 
         unless (-e "bundled/cdd/external/cdd/lib-src-gmp/cdd.h");
      $CXXflags='-I$(ExtensionTop)/external/cdd/lib-src-gmp';
      # Up to the main BuildDir to descent into the right extension
      # Mainly for the group extension.
      $CddLib='${BuildDir}/../../bundled/cdd/staticlib/cdd/libcddgmp%A';
   } else {
      $CddLib="-lcddgmp";
   }
   $CddCflags="$CXXflags";

   return $BundledCdd eq "yes" ? 
            "bundled" : 
            ("$cddversion @ ".($cdd_path//"system"));
}

