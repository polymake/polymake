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

package Visual::DynamicCoords;

use Polymake::Struct (
   [ new => '$@' ],
   [ '$source' => '#1' ],
   [ '%options' => '@' ],
   [ '$client_object' => 'undef' ],     # opaque C++ object returned by the serving client
   [ '@coord' => 'undef' ],
);

use overload 'bool' => sub { 1 },
             '==' => \&refcmp,
             '!=' => sub { !&refcmp },
             '@{}' => sub { my ($self)=@_; $self->coord //= $self->compute };

sub merge_options {
   my $self=shift;
   if ($#_==0 && is_hash(my $options=shift)) {
      while (my ($key, $value)=each %$options) {
         $self->options->{$key} //= $value;
      }
   } elsif ($#_ % 2) {
      while (my ($key, $value)=splice @_, 0, 2) {
         $self->options->{$key} //= $value;
      }
   } else {
      croak( "invalid option list: expected HASH or literal list of (keyword => value) pairs" );
   }
}

sub feedback {}  # don't consume anything
sub run {} # no action

sub closed {
   my ($self)=@_;
   if (defined $self->client_object) {
      $self->client_object->shutdown;
   }
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
