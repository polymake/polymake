#  Copyright (c) 1997-2016
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
use namespaces;

package Polymake::Background::Watcher;
use POSIX qw( :sys_wait_h );

my %active;	# pid => something to destroy on termination
my $init;

sub gather_zombies {
   my $flag=shift || 0;
   if (!$flag) {
      foreach (grep { ref($_) eq "CODE" } values %active) {
	 eval { $_->() };
	 if ($@) {
	    warn_print( "cleanup after background process failed: $@\n" );
	 }
      }
   }
   while (keys(%active) && (my $pid=waitpid(-1,$flag)) > 0) {
      delete $active{$pid};
   }
}

sub add_process {
   if ($init) {
      gather_zombies(WNOHANG);
   } else {
      add AtEnd("Background", \&gather_zombies, before => "tempdir");
      $init=1;
   }
   $active{$_[0]}=$_[1];
}

package Polymake::Background::Process;

sub new {
   shift;
   my $options=shift if ref($_[0]) eq "HASH";
   my ($cleanup, $redirect);
   if ($options) {
      $cleanup=delete $options->{CLEANUP};
      $redirect=delete $options->{REDIRECT};
      if (keys %$options) {
	 croak( "unknown option(s) for Background::Process: ", join(",", keys %$options) );
      }
   }
   dbg_print( "starting @_" ) if $DebugLevel;
   my $pid=fork;
   if (!$pid) {
      die "Background::Process: fork failed: $!\n" if !defined($pid);
      if (defined($redirect)) {
	 POSIX::dup2(fileno($redirect->[0]), 0);
	 POSIX::dup2(fileno($redirect->[1]), 1);
      } else {
	 STDIN->close;
      }
      POSIX::setsid();
      $SIG{INT}='IGNORE';
      $SIG{ALRM}='IGNORE';
      exec "@_"
      or do {
	 my ($progname)= @_==1 ? $_[0] =~ /^\s*(\S+)/ : @_;
	 print STDERR "could not start $progname: $!\n";
	 POSIX::_exit(1);	# avoid executing global destructors
      }
   }
   Watcher::add_process($pid, $cleanup);
   return $pid;
}

package Polymake::Background::Pipe;
use Symbol 'gensym';
use POSIX ':fcntl_h';

sub new {
   shift;
   my @handles=(gensym, gensym);
   my @pipends=(gensym, gensym);
   pipe $pipends[0], $handles[0];
   pipe $handles[1], $pipends[1];
   fcntl($_, F_SETFD, FD_CLOEXEC) for (@pipends, @handles);
   new Process({ REDIRECT => \@pipends, CLEANUP => sub { close $handles[0]; close $handles[1]; } }, @_);
   $handles[0]->autoflush;
   @handles;
}

1
