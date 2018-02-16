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

use strict;

do "client.pl";

if (readlink("/proc/self/fd/0") eq "/dev/null") {
   push @ARGV, "</dev/null";
}
if (readlink("/proc/self/fd/1") eq "/dev/null") {
   push @ARGV, ">/dev/null";
}
if (readlink("/proc/self/fd/2") eq "/dev/null") {
   push @ARGV, "2>/dev/null";
}

foreach (@ARGV) {
   if (/ / && !/^(['"]).+\1$/) {
      $_="'$_'";
   }
}

my $response=Polymake::HostAgent::call("run", @ARGV);
if ($response =~ /^ERROR/) {
   print STDERR $response;
   exit(1);
} elsif ($response =~ /^(\d+)$/) {
   exit($1 & 127 ? 1 : $1 >> 8);
}
