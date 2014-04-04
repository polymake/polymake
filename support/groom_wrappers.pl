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

# Evolution of automatically generated C++/perl glue code

use strict;

# Epoch 0001:
#  - fix the URL in the copyright note
#  - put everything in anonymous namespace
#  - delete "change guards" with perl::Anything

sub wv0001 {
   my $changed=0;
   $changed |= $_[0] =~ s/www\.polymake\.de/www.polymake.org/;
   $changed |= ( $_[0] =~ s/^(\s*namespace\s+polymake\s*\{\s*namespace\s+\w+\s*\{)(\s*$)/$1 namespace {$2/m
		 and
		 $_[0] =~ s/^(\s*\}\s*\})(\s*$)/$1 }$2/m );
   $changed |= $_[0] =~ s/^.*\bperl::Anything\b.*\n//mg;
}

my @chronology=( [ wrappers_v0001 => \&wv0001 ],
	       );

## main function

@ARGV==1 && -d $ARGV[0]
  or die "usage: $0 SOURCE_DIR\n";

# find the oldest epoch to start with

my $epoch;
for ($epoch=0; $epoch <= $#chronology && -f "$ARGV[0]/.".$chronology[$epoch]->[0]; ++$epoch) {}

if ($epoch <= $#chronology) {
   eval {
      my $files=0;
      for my $file (glob "$ARGV[0]/*.{cc,C,cpp}") {
	 open IN, $file
	   or die "can't read source file $file: $!\n";
	 local $/;
	 my $text=<IN>;
	 close IN;
	 my $changed=0;
	 for (@chronology[$epoch..$#chronology]) {
	    $changed |= $_->[1]->($text);
	 }
	 if ($changed) {
	    open OUT, ">", "$file.new"
	      or die "can't create new file $file.new: $!\n";
	    print OUT $text;
	    close OUT;
	    rename "$file.new", $file
	      or die "can't rename $file.new into $file: $!\n";
	 }
	 ++$files;
      }

      if ($files) {
	 my $stamp="$ARGV[0]/.".$chronology[-1]->[0];
	 open TOUCH, ">", $stamp
	   or die "can't create a timestamp file $stamp: $!\n";
	 close TOUCH;
	 if (`svn status $stamp 2>&1` =~ /^\? /) {
	    `svn add $stamp`;
	 }
      }
   };
   if ($@) {
      print "$0: error: $@";
      exit(1);
   }
}
