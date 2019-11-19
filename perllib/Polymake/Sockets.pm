#  Copyright (c) 1997-2019
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

require Polymake::Pipe;

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);

package Polymake::ServerSocket;
use Socket;
use Fcntl;
use POSIX qw(:errno_h);

use Polymake::Struct (
   [ new => ';$' ],
   [ '$handle' => 'undef' ],
   [ '$port' => '#1' ],
);

# Constructor: new ServerSocket([ port ])
sub new {
   my $self=&_new;
   socket $self->handle, PF_INET, SOCK_STREAM, getprotobyname('tcp')
      or die "socket failed: $!\n";
   my $local_ip=$ENV{POLYMAKE_HOST_AGENT} ? INADDR_ANY : INADDR_LOOPBACK;
   if (defined $self->port) {
      bind $self->handle, sockaddr_in($self->port, $local_ip)
         or die "bind failed: $!\n";
   } else {
      my $port;
      for ($port=30000; $port<65536; ++$port) {
         last if bind $self->handle, sockaddr_in($port, $local_ip);
         die "bind failed: $!\n" if $! != EADDRINUSE;
      }
      if ($port==65536) {
         die "bind failed: all ports seem occupied\n";
      }
      $self->port=$port;
   }
   fcntl($self->handle, F_SETFD, FD_CLOEXEC);

   listen $self->handle, 1
      or die "listen failed: $!\n";
   $self;
}

sub translate_port {
   my ($port)=@_;
   if ($port > 0 && $ENV{POLYMAKE_HOST_AGENT}) {
      require "$Polymake::InstallTop/resources/host-agent/client.pl";
      $port=HostAgent::call("port $port");
   }
   $port
}

sub DESTROY {
   close($_[0]->handle);
}

sub fileno { fileno($_[0]->handle) }

sub accept {
   my ($self)=@_;
   accept my $s, $self->handle
      or die "accept failed: $!\n";
   $s->autoflush;
   $s;
}

####################################################################################
package Polymake::ClientSocket;
use Socket;

# Constructor: new ClientSocket(hostname, port) || new ClientSocket("named/socket/path")
sub new {
   shift;
   my $socket;
   if (@_==2) {
      my ($hostname, $port)=@_;
      my $hostaddr=inet_aton($hostname)
        or die "unknown host name $hostname\n";
      socket($socket, AF_INET, SOCK_STREAM, PF_UNSPEC)
        or die "can't create socket: $!\n";
      connect($socket, sockaddr_in($port, $hostaddr))
        or die "can't connect to $hostname: $!\n";
   } elsif (@_==1 && -S $_[0]) {
      socket($socket, AF_UNIX, SOCK_STREAM, PF_UNSPEC)
        or die "can't create socket: $!\n";
      connect($socket, sockaddr_un($_[0]))
        or die "can't connect to named socket $_[0]: $!\n";
   } else {
      die "ClientSocket constructor got neither a remote host:port combination nor a named socket\n";
   }
   $socket->autoflush;
   $socket;
}

####################################################################################
package Polymake::LocalServer;

use Polymake::Struct (
   [ '@ISA' => 'Selector::Member' ],
   [ new => '$$' ],
   [ '$server_socket' => 'undef' ],
   [ '$handler_pkg' => '#2' ],
   [ '$data_conn' => 'undef' ],
);

# Constructor: HandlerPackage [ , port ] =>

sub new {
   my $serv=new ServerSocket(splice @_, 2);
   my $self=Selector::Member::new($_[0], $serv->handle, $_[1]);
   $self->server_socket=$serv;
   $self;
}

sub in_avail {
   my $self=shift;
   $self->data_conn=$self->handler_pkg->new($self->server_socket->accept);
}

sub port { (shift)->server_socket->port }

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
