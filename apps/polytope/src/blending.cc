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
#include "polymake/vector"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace polytope {

perl::Object blending(perl::Object p_in1, const int vertex1, perl::Object p_in2, const int vertex2, perl::OptionSet options)
{
   const int dim=p_in1.give("COMBINATORIAL_DIM"),
      dim2=p_in2.give("COMBINATORIAL_DIM");
   if (dim != dim2)
      throw std::runtime_error("dimension mismatch");

   const IncidenceMatrix<> VIF1=p_in1.give("VERTICES_IN_FACETS"),
      VIF2=p_in2.give("VERTICES_IN_FACETS");
   const int n_vertices1=VIF1.cols(), n_facets1=VIF1.rows(),
      n_vertices2=VIF2.cols();

   if (vertex1 < 0 || vertex1 >= n_vertices1)
      throw std::runtime_error("first vertex number out of range");
   if (vertex2 < 0 || vertex2 >= n_vertices2)
      throw std::runtime_error("second vertex number out of range");

   const Graph<> G1=p_in1.give("GRAPH.ADJACENCY"),
      G2=p_in2.give("GRAPH.ADJACENCY");

   if (G1.degree(vertex1) != dim)
      throw std::runtime_error("first vertex not simple");
   if (G2.degree(vertex2) != dim)
      throw std::runtime_error("second vertex not simple");

   Array<int> neighbors2(dim,-1);

   // reorder the neighbor vertices in P2 if required
   Entire<Graph<>::adjacent_node_list>::const_iterator nb=entire(G2.adjacent_nodes(vertex2));
   Array<int> permutation;
   if (options["permutation"] >> permutation) {
      if (permutation.size() != dim)
         throw std::runtime_error("wrong permutation size");

      for (Array<int>::const_iterator p_i=permutation.begin(); !nb.at_end(); ++p_i, ++nb) {
         if (*p_i < 0 || *p_i >= dim)
            throw std::runtime_error("permutation element out of range");

         if (neighbors2[*p_i] >= 0)
            throw std::runtime_error("permutation not surjective");

         neighbors2[*p_i] = *nb;
      }
   } else {
      copy(nb, neighbors2.begin());
   }

   perl::Object p_out("Polytope<Rational>");
   p_out.set_description() << "Blending of " << p_in1.name() << " at vertex " << vertex1 << " and " << p_in2.name() << " at vertex " << vertex2;
   if (permutation.empty())
      p_out.append_description() << '\n';
   else
      p_out.append_description() << " permuted with [" << permutation << "]\n";

   // initialize as a block-diagonal matrix
   IncidenceMatrix<> VIF_out=diag(VIF1,VIF2);

   // Facets to be glued are identified via opposite neighbors of vertex{1,2}
   // (that is, not belonging to them).
   // Since both vertices are simple, this mapping is unambigiuos
   Array<int>::const_iterator nb2=neighbors2.begin();
   for (nb=entire(G1.adjacent_nodes(vertex1)); !nb.at_end(); ++nb, ++nb2) {
      int f1=(VIF1.col(vertex1) - VIF1.col(*nb)).front(),
         f2=(VIF2.col(vertex2) - VIF2.col(*nb2)).front() + n_facets1;
      VIF_out.row(f1) += VIF_out.row(f2);
      VIF_out.row(f2).clear();
   }

   // get rid of the vertices
   VIF_out.col(vertex1).clear();
   VIF_out.col(vertex2+n_vertices1).clear();
   VIF_out.squeeze();

   p_out.take("COMBINATORIAL_DIM") << dim;
   p_out.take("N_VERTICES") << n_vertices1+n_vertices2-2;
   p_out.take("VERTICES_IN_FACETS") << VIF_out;

   if (options["relabel"]) {
      std::vector<std::string> labels1(n_vertices1), labels2(n_vertices2),
         labels_out(n_vertices1+n_vertices2-2);
      read_labels(p_in1, "VERTEX_LABELS", labels1);
      read_labels(p_in2, "VERTEX_LABELS", labels2);

      const std::string tick="'";
      copy(entire(concatenate(select(labels1, ~scalar2set(vertex1)),
                              attach_operation(select(labels2, ~scalar2set(vertex2)),
                                               constant(tick), operations::add()))),
           labels_out.begin());

      p_out.take("VERTEX_LABELS") << labels_out;
   }

   return p_out;
}

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Compute the blending of two polyhedra at simple vertices."
                  "# This is a slightly less standard construction."
                  "# A vertex is __simple__ if its vertex figure is a simplex."
                  "# "
                  "# Moving a vertex //v// of a bounded polytope to infinity yields an unbounded polyhedron"
                  "# with all edges through //v// becoming mutually parallel rays.  Do this to both"
                  "# input polytopes //P1// and //P2// at simple vertices //v1// and //v2//, respectively."
                  "# Up to an affine transformation one can assume that the orthogonal projections"
                  "# of //P1// and //P2// in direction //v1// and //v2//, respectively, are mutually congruent."
                  "# "
                  "# Any bijection b from the set of edges through //v1// to the edges through //v2//"
                  "# uniquely defines a way of glueing the unbounded polyhedra to obtain a new bounded"
                  "# polytope, the __blending__ with respect to b. The bijection is specified as a //permutation//"
                  "# of indices 0 1 2 etc. The default permutation is the identity."
                  "# "
                  "# The number of vertices of the blending is the sum of the numbers of vertices of the"
                  "# input polytopes minus 2.  The number of facets is the sum of the numbers of facets"
                  "# of the input polytopes minus the dimension."
                  "# "
                  "# The resulting polytope is described only combinatorially."
                  "# @author Kerstin Fritzsche (initial version)"
                  "# @param Polytope P1"
                  "# @param Int v1 the index of the first vertex"
                  "# @param Polytope P2"
                  "# @param Int v2 the index of the second vertex"
                  "# @option Array<Int> permutation"
                  "# @option Bool relabel copy vertex labels from the original polytope"
                  "# @return Polytope",
                  &blending, "blending(Polytope $ Polytope $ { permutation => undef, relabel => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
