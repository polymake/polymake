#  Copyright (c) 1997-2021
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

package Visual::Transformation;

use Polymake::Struct (
   [ new => '$' ],
   [ '$representative' => '#1' ],   # an Object where to attach the transformation
);

my $att_name="ViewTransformation";

sub feedback {
   my ($self, $cmd, $pipe, $viewer)=@_;
   if ($cmd =~ s/^T\s+//) {
      my $transf=$viewer->parse_transformation_matrix($cmd);
      $self->representative->attach($att_name, $transf);
      1
   } else {
      0
   }
}

sub run {}      # nothing happens
sub closed {}   # ignore the event

sub get_transformation_matrix {
   my ($self, $obj)=@_;
   ($obj // $self->representative)->get_attachment($att_name);
}

sub new_representative {
   my ($self, $new_rep)=@_;
   $new_rep->attach($att_name, $self->representative->get_attachment($att_name));
   $self->representative=$new_rep;
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
