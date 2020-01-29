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
#  This file is part of the polymake database interface polyDB.
#
#   @author Silke Horn, Andreas Paffenholz
#   http://www.mathematik.tu-darmstadt.de/~paffenholz
#

package PolyDB::Cursor;

@ISA = qw( MongoDB::Cursor );

use Try::Tiny;

sub prime {
   my ($pkg, $cursor, $postprocess) = @_;
   $cursor->immortal(1);
   if (defined($postprocess)) {
      $cursor->{postprocess} = $postprocess;
   }
   bless $cursor, $pkg;
}

sub next {
   my ($self) = @_;
   my $data = try {
      $self->SUPER::next
   } catch {
      die_neatly($_, $self->{_query}->full_name);
   };
   if (defined($data) && defined(my $postprocess = $self->{postprocess})) {
      $postprocess->($data)
   } else {
      $data
   }
}

sub all {
   my ($self) = @_;
   my $postprocess = $self->{postprocess};
   map { defined($postprocess) ? $postprocess->($_) : $_ }
   try {
      $self->SUPER::all
   } catch {
      die_neatly($_, $self->{_query});
   }
}


1

# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
