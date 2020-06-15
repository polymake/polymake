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

@conf_vars=qw( UseBundled CFLAGS LDFLAGS LIBS );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( cdd cdd-include cdd-lib) }=();
}


sub usage {
   print STDERR "  --with-cdd=PATH  Installation path of cddlib, if non-standard.\n",
                "                   By default, polymake will try to use a system-wide\n",
                "                   installation or fall back to the bundled cddlib\n",
                "                   (bundled/cdd/external/cdd) if it exists.\n",
                "                   To force the bundled version, specify 'bundled' as PATH.\n",
                "  --with-cdd-include=PATH  Path to the folder containing cdd.h\n",
                "  --with-cdd-lib=PATH      Path to the folder containing libcddgmp.{a,so,dylib} \n";
}

sub check_bundled {
   -e "bundled/cdd/external/cdd/lib-src-gmp/cdd.h"
}

sub proceed {
   my ($options)=@_;
   my $cdd_path;
   my $cddversion;
   $UseBundled = 1;

   if (defined ($cdd_path=$options->{cdd}) and $cdd_path ne "bundled") {
      my $cdd_inc="$cdd_path/include";
      if (-f "$cdd_inc/cdd.h") {
         $CFLAGS = "-I$cdd_inc";
      } elsif (-f "$cdd_inc/cdd/cdd.h") {
         # This is for debian where the includes are moved to a subdirectory
         $CFLAGS = "-I$cdd_inc/cdd";
      } elsif (-f "$cdd_inc/cddlib/cdd.h") {
         # This is for fedora where the includes are moved to a subdirectory
         $CFLAGS = "-I$cdd_inc/cddlib";
      }
      my $cdd_lib=Polymake::Configure::get_libdir($cdd_path, "cddgmp");
      if (-f "$cdd_lib/libcddgmp.$Config::Config{so}" ) {
         $LDFLAGS = "-L$cdd_lib";
         $LDFLAGS .= " -Wl,-rpath,$cdd_lib" if $cdd_path ne "/usr";
      }
      if (!$CFLAGS or !$LDFLAGS) {
         die "Invalid installation location of cddlib: header file cdd.h and/or library libcddgmp.$Config::Config{so} not found.\n",
             "You might try to use --with-cdd-include and --with-cdd-lib.\n";
      }
   }

   if (defined (my $cdd_inc=$options->{'cdd-include'})) {
      $CFLAGS = "-I$cdd_inc";
      $cdd_path .= "include: $cdd_inc ";
   }
   if (defined ($cdd_lib=$options->{'cdd-lib'})) {
      $LDFLAGS = "-L$cdd_lib -Wl,-rpath,$cdd_lib";
      $cdd_path .= "lib: $cdd_lib";
   }

   
   if ($cdd_path ne "bundled") {
      my $testcode = <<"---";
#include <cstddef>
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
      my $error=Polymake::Configure::build_test_program($testcode, LIBS => "-lcddgmp -lgmp", CXXFLAGS => "$CFLAGS", LDFLAGS => "$LDFLAGS");
      if ($? != 0) {
         # if this failed then we try to determine whether the includes from cdd are in a subfolder
         # (like on debian or fedora)
         my $newcode;
         foreach my $subdir (qw(cdd cddlib)) {
            ($newcode = $testcode) =~ s{setoper\.h|cdd\.h}{$subdir/$&}g;
            $error = Polymake::Configure::build_test_program($newcode, CXXFLAGS => "$CFLAGS", LIBS => "-lcddgmp -lgmp", LDFLAGS => "$LDFLAGS");
            last if $? == 0;
         }
         if ($? == 0) {
            # run just the preprocessor and parse the output to find the path of cdd.h
            open my $source, "echo '$newcode' | $Polymake::Configure::CXX $CFLAGS -xc++ -E - 2>/dev/null |"
               or die "This looks like debian or fedora with cdd.h in a subfolder but we could not\n",
                      "run the preprocessor to find the cdd include path '$Polymake::Configure::CXX $CFLAGS -xc++ -E -': $!\n",
                      "You can try specifying --with-cdd-include and --with-cdd-lib";
            while (<$source>) {
               if (m{\# \d+ "(\S+)/cdd\.h"}) {
                  $CFLAGS .= " -I$1";
                  last;
               }
            }
            close $source;
         }
      }
      $error=Polymake::Configure::build_test_program($testcode, LIBS => "-lcddgmp -lgmp", CXXFLAGS => "$CFLAGS", LDFLAGS => "$LDFLAGS");
      if ($?==0) {
         my $message=Polymake::Configure::run_test_program();
         if ($?) {
            check_bundled() and !defined($cdd_path) or 
               die "Could not run a test program checking for cdd library.\n",
                   "The complete error log follows:\n\n$message\n",
                   "Please investigate the reasons and fix the installation.\n";
         } else {
            my ($ddver,$ddfver) = $message =~ /dd Version ([0-9a-z.]+).*\nddf Version ([0-9a-z.]+)/;
            if ($ddver ne $ddfver) {
               check_bundled() and !defined($cdd_path) or 
                  die "Your cddlib installation does not seem to contain the floating-point arithmetic version.\n",
                      "Reported versions: dd_DDVERSION=$ddver and ddf_DDVERSION=$ddfver\n";
            } elsif (($ddver cmp "0.94f") >= 0) {
               $UseBundled = 0;
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

   if ($UseBundled) {
      die "bundled cdd requested but it cannot be found" 
         unless (-e "bundled/cdd/external/cdd/lib-src-gmp/cdd.h");
      undef $LIBS;
      $CFLAGS='-I${root}/bundled/cdd/external/cdd/lib-src-gmp';
   } else {
      $LIBS="-lcddgmp";
   }

   return $UseBundled ? "bundled" : ("$cddversion @ ".($cdd_path//"system"));
}

