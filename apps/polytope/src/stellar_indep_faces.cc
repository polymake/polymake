/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/polytope/bisector.h"

namespace polymake { namespace polytope {

template<typename Scalar>
BigObject stellar_indep_faces(BigObject p_in, const Array<Set<Int>>& in_faces)
{
   const bool bounded=p_in.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("stellar_indep_faces: input polytope must be bounded\n");

   Matrix<Scalar> V=p_in.give("VERTICES");
   const Matrix<Scalar> F=p_in.give("FACETS");
   const Matrix<Scalar> lineality_space=p_in.give("LINEALITY_SPACE");
   const Vector<Scalar> rel_int_point=p_in.give("REL_INT_POINT");
   const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
   graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential> HD = p_in.give("HASSE_DIAGRAM");

   const Graph<> DG=p_in.give("DUAL_GRAPH.ADJACENCY");

   PowerSet<Int> spec_faces;
   for (auto it = entire(in_faces); !it.at_end(); ++it)
      spec_faces += *it;

   const Int dim = HD.rank()-1;

   Int v_count = V.rows();
   Int indep_vert = 0;
   V.resize(V.rows()+spec_faces.size(), V.cols());

   // iterate over dimensions dim-1..0 and over all faces of each dimension
   for (Int d = dim-1; d >= 0; --d) {
      for (const auto face_index : HD.nodes_of_rank(d+1)) {
         const Set<Int>& face = HD.face(face_index);
         if (spec_faces.contains(face)) {

            // produce all relevant inequalities = each neighbour (in the dual graph) of the nodes
            // corresponding to the facets of star(face) produces one inequality
            const Vector<Scalar> m = average( rows(V.minor(face, All)) ) - rel_int_point;
            Scalar t_max(2);
            
            // compute star(face)
            auto s_it = entire(face);
            Set<Int> star_facets = VIF.col(*s_it);  ++s_it;
            for ( ; !s_it.at_end(); ++s_it)
               star_facets *= VIF.col(*s_it);

            for (auto st_it = entire(star_facets); !st_it.at_end(); ++st_it) {
               const Set<Int> neighbors = DG.adjacent_nodes(*st_it) - star_facets;

               for (auto n_it = entire(neighbors); !n_it.at_end(); ++n_it) {
                  const Int v = (VIF[*st_it] * VIF[*n_it]).front();
                  const Vector<Scalar> inequ = bisector(F[*st_it], F[*n_it], V[v]);

                  const Scalar denominator = inequ * m;
                  if (denominator == 0)
                     continue;

                  const Scalar t = -inequ * rel_int_point / denominator;
                  if (t < 0)
                     continue;
                  if (t<t_max)
                     t_max = t;
               }
            }

            // add vertex
            const Scalar scale = (t_max+1)/2;
            if (d==0) {  // replace subdevided vertex
               V[face.front()] = rel_int_point + m*scale;
               ++indep_vert;

            } else {
               V[v_count] = rel_int_point + m*scale;
               ++v_count;
            }
         }
      }
   }

   if (V.rows() - v_count != indep_vert)
      throw std::runtime_error("stellar_indep_faces: non-faces specified");

   V.resize( v_count,V.cols() );

   BigObject p_out("Polytope", mlist<Scalar>(),
                   "VERTICES", V,
                   "LINEALITY_SPACE", lineality_space);
   p_out.set_description() << "Stellar subdivision of " << p_in.name() << endl;
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Perform a stellar subdivision of the faces //in_faces// of a polyhedron //P//."
                          "# "
                          "# The faces must have the following property:"
                          "# The open vertex stars of any pair of faces must be disjoint."
                          "# @param Polytope P, must be bounded"
                          "# @param Array<Set<Int>> in_faces"
                          "# @return Polytope"
                          "# @author Nikolaus Witte",
                          "stellar_indep_faces<Scalar>(Polytope<Scalar> $)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
