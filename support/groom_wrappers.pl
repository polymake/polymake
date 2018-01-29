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

# Evolution of automatically generated C++/perl glue code

# The main function of this script is called from generate_ninja_targets.pl
# for every `perl' subdirectory of the source tree which might contain
# automatically generated wrappers.

use strict;

# Epoch 0001:
#  - fix the URL in the copyright note
#  - put everything in anonymous namespace
#  - delete "change guards" with perl::Anything

sub epoch_0001 {
   my $changed=0;
   $changed |= $_[0] =~ s/www\.polymake\.de/www.polymake.org/;
   $changed |= ( $_[0] =~ s/^(\s*namespace\s+polymake\s*\{\s*namespace\s+\w+\s*\{)(\s*$)/$1 namespace {$2/m
		 and
		 $_[0] =~ s/^(\s*\}\s*\})(\s*$)/$1 }$2/m );
   $changed |= $_[0] =~ s/^.*\bperl::Anything\b.*\n//mg;
}

my @chronology=( \&epoch_0001 );

my $current_marker=sprintf ".wrappers_v%04d", scalar @chronology;

sub groom_wrappers {
   my ($dir, @files)=@_;
   return if -f "$dir/$current_marker";

   # find the oldest epoch to start with
   my ($old_marker)=glob("$dir/.wrappers_v*");
   my ($epoch)= $old_marker =~ /_v(\d+)$/;
   $epoch //= 0;

   for my $file (@files) {
      open my $in, $file
        or die "can't read source file $file: $!\n";
      local $/;
      my $text=<$in>;
      close $in;
      my $changed=0;
      for (@chronology[$epoch..$#chronology]) {
	 $changed |= $_->($text);
      }
      if ($changed) {
	 open my $out, ">", "$file.new"
	   or die "can't create new file $file.new: $!\n";
	 print $out $text;
	 close $out;
	 rename "$file.new", $file
	   or die "can't rename $file.new into $file: $!\n";
      }
   }

   open my $touch, ">", "$dir/$current_marker"
   or die "can't create a timestamp file $dir/$current_marker: $!\n";
   close $touch;
   if ($old_marker) {
      unlink $old_marker;
   }
}

1
