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

###############################################################################
#
#  Visual::PointSet  - a cloud of points
#

package Visual::PointSet;

use Polymake::Struct (
   [ '@ISA' => 'Visual::Object' ],

   # [ "coord vector", ... ]
   [ '$Vertices | Points' => 'check_points(#%)', default => 'croak("Points missing")' ],

   # ambient dimension
   [ '$Dim' => '$this->Vertices->cols' ],

   # [ "string", ... ]
   # or "hidden"                suppress any labels
   #    default: point indices (0,1,...) as labels
   [ '$VertexLabels | PointLabels' => 'unify_labels(#%)', default => 'undef' ],

   [ '$VertexColor | PointColor' => 'unify_decor(#%)', merge => 'unify_decor(#%)', default => '$Visual::Color::vertices' ],

   [ '$VertexThickness | PointThickness' => 'unify_decor(#%)', merge => 'unify_decor(#%)', default => '1' ],

   [ '$VertexBorderColor | PointBorderColor' => 'unify_decor(#%)', merge => 'unify_decor(#%)', default => 'undef' ],

   [ '$VertexBorderThickness | PointBorderThickness' => 'unify_decor(#%)', merge => 'unify_decor(#%)', default => 'undef' ],

   #  recognized values:
   #    "hidden"                draw neither circles nor point labels
   [ '$VertexStyle | PointStyle' => 'unify_decor(#%)', merge => 'unify_decor(#%)', default => 'undef' ],

   # boolean: don't show this object at the session start (only makes sense for interactive viewers)
   [ '$Hidden' => '#%' ],

   # View and Scale options for packages like Sketch capable of rendering 3-d prictures.
   # Default values correspond the standard camera position in jReality.
   [ '$ViewPoint' => '#%', default => '[0, 0, 1]' ],
   [ '$ViewDirection' => '#%', default => '[0, 0, 0]' ],
   [ '$ViewUp' => '#%', default => '[0, 1, 0]' ],
   [ '$Scale' => '#%', default => '1' ],

   [ '$LabelAlignment' => '#%', default => '"left"'],
);


# inverse transformation matrix, defaults pkg => (ViewPoint, ViewDirection, ViewUp, Scale)
sub transform2view {
   my ($self, $inv_transform, $defaults_pkg)=@_;

   ( !Struct::is_default($self->ViewPoint)
     ? $self->ViewPoint :
     defined($inv_transform)
     ? ($inv_transform * (new Vector<Float>(1., @{$self->ViewPoint})))->slice(1)
     : *{$defaults_pkg->{view_point}}{ARRAY}
   ),
   ( !Struct::is_default($self->ViewDirection)
     ? $self->ViewDirection :
     defined($inv_transform)
     ? ($inv_transform * (new Vector<Float>(1., @{$self->ViewDirection})))->slice(1)
     : *{$defaults_pkg->{view_direction}}{ARRAY}
   ),
   ( !Struct::is_default($self->ViewUp)
     ? $self->ViewUp :
     defined($inv_transform)
     ? ($inv_transform * (new Vector<Float>(0., @{$self->ViewUp})))->slice(1)
     : *{$defaults_pkg->{view_up}}{ARRAY}
   ),
   ( !Struct::is_default($self->Scale)
     ? $self->Scale :
     defined($inv_transform)
     ? do {
          my $ray=new Vector<Float>(0, 1, 1, 1);
          my $img=$inv_transform*$ray;
          $self->Scale * sqrt(3. / ($img*$img))
       }
     : ${$defaults_pkg->{scale}}
   )
}

1

# Local Variables:
# mode:perl
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
