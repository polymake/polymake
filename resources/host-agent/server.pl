#!/usr/bin/perl
#
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
#  An agent process running in the host namespace (outside any containers)
#  or in a different container than the polymake process.
#  Its purpose is to launch other programs in background like static visualization
#  tools and to assist the rule configuration procedures in exploring the host file system.
#

use strict;
use Socket;
use IO::Handle;
use POSIX qw(:fcntl_h :sys_wait_h);

use Getopt::Long qw( GetOptions :config require_order no_ignore_case );

my ($socketfile, $dockercmd, $cid);
GetOptions( 'socket=s' => \$socketfile, 'docker-cmd=s' => \$dockercmd, 'cid=s' => \$cid )
  && !@ARGV
  && length($socketfile) && length($cid)
or die "usage: $0 --socket SOCKET_FILE --cid CONTAINER_ID [ --docker-cmd 'COMMAND ...' ]\n";

$dockercmd //= "docker";

unlink $socketfile;

my $serv;
if ($^O eq "darwin") {
  # docker on MacOS does not support data exchange between host and container over filesystem sockets yet
  my $port;
  socket($serv, PF_INET, SOCK_STREAM, getprotobyname('tcp')) or die "socket failed: $!\n";
  for ($port=30000; $port<65536; ++$port) {
    last if bind($serv, sockaddr_in($port, INADDR_LOOPBACK));
    die "bind failed: $!\n" unless $!{EADDRINUSE};
  }
  if ($port==65536) {
    die "bind failed: all ports seem occupied\n";
  }
  symlink("docker.for.mac.localhost:$port", $socketfile)
    or die "can't create a symlink $socketfile -> docker.for.mac.localhost:$port : $!\n";
} else {
  socket($serv, PF_UNIX, SOCK_STREAM, 0) or die "socket failed: $!\n";
  bind($serv, sockaddr_un($socketfile)) or die "bind failed: $!\n";
}
listen($serv, 1) or die "listen failed: $!";
POSIX::fcntl($serv, F_SETFD, FD_CLOEXEC);

my ($heartbeat_src, $heartbeat_sink);
pipe $heartbeat_sink, $heartbeat_src;

if (my $pid=fork) {
   # parent process
   close $heartbeat_sink;
   POSIX::fcntl($heartbeat_src, F_SETFD, 0);
   exec "$dockercmd start -i -a $cid";
   die "could not execute $dockercmd: $!\n";
   exit(1);
}

close $heartbeat_src;

$SIG{INT}='IGNORE';
$SIG{TERM}=\&stop;
$SIG{CHLD}=\&collect_children;

my $select_mask="";
vec($select_mask, fileno($heartbeat_sink), 1)=1;
vec($select_mask, fileno($serv), 1)=1;

for (;;) {
   $!=0;
   if (select(my $ready=$select_mask, undef, undef, undef)) {
      # For mysterious reasons, presumably a bug in perl itself,
      # the return value from an interrupted select() is unpredictable,
      # even if no channel is in fact ready.
      # Thus it's safer to check for interrupts in both cases.
      next if $!{EINTR};
      if (vec($ready, fileno($heartbeat_sink), 1)) {
         # container process finished
         last;
      }
      if (vec($ready, fileno($serv), 1) &&
          accept(my $socket, $serv)) {
         $socket->autoflush;
         $_=<$socket>;
         chomp;
         if (s/^run //) {
            run($socket);
         } elsif (s/^verify //) {
            verify($socket);
            close $socket;
         } elsif (s/^find //) {
            find($socket);
            close $socket;
         } elsif (s/^complete //) {
            complete($socket);
            close $socket;
         } elsif (s/^port //) {
            translate_port($socket);
            close $socket;
         } elsif ($_ eq "system") {
            print $socket $^O;
            close $socket;
         } else {
            print $socket "ERROR: unknown command $_\n";
            close $socket;
         }
      }
   } else {
      last unless $!{EINTR};
   }
}

stop();

sub stop {
   close $serv;
   unlink $socketfile;
   exit(0);
}

sub verify {
   my ($socket)=@_;
   print $socket -x && -f _;
}

sub find {
   my ($socket)=@_;
   foreach my $dir (split /:/, $ENV{PATH}) {
      if (-x "$dir/$_" && -f _) {
         print $socket "$dir/$_";
         return;
      }
   }
}

sub complete {
   my ($socket)=@_;
   my $pattern=$_;
   my @candidates;
   if ($pattern =~ m{^/}) {
      @candidates=grep { -x } glob("${pattern}*");
   } else {
      @candidates=grep { -x && -f _ } map { glob("$_/${pattern}*") } split /:/, $ENV{PATH};
   }
   print $socket join(" ", @candidates);
}

sub translate_port {
   my ($socket)=@_;
   my $mapping=`docker port $cid $_`;
   if ($mapping =~ /:(\d+)$/) {
      print $socket $1;
   }
}

my %children;

sub run {
   my ($socket)=@_;
   if (my $pid=fork) {
      $children{$pid}=$socket;
   } else {
      POSIX::fcntl($socket, F_SETFD, FD_CLOEXEC);
      exec $_
      or do {
         print $socket "ERROR: could not run $_: $!\n";
         POSIX::_exit(1);
      }
   }
}

sub collect_children {
   while (keys(%children) && (my $pid=waitpid(-1, WNOHANG)) > 0) {
      my $socket=delete $children{$pid};
      print $socket "$?\n";
      close $socket;
   }
}


# Local Variables:
# mode: perl
# cperl-indent-level: 3
# indent-tabs-mode:nil
# End:
