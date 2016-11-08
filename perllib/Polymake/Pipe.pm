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

package Polymake::Selector;
use Fcntl;

declare %active;
declare @channels;
declare $rmask="";
declare $wmask="";

package Polymake::Selector::Member;
use Polymake::Struct (
   [ new => '$' ],
   [ '$handle' => '#1' ],
   [ '$rfd | wfd' => 'fileno(#1)' ],
);

sub new {
   my $self=&_new;
   if (keys(%active)==1) {
      $self->not_alone;
      (each %active)->not_alone;
   }
   $active{$self}=0;
   weak($channels[$self->rfd]=$self);
   vec($rmask, $self->rfd, 1)=1;
   $self;
}

sub FILENO { (shift)->rfd }

sub bye {
   my ($self, $destroying)=@_;
   delete $active{$self};
   undef $self->rfd;
   if (keys(%active)==1) {
      (each %active)->alone;
   }
   bless $self, "Polymake::Selector::Closed" unless $destroying;
   1
}

sub forget {
   my ($self)=@_;
   undef $channels[$self->rfd];
   vec($rmask,$self->rfd,1)=0;
   vec($wmask,$self->wfd,1)=0;
}

sub CLOSE {
   my ($self)=@_;
   &forget;
   POSIX::close($self->rfd);
   &bye;
}

sub DESTROY {
   my $self=shift;
   if (defined($self->rfd) && vec($rmask,$self->rfd,1)) {
      $self->CLOSE(1);
   }
}

sub alone {}
sub not_alone {}

#####################################################################################
package Polymake::Selector;

sub try_read {
   my $member=shift;
   my ($rready, $wready);
   while (select $rready=$rmask, $wready=$wmask, undef, undef) {
      if (vec($rready, $member->rfd, 1)) {
	 return 1;
      }
      foreach my $wfd (ones($wready)) {
	 $channels[$wfd]->out_avail;
      }
      foreach my $rfd (ones($rready)) {
	 $channels[$rfd]->in_avail;
      }
   }
   0
}

sub try_write {
   my $member=shift;
   my ($rready, $wready);
   while (select $rready=$rmask, $wready=$wmask, undef, 0) {
      if (vec($wready, $member->wfd, 1)) {
	 return $member->out_avail;
      }
      foreach my $wfd (ones($wready)) {
	 $channels[$wfd]->out_avail;
      }
      foreach my $rfd (ones($rready)) {
	 $channels[$rfd]->in_avail;
      }
   }
   0
}

#####################################################################################
package Polymake::Selector::Closed;
use Polymake::Struct [ '@ISA' => 'Member' ];

sub READ { 0 }
sub WRITE { 0 }
sub CLOSE { 1 }
sub DESTROY { }

#####################################################################################
package Polymake::Pipe;
use Fcntl;
use POSIX qw(:errno_h);

use Polymake::Struct (
   [ '@ISA' => 'Selector::Member' ],
   [ '$flags' => 'undef' ],
   '$rbuffer',
   '$wbuffer',
);

sub construct {
   my $self=&Member::new;
   tie *{$self->handle}, $self;
   $self->flags=fcntl($self->handle, F_GETFL, 0);
   $self;
}

# returns the tied handle, not the Pipe object!
sub new {
   &construct; shift;
}

sub TIEHANDLE { shift }

sub READLINE {
   my $self=shift;
   my ($l, $gotten)=(0);
   if (defined $/) {
      do {
	 if ((my $endl=index($self->rbuffer, $/, $l)) >= 0) {
	    return substr($self->rbuffer, 0, $endl+1, "");
	 }
	 $l=length($self->rbuffer);
      } while ($gotten=$self->do_read($self->rbuffer,1024,$l)) > 0;
   } else {
      my $whole=$self->rbuffer;
      do {
	 $gotten=$self->do_read($whole,2<<16,length($whole));
      } while ($gotten>0);
      return $whole if defined($gotten);
   }
   die "error reading from Pipe: $!\n" if !defined $gotten;
   undef;
}

sub READ {
   my $self=shift;
   if (length($self->rbuffer)) {
      my (undef, $len, $offset)=@_;
      assign_min($len, length($self->rbuffer));
      if ($offset) {
	 substr($_[0],$offset)=substr($self->rbuffer,0,$len,"");
      } else {
	 $_[0]=substr($self->rbuffer,0,$len,"");
      }
      return $len;
   } else {
      $self->do_read(@_);
   }
}

sub do_read {
   my ($self, undef, $len, $offset)=@_;
   my $gotten;
   if ($offset) {
      my $app;
      $gotten=POSIX::read($self->rfd, $app, $len)
      and substr($_[1],$offset)=$app;
   } else {
      $gotten=POSIX::read($self->rfd, $_[1], $len);
   }
   $self->CLOSE if $gotten==0;
   $gotten;
}

sub read_chunk {
   my ($self, $req)=@_;
   my $data="";
   my $l;
   while (($l=length($data)) < $req) {
      defined(READ($self,$data,$req-$l,$l)) or last;
   }
   $data;
}

sub PRINT {
   my $self=shift;
   $self->WRITE(join($, , @_).$\);
}

sub PRINTF {
   my $self=shift;
   $self->WRITE(sprintf(@_));
}

sub WRITE {
   my $self=shift;
   my ($str, $len);
   if (@_==1) {
      $str=$_[0];  $len=length($str);
   } else {
      $str=substr($_[0],$_[2],$len=$_[1]);
   }
   $self->set_non_blocking_write;
   my $written=POSIX::write($self->wfd,$str,$len);
   if (!defined($written)) {
      if ($!==POSIX::EAGAIN) {
	 $written=0;
      } else {
	 $self->CLOSE;
	 return undef;
      }
   }
   $self->set_blocking_write;
   if ($written<$len) {
      $self->wbuffer=substr($str,$written,$len-$written);
      vec($wmask,$self->wfd,1)=1;
      $self->not_alone if keys(%active)==1;
   }
   $len;
}

sub CLOSE {
   my ($self)=@_;
   &forget;
   untie *{$self->handle};
   close($self->handle);
   &bye;
}

sub not_alone {
   bless shift, "Polymake::CollaborativePipe";
}

sub set_non_blocking_write {
   my $self=shift;
   fcntl($self->handle, F_SETFL, $self->flags|O_NONBLOCK);
}

sub set_blocking_write {
   my $self=shift;
   fcntl($self->handle, F_SETFL, $self->flags);
}

#####################################################################################
package Polymake::CollaborativePipe;
use Polymake::Struct [ '@ISA' => 'Pipe' ];

sub out_avail {
   my $self=shift;
   my $written=POSIX::write($self->wfd, $self->wbuffer, length($self->wbuffer));
   if (!defined($written)) {
      if ($!==POSIX::EAGAIN) {
	 $written=0;
      } else {
	 $self->CLOSE;
	 return undef;	# dead pipe
      }
   }
   if ($written == length($self->wbuffer)) {
      vec($wmask, $self->wfd, 1)=0;
      $self->wbuffer="";
      $self->alone if keys %active==1;
      1;		# stalled output completely drained
   } else {
      substr($self->wbuffer, 0, $written)="";
      0			# still output pending
   }
}

sub in_avail {
   my $self=shift;
   Pipe::do_read($self, $self->rbuffer, 1024, length($self->rbuffer));
}

sub do_read {
   my ($self)=@_;
   try_read($self) && &Pipe::do_read;
}

sub WRITE {
   my ($self)=@_;
   if (length($self->wbuffer) and !(my $rc=try_write($self))) {
      defined($rc) or return;
      if (@_==1) {
	 $self->wbuffer.=$_[0];
	 length($_[0]);
      } else {
	 $self->wbuffer.=substr($_[0],$_[2],$_[1]);
	 $_[1];
      }
   } else {
      &Pipe::WRITE;
   }
}

sub alone {
   my $self=shift;
   if (!length($self->wbuffer)) {
      bless $self, "Polymake::Pipe";
   }
}

sub not_alone {}

#####################################################################################
package Polymake::Pipe::WithRedirection;
use IPC::Open3;
use Fcntl;

use Polymake::Struct (
   [ '@ISA' => 'CollaborativePipe' ],
   [ new => '$$$' ],
   [ '$out' => '#2' ],
   [ '$wfd' => 'fileno(#2)' ],
   [ '$pid' => '#3' ],
);

sub new {
   my $pkg=shift;
   my ($in, $out, $err);
   if ($_[0] =~ /^2>/) {
      shift;
      open MYERR, ">", $' or die "Can't redirect STDERR to $': $!\n";
      $err=">&MYERR";
   } else {
      $err=">&STDERR";
   }
   my $pid=eval { open3($out, $in, $err, @_) };
   close MYERR;
   if ($@) {
      $@ =~ s/^.*?://;
      $@ =~ s/ at \(eval.*$/\n/;
      if ($$ != getpgrp) {	# we are in the fork!!!
	 print STDERR $@;
	 POSIX::_exit(1);	# avoid executing global destructors
      }
      die $@;
   }
   fcntl($out, F_SETFL, O_NONBLOCK);
   my $self=construct($pkg,$in,$out,$pid);
   weak($channels[$self->wfd]=$self);
   $self;
}

sub close_out {
   my ($self)=@_;
   undef $channels[$self->wfd];
   vec($wmask,$self->wfd,1)=0;
   close($self->out);
}

sub CLOSE { &close_out; &Pipe::CLOSE; }

sub set_blocking_write {}	# remains always non-blocking
sub set_non_blocking_write {}
sub alone {}

1
