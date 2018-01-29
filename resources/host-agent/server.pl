#!/usr/bin/perl
#
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
#
#  An agent process running in the host namespace (outside any containers)
#  or in a different container than the polymake process.
#  Its purpose is to launch other programs in background like static visualization
#  tools and to assist the rule configuration procedures in exploring the host file system.
#

use strict;
use Socket;
use IO::Handle;
use POSIX qw(:fcntl_h :sys_wait_h :errno_h);

my $socketfile=$ARGV[0];
(my $pidfile=$socketfile) =~ s{^.*/\K[^/]+$}{pid};
unlink $socketfile;

my $serv;
if ($^O eq "darwin") {
  # docker on MacOS does not support data exchange between host and container over filesystem sockets yet
  my $port;
  socket($serv, PF_INET, SOCK_STREAM, getprotobyname('tcp')) or die "socket failed: $!\n";
  for ($port=30000; $port<65536; ++$port) {
    last if bind($serv, sockaddr_in($port, INADDR_LOOPBACK));
    die "bind failed: $!\n" if $! != EADDRINUSE;
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

if (my $pid=fork) {
   open PID, ">", $pidfile;
   print PID $pid;
   close PID;
   exit(0);
}

POSIX::setsid();
$SIG{INT}='IGNORE';
$SIG{TERM}=\&stop;
$SIG{CHLD}=\&collect_children;

for (;;) {
   if (accept(my $socket, $serv)) {
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
      } else {
	 print $socket "ERROR: unknown command $_\n";
	 close $socket;
      }
   } elsif (POSIX::errno != EINTR) {
      last;
   }
}

stop();

sub stop {
   close $serv;
   unlink $socketfile, $pidfile;
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
