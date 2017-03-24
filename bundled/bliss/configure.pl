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
   @$allowed_with{ qw( bliss ) }=();
}

sub usage {
   print STDERR "  --with-bliss=PATH  installation path of bliss library, if non-standard\n";
}

sub proceed {
   my ($options)=@_;
   my $bliss_path;
   if (defined ($bliss_path=$options->{bliss})) {
      my $bliss_inc="$bliss_path/include";
      my $bliss_lib=Polymake::Configure::get_libdir($bliss_path, "bliss");
      if (-f "$bliss_inc/bliss/graph.hh" && -f "$bliss_lib/libbliss.$Config::Config{dlext}" ) {
         $CXXflags="-I$bliss_inc";
         $LDflags="-L$bliss_lib";
         $LDflags.=" -Wl,-rpath,$bliss_lib" unless $bliss_path =~ m|^/usr$|;
      } elsif (-f "$bliss_inc/bliss/graph.hh" && -f "$bliss_lib/libbliss.a" ) {
         $CXXflags="-I$bliss_inc";
         $LDflags="-L$bliss_lib";
      } elsif (-f "$bliss_path/graph.hh" && -f "$bliss_path/libbliss.a") {
      	$CXXflags="-I$bliss_path/..";
      	$LDflags="-L$bliss_path";
      } else {
         die "Invalid installation location of bliss library: header file bliss/graph.hh and/or library libbliss.$Config::Config{dlext} / libbliss.a not found\n";
      }

   }
   my $testfile = <<'---';
#include "bliss/graph.hh"
#include <stdio.h>
int main() {
  bliss::Graph g1(5);
  bliss::Stats stats;
  g1.find_automorphisms(stats,NULL,NULL);
  stats.print(stdout);
  bliss::Graph g2(5);
  g2.add_edge(0,1);
  return !g1.cmp(g2);
}
---
   RETRY:
   # the gmp libary needs to be here because some (old) ubuntu bliss packages are not linked to gmp
   # it is also needed if we try with the gmp flag
   my $error=Polymake::Configure::build_test_program($testfile, CXXflags => $CXXflags, LDflags => $LDflags, Libs => "-lbliss -lgmp");
   if ($?==0) {
      $error=Polymake::Configure::run_test_program();
      # there does not seem to be a way to determine whether bliss was built with BLISS_USE_GMP
      # but this determines whether some internal variables (Stats::group_size) are gmp based or double
      # if printing the number of automorphisms of 5 disjoint nodes segfaults or
      # gives something other than 120 we retry with the flag
      if ($? or $error !~ /Aut.*120$/m) {
         $CXXflags .= " -DBLISS_USE_GMP" and goto RETRY
            unless $CXXflags =~ /-DBLISS_USE_GMP/;
         die "Could not run a test program checking for bliss library.\n",
             "The complete error log follows:\n\n$error\n",
             "Please investigate the reasons and fix the installation.\n";
      }
   } else {
      die "Could not compile a test program checking for bliss library.\n",
          "The most probable reasons are that the library is installed at a non-standard location,\n",
          "is not configured to build a shared module, or missing at all.\n",
          "The complete error log follows:\n\n$error\n",
          "Please install the library and specify its location using --with-bliss option, if needed.\n",
          "Please remember to enable shared modules when configuring the bliss library!\n\n",
          "If you are running a Debian system and now see some missing GMP functions in the error message,\n",
          "then you've got an old broken bliss package;  please upgrade it to the latest version.\n";
   }

   $Libs="-lbliss";
   return $bliss_path;
}
