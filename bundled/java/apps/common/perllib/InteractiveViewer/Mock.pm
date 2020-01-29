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

#
#  Mock class for testing
#

package Polymake::InteractiveViewer::Mock;

sub simulate {
   my $logfile=shift;
   open my $log, "<", $logfile or die "can't read input log file $logfile: $!\n";
   my $port;
   while (@_ && shift(@_) !~ /SelectorThread$/) { }
   $port=shift or die "could not find port number among arguments\n";

   my @sockets;
   push @sockets, new ClientSocket("localhost", $port);

   local $_;
   local $/="\n";
   while (<$log>) {
      if (s/^(\d+)([<=>]) //) {
         my $socket=$sockets[$1];
         defined($socket) or die "logfile $logfile, line $.: non-existing channel number $1\n";

         if ($2 eq ">") {
            # print something
            print $socket $_ or die "socket $1 closed prematurely\n";

         } elsif ($2 eq "<") {
            # read one line and match against the expression
            chomp;
            my $match=qr/$_/;
            $_=readline $socket;
            if ($_ =~ $match) {
               if (defined ($port=$+{port})) {
                  # open an additonal channel
                  push @sockets, new ClientSocket("localhost", $port);
               }
            } else {
               chomp;
               die "consumed input '$_' did not match the expectation\n";
            }

         } else {
            # read and discard the input until the matching line occurs
            chomp;
            my $match=qr/$_/;
            while (defined($_ = readline $socket) && $_ !~ $match) {
            }
            defined($_) or die "expected string $match did not occur\n";
         }
      } else {
         die "logfile $logfile, line $.: unrecognized command $_\n";
      }
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
