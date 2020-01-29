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

#  Extracts all public symbols from application shared modules
#  and generates a source file pretending to define them all.

use strict;
use Config;

my @out;
my $nmopts= $^O eq "darwin" ? "-Ugp" : "--defined-only --extern-only -p";

for my $shlib (@ARGV) {
   -r $shlib or die "shared module $shlib does not exist or unreadable\n";

   open SYMS, "nm $nmopts $shlib |"
     or die "can't run nm $shlib: $!\n";

   my ($appname)= $shlib =~ m{(?:^|/)(\w+)\.$Config::Config{dlext}$}
     or die "can't derive application name from shared module name $shlib\n";

   my $prefix="8polymake".length($appname).$appname;

   while (<SYMS>) {
      if (/ [TW] ([_ZNK]+$prefix\w+)$/) {
	  if ( $^O eq "darwin" ) {    # aliases don't seem to work on MacOS, so we actually define the functions with empty body
	      my $functionname = $1;
	      $functionname =~ s/^__/_/;
	      push @out, "void $functionname() {};\n";
	  } else {
	      push @out, "void $1() __attribute__((alias(\"__dummy\")));\n"; 
	  }
      }
   }
   close SYMS;
}

if (@out) {
   if ($^O eq "darwin") {
      print "#ifndef POLYMAKE_FAKE_FUNCTIONS\n";
   }
   print <<'.';
void __dummy() __attribute__((visibility ("hidden")));
void __dummy() { }
.
   if ($^O eq "darwin") {
      print "#endif\n";
   }
   print "#ifdef POLYMAKE_FAKE_FUNCTIONS\n", @out, "#endif\n";
} else {
   warn "no external symbols found!\n";
   exit(1);
}
