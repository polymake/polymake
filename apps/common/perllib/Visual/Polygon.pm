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
#  GNU General Public License for more -----------------------------------------

###############################################################################
#
#  Visual::Polygon  - a flat polygon embedded in R^2 or R^3
#

package Visual::Polygon;

use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],

   # the proper cyclic drawing order of Vertices
   [ '$Facet' => '#%', default => 'undef' ],

   #  "R G B"           to fill the inner area
   [ '$FacetColor' => 'get_RGB(#%)', default => '$Visual::Color::facets' ],

   [ '$FacetTransparency' => '#%', default => 'undef' ],

   [ '$FacetNormals' => '#%', default => 'undef' ],

   #  "hidden"
   [ '$FacetStyle' => '#%', default => 'undef' ],

   #  "R G B"                the facet borders
   [ '$EdgeColor' => 'get_RGB(#%)', default => '$Visual::Color::edges' ],

   [ '$EdgeThickness' => '#%', default => 'undef' ],

   #  "hidden"
   [ '$EdgeStyle' => '#%', default => 'undef' ],

   [ '$NEdges' => '#%' ],
);

sub Facets : method {
   my $self=shift;
   [ $self->Facet // [ 0..$#{$self->Vertices} ] ]
}

sub Closed { 0 }
sub FacetLabels { undef }
sub FacetNeighbors { undef }

sub n_edges {scalar(@{(shift)->Vertices}) }

package Visual::DegeneratedPolygon;
use Polymake::Struct (
   [ '@ISA' => 'Polygon' ],
);

sub new {
   bless [ @{(pop)} ];
}

# make it compatible to Visual::Wire
sub all_edges {
   return new Wire::EdgeIterator([ [0,1] ]);
}

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
