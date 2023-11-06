#  Copyright (c) 1997-2022
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
#   @author Andreas Paffenholz
#   (c) 2015 - 2023
#   https://polydb.org
#   https://www.mathematik.tu-darmstadt.de/~paffenholz
#


package PolyDB::Cursor;

use Polymake::Struct (
   [ new => '$;$' ],
   '$cursor',
   '$postprocess',
);

# FIXME apparently we cannot create an immortal cursor with mongo
sub new {
   my $self=&_new;
   my ($cursor,$postprocess) = @_;

   if (defined($postprocess)) {
      $self->postprocess = $postprocess;
   } else {
      $self->postprocess = undef;
   }
   $self->cursor = new Polymake::common::PolyDBCursor($cursor);
   $self;
}

sub next {
   my ($self) = @_;
   if ( $self->cursor->has_next() ) {
      my $json = JSON->new;
      my $data = $json->decode($self->cursor->next);

      if (defined($data) && defined(my $postprocess = $self->postprocess)) {
         $postprocess->($data)
      } else {
         $data
      }
   } else {
      return undef;      
   }
}

sub has_next {
   my ($self) = @_;
   return $self->cursor->has_next();
}

sub all {
   my ($self) = @_;
   my $res = $self->cursor->all();

   my $data = [];
   my $json = JSON->new;
   if (defined($res) ) {
      if (defined(my $postprocess = $self->postprocess)) {
         foreach (@$res) {
            push @$data, $postprocess->($json->decode($_));
         }
      } else {
         foreach (@$res) {
            push @$data, $json->decode($_);
         }
      }     
   }
   $data;
}

1

# Local Variables:
# mode: perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
