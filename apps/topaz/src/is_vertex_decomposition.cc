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

namespace polymake { namespace topaz {

bool is_vertex_decomposition(perl::Object p, const Array<int>& ShedVert, perl::OptionSet options)
{
   if (ShedVert.empty())
      throw std::runtime_error("no shedding vertices specified");

   const bool verbose=options["verbose"];

   const bool pure=p.give("PURE");
   if (!pure) {
      if (verbose) cout << "The complex is not pure." << endl;
      return false;
   }
   const int d=p.give("DIM");

   if (d==0) {
      const int n_vertices=p.give("N_VERTICES");
      if (ShedVert.size() > n_vertices) {
         if (verbose) cout << "Too many shedding vertices specified." << endl;
         return false;
      }
      Set<int> V(sequence(0,n_vertices));
      accumulate_in(entire(ShedVert), operations::sub(), V);
      if (!V.empty()) {
         if (verbose) cout << "The complex is not completely decomposed." << endl;
         return false;
      }
      return true;
   }
   if (d>3)
      throw std::runtime_error("is_vertex_decomposition: Dimension of the complex must be smaller than 4.");

   const bool is_MF=p.give("MANIFOLD");
   if (!is_MF) {
      if (verbose) cout << "Complex is not a manifold." << endl;
      return false;
   }

   Lattice<BasicDecoration> HD_obj = p.give("HASSE_DIAGRAM");
   ShrinkingLattice<BasicDecoration> HD(HD_obj);
   // for all v in ShedVert
   for (Entire< Array<int> >::const_iterator v_it=entire(ShedVert); !v_it.at_end(); ++v_it) {
      const int v=*v_it;
      // if the remaining complex consists of v only -> complex is decomposed
      const auto rest_vertex_nodes=HD.nodes_of_rank(1);
      if (rest_vertex_nodes.size()==1) {
         if (HD.face(rest_vertex_nodes.front()).front() != v || !(++v_it).at_end()) {
            if (verbose) cout << "Too many shedding vertices specified." << endl;
            return false;
         }
         return true;
      }

      // compute the vertices of link(v) and remove star(v) from HD
      const Set<int> V_of_link = vertices_of_vertex_link(HD,v);
      remove_vertex_star(HD,v);
      // check the invariant:
      // dim == 0 -> true
      // dim > 0  -> HD is a manifold.
      const int dim = HD.rank()-2;
      if (dim==0) continue;
      // it suffices to check the link for all vertices of link(v)
      for (Set<int>::const_iterator l_it=V_of_link.begin(); !l_it.at_end(); ++l_it) {
         const int w=*l_it;
         const std::list< Set<int> > link=as_iterator_range(vertex_link_in_HD(HD,w));
         if (dim==1 && !link.empty() && link.size()<3) continue;
         if (dim==2 && !link.empty() && is_ball_or_sphere(link, int_constant<1>())>0) continue;
         if (dim==3 && !link.empty() && is_ball_or_sphere(link, int_constant<2>())>0) continue;
         if (verbose) cout << "The remaining complex after removing vertex star(" << v << ") is not a manifold." << endl;
         return false;
      }
   }  // end for all v in ShedVert

   if (verbose) cout << "The complex is not completely decomposed." << endl;
   return false;
}

UserFunction4perl("# @category Other"
                  "# Check whether a given ordered subset of the vertex set is a __vertex decomposition__.\n"
                  "# Works for 1-, 2- and 3-manifolds only!\n"
                  "# @param SimplicialComplex complex"
                  "# @param Array<Int> vertices shedding vertices"
                  "# @option Bool verbose"
                  "# @return Bool",
                  &is_vertex_decomposition, "is_vertex_decomposition(SimplicialComplex $ { verbose=>0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
