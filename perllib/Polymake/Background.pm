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

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);

package Polymake::Background;
use POSIX qw( :sys_wait_h );

my %active;     # pid => something to destroy on termination
my $init;

sub gather_zombies {
   my $flag=shift // 0;
   if (!$flag) {
      foreach (grep { ref($_) eq "CODE" } values %active) {
         eval { $_->() };
         if ($@) {
            warn_print( "cleanup after background process failed: $@\n" );
         }
      }
   }
   while (keys(%active) && (my $pid=waitpid(-1, $flag)) > 0) {
      delete $active{$pid};
   }
}

sub wait_for_active {
   my $rc=0;
   foreach my $pid (keys %active) {
      waitpid($pid, 0);
      $rc ||= $?;
      delete $active{$pid};
   }
   return $rc;
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
   my $pid=launch($redirect, 1, @_);
   add_process($pid, $cleanup);
   return $pid;
}

sub launch {
   my ($redirect, $hide, @cmdline)=@_;
   dbg_print( "starting @cmdline" ) if $Verbose::external;
   my $pid=fork;
   if (!$pid) {
      die "Background::Process: fork failed: $!\n" if !defined($pid);
      if (defined($redirect)) {
         POSIX::dup2(fileno($redirect->[0]), 0);
         POSIX::dup2(fileno($redirect->[1]), 1);
      } else {
         STDIN->close;
      }
      if ($hide) {
         POSIX::setsid();
         $SIG{INT}='IGNORE';
         $SIG{ALRM}='IGNORE';
      }
      if (ref($cmdline[0]) eq "ARRAY") {
         # internal subprocess: [ \&sub, leading arguments ], further arguments
         my $lead=shift @cmdline;
         my $sub=shift @$lead;
         if (ref($sub) eq "CODE") {
            eval { $sub->(@$lead, @cmdline) };
         } else {
            $@="wrong use: a CODE reference expected\n";
         }
         if ($@) {
            print STDERR "internal subprocess terminated with an error: $@";
            POSIX::_exit(1);
         } else {
            POSIX::_exit(0);    # avoid executing global destructors
         }

      } else {
         exec "@cmdline"
         or do {
            my ($progname)= @cmdline==1 ? $cmdline[0] =~ /^\s*(\S+)/ : @cmdline;
            print STDERR "could not start $progname: $!\n";
            POSIX::_exit(1);    # avoid executing global destructors
         }
      }
   }
   return $pid;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
