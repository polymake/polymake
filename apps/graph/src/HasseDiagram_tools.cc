/* Copyright (c) 1997-2014
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
#include "polymake/graph/HasseDiagram.h"

namespace polymake { namespace graph {

template <typename Permutation>
perl::Object permuted_atoms(perl::Object in, const Permutation& perm)
{
   const HasseDiagram H(in);
   const sequence atoms=H.node_range_of_dim(0);
   const int atom0=atoms.front();
   Array<int> node_perm(H.nodes(), sequence(0).begin());
   copy(entire(translate(perm, atom0)), node_perm.begin()+atom0);

   Graph<Directed> G=permuted_nodes(H.graph(), node_perm);
   NodeMap<Directed, Set<int> > F(G);

   F[H.top_node()]=H.face(H.top_node());
   copy(entire(select(H.faces(), atoms)), select(F, atoms).begin());

   if (H.dim()>1) {
      for (Entire<sequence>::const_iterator f=entire(H.node_range_of_dim(1,-1)); !f.at_end(); ++f) {
         F[*f]=permuted(H.face(*f), perm);
      }
   }

   perl::Object out("FaceLattice");
   out.take("ADJACENCY") << G;
   out.take("FACES") << F;
   out.take("DIMS") << H.dims();
   return out;
}

template <typename Permutation>
perl::Object permuted_coatoms(perl::Object in, const Permutation& perm)
{
   const HasseDiagram H(in);
   const int coatom0=H.node_range_of_dim(-1).front();
   Array<int> node_perm(H.nodes(), sequence(0).begin());
   copy(entire(translate(perm, coatom0)), node_perm.begin()+coatom0);

   Graph<Directed> G=permuted_nodes(H.graph(), node_perm);
   NodeMap<Directed, Set<int> > F(G, entire(select(H.faces(), node_perm)));

   perl::Object out("FaceLattice");
   out.take("ADJACENCY") << G;
   out.take("FACES") << F;
   out.take("DIMS") << H.dims();
   return out;
}

FunctionTemplate4perl("permuted_atoms(FaceLattice, *)");
FunctionTemplate4perl("permuted_coatoms(FaceLattice, *)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
