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

use namespaces;

package Polymake::Core::CPlusPlus;

sub configure_make {
   my $custom_handler=shift;

   $MAKE ||= $^O =~ /bsd|solaris/i ? "gmake" : "make";

   my $cnt=0;
   local $_;
   if (-f "/proc/cpuinfo") {
      # Linux
      if (open my $info, "/proc/cpuinfo") {
         while (<$info>) {
            ++$cnt if /^processor\s*:/i;
         }
      }
   } elsif (-x "/usr/sbin/system_profiler") {
      # MacOS
      if (open my $info, "/usr/sbin/system_profiler -detailLevel mini SPHardwareDataType |") {
         while (<$info>) {
            if (/number of cpus:\s*(\d+)/i) {
               $cnt=$1; last;
            } elsif (/total number of cores:\s*(\d+)/i) {
               $cnt=$1; last;
            }
         }
      }
   } elsif (-x "/sbin/sysctl") {
      # FreeBSD
      if (open my $info, "/sbin/sysctl hw.ncpu |") {
         while (<$info>) {
            if (/^hw\.ncpu:\s+(\d+)/) {
               $cnt=$1; last;
            }
         }
      }
   } elsif (-x "/usr/sbin/psrinfo") {
      # Solaris
      if (open my $info, "/usr/sbin/psrinfo |") {
         while (<$info>) {
            ++$cnt if /on-line/;
         }
      }
   }
   if ($cnt>1) {
      $MAKEFLAGS="-j$cnt";
   } else {
      $MAKEFLAGS="";
   }

   $custom_handler->set($_) for qw($MAKE $MAKEFLAGS);
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
