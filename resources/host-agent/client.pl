#  Copyright (c) 1997-2022
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

package Polymake::HostAgent;

use strict;
use Socket;
use IO::Handle;

my $socketfile=$ENV{POLYMAKE_HOST_AGENT};

sub call {
   my $response="";
   if (defined($socketfile) &&
       defined(my $socket=connect_to_server())) {
      $socket->autoflush;
      print $socket "@_\n";
      while (read $socket, $response, 1024, length($response)) { }
      close $socket;

   } else {
      my $error= defined($socketfile)
         ? "Connection to socket $socketfile failed: $!\n"
	 : "Polymake host agent did not start.\n";
      $error .= "Running programs outside the polymake container is disabled for this session\n.";
      if ($_[0] eq "run") {
	 $response="ERROR: $error";
      } else {
	 delete $ENV{POLYMAKE_HOST_AGENT};
	 print STDERR $error;
      }
      *call=sub { };
   }

   $response
}

sub connect_to_server {
   my $socket;
   if (-l $socketfile) {
      my ($ip, $port)=split /:/, readlink($socketfile);
      socket($socket, AF_INET, SOCK_STREAM, PF_UNSPEC) and
      connect($socket, sockaddr_in($port, inet_aton($ip))) and
      return $socket;
   } elsif (-S $socketfile) {
      socket($socket, AF_UNIX, SOCK_STREAM, PF_UNSPEC) and
      connect($socket, sockaddr_un($socketfile)) and
      return $socket;
   }
   undef
}

1
