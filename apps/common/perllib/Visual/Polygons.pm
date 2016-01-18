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
#  Visual::Polygons - a collection of polygons sharing the same vertex set
#                     and making up a single geometric object
#                     (this makes a difference for some visualizers, esp. javaview.)

package Visual::Polygons;
use Polymake::Struct (
   [ '@ISA' => 'PointSet' ],
   [ '$Facets' => 'check_points(#%)', default => 'croak("Facets missing")' ],
   [ '$FacetNormals' => '#%', default => 'undef' ],
   [ '$FacetNeighbors' => '#%', default => 'undef' ],
   [ '$FacetLabels' => 'unify_labels(#%)', default => '"hidden"' ],
   [ '$FacetColor' => 'unify_decor(#%)', merge => 'unify_decor(#%)', default => '$Visual::Color::facets' ],
   [ '$FacetTransparency' => 'unify_decor(#%)', merge => 'unify_decor(#%)', default => 'undef' ],
   [ '$FacetStyle' => 'unify_decor(#%)', merge => 'unify_decor(#%)', default => 'undef' ],
   [ '$EdgeColor' => 'get_RGB(#%)', default => '$Visual::Color::edges' ],
   [ '$EdgeThickness' => '#%', default => 'undef' ],
   [ '$EdgeStyle' => '#%', default => 'undef' ],
   [ '$Closed' => '#%', default => 'defined($this->FacetNeighbors) && $this->Dim==3' ],
   [ '$NEdges' => '#%', default => 'croak("NEdges missing")' ],
);

1

# Local Variables:
# cperl-indent-level:3
# indent-tabs-mode:nil
# End:
