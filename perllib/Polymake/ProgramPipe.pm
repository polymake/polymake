#  Copyright (c) 1997-2015
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

#####################################################################################
package Polymake::ProgramPipe::Reading;
use POSIX qw(:sys_wait_h);
use Fcntl;

use Polymake::Struct (
   [ '@ISA' => 'Pipe::WithRedirection' ],
   [ '$program' => 'undef' ],
);

sub CLOSE {
   my ($self)=@_;
   my $rc=Pipe::CLOSE($self,1);
   waitpid $self->pid, 0;
   if (!$@  &&  $?  &&  defined($self->program)) {
      my $rc=$?>>8;
      $self->program =~ m'([^/\s]+)(?:\s|$)';
      die $rc > 128 ? ("$1 exited with code ", $rc>>8)
	            : ("$1 killed by signal ", $rc&127),
          "\n";
   }
   $rc;
}

#####################################################################################
package Polymake::ProgramPipe;
use POSIX qw(:signal_h);

#  Constructor:  new ProgramPipe([ "2>errorstream" ], 'command', 'arg', ... );
#
#  Supports only one special, although very common communication "protocol":
#  send the source data to the program, then switch into the input mode,
#  and read all the results it has produced.
#
use Polymake::Struct (
   [ '@ISA' => 'Reading' ],
);

# returns the tied handle, not the ProgramPipe object!
sub new {
   my $self=&Pipe::WithRedirection::new;
   dbg_print( "running '@_'" ) if $Verbose::external;
   $self->program=$_[0];
   $self->handle;
}

#  The source data (output for us) are complete, start receiving the results
sub switch_to_input {
   my $self=shift;
   # flush the output buffer
   if (length($self->wbuffer)) {
      until (my $rc=try_write($self)) {
	 defined($rc) or die "closed pipe\n";
      }
   }
   $self->close_out;
   bless $self, "Polymake::ProgramPipe::Reading";
}

sub READ {
   (shift)->switch_to_input->READ(@_);
}

sub READLINE {
   (shift)->switch_to_input->READLINE;
}

# seems to die before started to consume input: shoot the program down
sub CLOSE {
   my $self=shift;
   $self->close_out;
   kill SIGINT, $self->pid;
   undef $self->program;
   $self->SUPER::CLOSE(@_);
   1
}


1

# Local Variables:
# c-basic-offset:3
# End:
