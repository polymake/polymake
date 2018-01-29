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
use namespaces;
use warnings qw(FATAL void syntax misc);

require Polymake::Background;

#####################################################################################
package Polymake::ProgramPipe::Reading;
use POSIX qw(:sys_wait_h);

use Polymake::Struct (
   [ '@ISA' => 'Pipe::Collaborative' ],
   [ new => '$$$' ],
   [ '$out' => '#2' ],
   [ '$wfd' => 'fileno(#2)' ],
   [ '$pid' => '#3' ],
   [ '$program' => 'undef' ],
);

sub CLOSE {
   my ($self)=@_;
   my $rc=Pipe::CLOSE($self, 1);
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

sub set_blocking_write {}	# remains always non-blocking
sub set_non_blocking_write {}
sub alone {}

#####################################################################################
package Polymake::ProgramPipe;
use POSIX qw(:signal_h);
use Fcntl;

#  Constructor:  new ProgramPipe('command', 'arg', ... );
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
   my $pkg=shift;
   my ($in, $out, $pid)=new Background::Pipe(@_);
   fcntl($out, F_SETFL, O_NONBLOCK);
   my $self=construct($pkg, $in, $out, $pid);
   $self->register_write_channel;
   $self->program=$_[0];
   $self->handle;
}

#  The source data (output for us) are complete, start receiving the results
sub switch_to_input {
   my ($self)=@_;
   # flush the output buffer
   if (length($self->wbuffer)) {
      until (my $rc=try_write($self)) {
	 defined($rc) or die "closed pipe\n";
      }
   }
   &close_out;
   bless $self, "Polymake::ProgramPipe::Reading";
}

sub close_out {
   my ($self)=@_;
   $self->unregister_write_channel;
   close($self->out);
}

sub READ {
   (shift)->switch_to_input->READ(@_);
}

sub READLINE {
   (shift)->switch_to_input->READLINE;
}

# seems to die before started to consume input: shoot the program down
sub CLOSE {
   my ($self)=@_;
   &close_out;
   kill SIGINT, $self->pid;
   undef $self->program;
   Reading::CLOSE($self);
   1
}


1

# Local Variables:
# c-basic-offset:3
# End:
