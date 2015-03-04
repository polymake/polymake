/* Copyright (c) 1997-2015
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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/polytope/bisector.h"

namespace polymake { namespace polytope {

template<typename Scalar>
perl::Object stellar_indep_faces(perl::Object p_in, const Array< Set<int> >& in_faces)
{
   const bool bounded=p_in.give("BOUNDED");
   if (!bounded)
      throw std::runtime_error("stellar_indep_faces: input polytope must be bounded\n");

   Matrix<Scalar> V=p_in.give("VERTICES");
   const Matrix<Scalar> F=p_in.give("FACETS");
   const Matrix<Scalar> lineality_space=p_in.give("LINEALITY_SPACE");
   const Vector<Scalar> rel_int_point=p_in.give("REL_INT_POINT");
   const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
   const graph::HasseDiagram HD=p_in.give("HASSE_DIAGRAM");
   const Graph<> DG=p_in.give("DUAL_GRAPH.ADJACENCY");

   PowerSet<int> spec_faces;
   for (Entire< Array< Set<int> > >::const_iterator it=entire(in_faces);
        !it.at_end(); ++it)
      spec_faces += *it;

   const int dim = HD.dim();

   int v_count = V.rows();
   int indep_vert = 0;
   V.resize(V.rows()+spec_faces.size(), V.cols());

   // iterate over dimensions dim-1..0 and over all faces of each dimension
   for (int d=dim-1; d>=0; --d) {
      for (Entire<sequence>::iterator it=entire(HD.node_range_of_dim(d));
           !it.at_end(); ++it) {
         const Set<int>& face = HD.face(*it);
         if (spec_faces.contains(face)) {

            // produce all relevant inequalities = each neighbour (in the dual graph) of the nodes
            // corresponding to the facets of star(face) produces one inequality
            const Vector<Scalar> m = average( rows(V.minor(face, All)) ) - rel_int_point;
            Scalar t_max(2);
            
            // compute star(face)
            Entire< Set<int> >::const_iterator s_it=entire(face);
            Set<int> star_facets = VIF.col(*s_it);  ++s_it;
            for ( ; !s_it.at_end(); ++s_it)
               star_facets *= VIF.col(*s_it);

            for (Entire< Set<int> >::iterator st_it=entire(star_facets);
                 !st_it.at_end(); ++st_it) {
               const Set<int> neighbors = DG.adjacent_nodes(*st_it) - star_facets;

               for (Entire< Set<int> >::const_iterator n_it=entire(neighbors);
                    !n_it.at_end(); ++n_it) {
                  const int v= (VIF[*st_it] * VIF[*n_it]).front();
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
            const Scalar scale = (t_max + 1) / 2;
            if (d==0) {  // replace subdevided vertex
               V[face.front()] = rel_int_point + m * scale;
               ++indep_vert;

            } else {
               V[v_count] = rel_int_point + m * scale;
               ++v_count;
            }
         }
      }
   }

   if (V.rows() - v_count != indep_vert)
      throw std::runtime_error("stellar_indep_faces: non-faces specified");

   V.resize( v_count,V.cols() );

   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.set_description() << "Stellar subdivision of " << p_in.name() << endl;
   p_out.take("VERTICES") << V;
   p_out.take("LINEALITY_SPACE") << lineality_space;
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
