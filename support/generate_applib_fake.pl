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

#  Extracts all public symbols from application shared modules
#  and generates a source file pretending to define them all.

use strict;

die "usage: $0 STUB_OUTFILE FAKE_OUTFILE SHARED_OBJECT ... \n" if @ARGV<3;

my @out;
my $nmopts= $^O eq "darwin" ? "-Ugp" : "--defined-only --extern-only -p";

for my $shlib (splice @ARGV, 2) {
   -r $shlib or die "shared library $shlib does not exist or unreadable\n";

   open SYMS, "nm $nmopts $shlib |"
     or die "can't run nm $shlib: $!\n";

   my ($appname)= $shlib =~ m{(?:^|/)(\w+)(?:-\w+)?\.[^/]+$}
     or die "can't derive application name from shared library name $shlib\n";

   my $prefix="8polymake".length($appname).$appname;

   while (<SYMS>) {
      if (/ T ([_ZNK]+$prefix\w+)$/) {
	  if ( $^O eq "darwin" ) {    # aliases don't seem to work on MacOS, so we actually define the functions with emtpy body
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
   open STUB, ">", $ARGV[0]
     or die "can't create output file $ARGV[0]: $!\n";
   open FAKE, ">", $ARGV[1]
     or die "can't create output file $ARGV[1]: $!\n";

   my $dummy= <<'.';
void __dummy() __attribute__((visibility ("hidden")));
void __dummy() { }
.
    
    print STUB $dummy;

if ( $^O eq "darwin" ) {  # we define proper functions on MacOS, not aliases, so no dummy needed 
    print FAKE @out;
} else {
    print FAKE $dummy, @out;
}

   close STUB;
   close FAKE;
} else {
   warn "no external symbols found, output suppressed!\n";
   exit(1);
}
