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

use strict;
use namespaces;
use warnings qw(FATAL void syntax misc);

package Polymake::Test::Output;

use Polymake::Struct(
   [ '@ISA' => 'Stream' ],
   [ new => '$$%' ],
   [ '$name' => '#2' ],
   [ '$body' => '#1' ],
   [ '$expected_error' => '#%' ],
   '$gotten_error',
);

sub new {
   my $self = &Case::new;
   local open STDOUT, ">:utf8", \($self->buffer);
   $self->run_code;
   $self;
}

sub run_code {
   my ($self) = @_;
   eval { $self->body->() };
   if ($@) {
      $self->gotten_error = neutralized_ERROR();
      $@ = "";
   }
}

sub execute {
   my ($self) = @_;
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
