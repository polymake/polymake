/* Copyright (c) 1997-2018
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/

#include "polymake/client.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/topaz/boundary_tools.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace topaz {

   // returns (facets, vertex_map)
perl::ListReturn boundary_of_pseudo_manifold_client(perl::Object sc)
{
   Lattice<BasicDecoration> HD = sc.give("HASSE_DIAGRAM");
   auto faces = IncidenceMatrix<>{ attach_member_accessor( boundary_of_pseudo_manifold(HD),
         ptr2type<BasicDecoration, Set<int>, &BasicDecoration::face>()) };

   perl::ListReturn result;
   auto sq = squeeze_faces(faces);
   result << sq.first << sq.second;
   return result;
}

std::pair< Array<Set<int>>, Array<int> > squeeze_faces_client(IncidenceMatrix<> in){
   return squeeze_faces(in);
}

Function4perl(&boundary_of_pseudo_manifold_client, "boundary_of_pseudo_manifold(SimplicialComplex)");
Function4perl(&squeeze_faces_client, "squeeze_faces($)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
