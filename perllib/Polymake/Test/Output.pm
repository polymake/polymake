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

package Polymake::Test::Output;

use Polymake::Struct(
   [ '@ISA' => 'Stream' ],
   [ new => '$$%' ],
   [ '$name' => '#2' ],
   [ '@filters' => '[]' ],
   [ '$body' => '#1' ],
   [ '$expected_error' => '#%' ],
   '$gotten_error',
);

sub new {
   my $self=&Case::new;
   before_run($self);
   eval { $self->body->() };
   if ($@) {
      $self->gotten_error=neutralized_ERROR();
      $@="";
   }
   after_run($self);
   $self;
}

sub before_run {
   my ($self)=@_;
   open my $saved_STDOUT, ">&=STDOUT";
   $self->handle=$saved_STDOUT;
   close STDOUT;
   open STDOUT, ">:utf8", \($self->buffer);
}

sub after_run {
   my ($self)=@_;
   close STDOUT;
   open STDOUT, ">&=", $self->handle;
}

sub execute {
   my ($self)=@_;
   if (length($self->gotten_error)) {
      if ($self->expected_error) {
         $self->buffer .= $self->gotten_error;
      } else {
         $self->fail_log="expected: regular return\n".
                         "     got: EXCEPTION: ".$self->gotten_error;
         return 0;
      }
   } elsif ($self->expected_error) {
      $self->fail_log="expected: EXCEPTION\n".
                      "     got: regular return ".
                      (length($self->buffer) ? "with output\n".$self->buffer : "without any output\n");
      return 0;
   }
   &Stream::execute;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
