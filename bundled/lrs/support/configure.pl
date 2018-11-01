#  Copyright (c) 1997-2018
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

@conf_vars=qw( UseBundled CFLAGS LDFLAGS LIBS );

sub allowed_options {
   my ($allowed_options, $allowed_with)=@_;
   @$allowed_with{ qw( lrs lrs-include ) }=();
}


sub usage {
   print STDERR "  --with-lrs=PATH  Installation path of lrslib, if non-standard.\n",
                "                   By default, polymake will try to use a system-wide\n",
                "                   installation or fall back to the bundled lrslib\n",
                "                   (bundled/lrs/external/lrs) if it exists.\n",
                "                   To force the bundled version, specify 'bundled' as PATH.\n",
                "  --with-lrs-include=PATH  Path to the folder containing lrslib.h\n";
}

sub check_bundled {
   -e "bundled/lrs/external/lrs/lrslib.h"
}

sub proceed {
   my ($options)=@_;
   my $lrs_path;
   my $lrs_inc = $options->{'lrs-include'};
   my $lrs_version;
   my $lrs_libname = "lrs";
   $UseBundled = 1;
   # suppress lrs output when it was built without LRS_QUIET
   # 1: use /dev/null as output stream instead of nullptr
   # 2: also redirect stdout because of gmp v.?.?
   # we check whether solving an unbounded lp produces output
   my $suppress_output=0;


   if (defined ($lrs_path=$options->{lrs}) and $lrs_path ne "bundled") {
      my $lrs_lib = Polymake::Configure::get_libdir($lrs_path, $lrs_libname);
      unless (-f "$lrs_lib/lib$lrs_libname.$Config::Config{so}" ||
              -f "$lrs_lib/lib$lrs_libname.a") {
         # retry for legacy lrsgmp
         $lrs_libname = "lrsgmp";
         $lrs_lib = Polymake::Configure::get_libdir($lrs_path, $lrs_libname)
      }

      $lrs_inc //= "$lrs_path/include";
      if (-f "$lrs_inc/lrslib/lrslib.h") {
         $lrs_inc = "$lrs_inc/lrslib";
      } elsif (!-f "$lrs_inc/lrslib.h") {
         die "Invalid installation location of lrslib: header file lrslib.h not found\n";
      }

      if (-f "$lrs_lib/lib$lrs_libname.$Config::Config{so}" ) {
         $LDFLAGS = "-L$lrs_lib -Wl,-rpath,$lrs_lib";
      } elsif (-f "$lrs_lib/lib$lrs_libname.a" ) {
         $LDFLAGS = "-L$lrs_lib";
      } else {
         die "Invalid installation location of lrslib: library lib{lrs,lrsgmp}.$Config::Config{so} / lib{lrs,lrsgmp}.a not found\n";
      }
   }

   if ($lrs_path ne "bundled") {
      my $testcode = <<'---';
#include <cstddef>
#include <iostream>
#include <gmp.h>
#define GMP
#define MA
extern "C" {
   #include <lrslib.h>
}

int main (int argc, char *argv[])
{
   if (!lrs_mp_init(0,NULL,stdout)) {
      std::cout << "ERROR: failed to initialize lrslib via lrs_mp_init" << std::endl;
      exit(1);
   }
   std::cout << "version " << VERSION << std::endl;

   lrs_dat *Q = lrs_alloc_dat("LRS globals");
   if (Q == NULL) {
      std::cout << "ERROR: failed to allocate lrslib globals via lrs_alloc_dat" << std::endl;
      exit(1);
   }
   Q->m=1; Q->n=2; Q->lponly=TRUE;
   lrs_dic *P = lrs_alloc_dic(Q);
   if (Q == NULL) {
      std::cout << "ERROR: failed to allocate lrslib dictionary via lrs_alloc_dic" << std::endl;
      exit(1);
   }

   long den[2] = {1,1};
   long num[2] = {0,1};
   lrs_set_row(P,Q,1,num,den,GE);

   lrs_set_obj(P,Q,num,den,MAXIMIZE);

   if (!lrs_solve_lp(P,Q))
      std::cout << "ERROR: failed to solve lp" << std::endl;

   lrs_free_dic (P,Q);
   lrs_free_dat (Q);
   exit(0);
}
---

RETRY:
      $CFLAGS .=" -I$lrs_inc" if defined($lrs_inc);

      my $error=Polymake::Configure::build_test_program($testcode, LIBS => "-l$lrs_libname -lgmp", CXXFLAGS => "$CFLAGS", LDFLAGS => "$LDFLAGS");

      if ($error =~ /cannot find -llrs/ && $lrs_libname eq "lrs") {
         $lrs_libname = "lrsgmp";
         goto RETRY;
      }

      if ($?==0) {
         my $message=Polymake::Configure::run_test_program();
         if ($?) {
            check_bundled() and !defined($lrs_path) or
               die "Could not run a test program checking for lrs library.\n",
                   "The complete error log follows:\n\n$message\n",
                   "Please investigate the reasons and fix the installation.\n";
         } else {
            my ($lrsver) = $message =~ /version v\.([0-9.]+)[a-z]? [0-9.]+/;
            if (Polymake::Configure::v_cmp($lrsver,"5.1") >= 0) {
               $suppress_output = 1
                  if ($message =~ /\*Unbounded solution/);
               $suppress_output = 2
                  if ($message =~ / gmp v\.\d+\.\d+/);
               $UseBundled = 0;
               $lrs_version = $lrsver;
            } else {
               check_bundled() and !defined($lrs_path) or
                  die "Your lrslib version $lrsver is too old, at least version 5.1 is required.\n";
            }
         }
      } else {
         if ($error =~ /lrslib\.h/m) {
            (my $newcode = $testcode) =~ s#<lrslib.h>#<lrslib/lrslib.h>#g;
            open my $source, "echo '$newcode' | $Polymake::Configure::CXX $CFLAGS -xc++ -E - 2>/dev/null |"
               or die "This looks like recent lrslib with lrslib.h in a lrslib subfolder but we could not\n",
                      "run the preprocessor to find the lrslib include path '$Polymake::Configure::CXX $CFLAGS -xc++ -E -': $!\n",
                      "You can try specifying --with-lrs-include";
            while (<$source>) {
               if (m{\# \d+ "(\S+)/lrslib\.h"}) {
                  $CFLAGS .= " -I$1";
                  close $source;
                  goto RETRY;
               }
            }
            close $source;
         }
         check_bundled() and !defined($lrs_path) or
            die "Could not compile a test program checking for lrs library.\n",
                "The most probable reasons are that the library is installed at a non-standard location,\n",
                "is not configured to build a shared module, or missing at all.\n",
                "The complete error log follows:\n\n$error\n",
                "Please install the library and specify its location using --with-lrs option, if needed.\n",
                "Please remember to enable shared modules when configuring the lrs library!\n";
      }
   }

   if ($UseBundled) {
      die "bundled lrs requested but it cannot be found" 
         if (! -e "bundled/lrs/external/lrs/lrslib.h");
      undef $LIBS;
      $CFLAGS='-I${root}/bundled/lrs/external/lrs';
   } else {
      $LIBS="-l$lrs_libname";
      $CFLAGS.=" -DPM_LRS_SUPPRESS_OUTPUT=$suppress_output" if $suppress_output;
   }

   return $UseBundled ? "bundled" : ("$lrs_version @ ".($lrs_path//"system"));

}
