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
#include "polymake/Array.h"
#include "polymake/Graph.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/polytope/to_interface.h"

namespace polymake { namespace polytope {

template<typename Scalar>
perl::Object vertex_figure(perl::Object p_in, int v_cut_off, perl::OptionSet options)
{
   if (options.exists("cutoff") && options.exists("no_coordinates")) 
       throw std::runtime_error("vertex_figure: cannot specify cutoff and no_coordinates options simultaneously");

   const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
   const Graph<> G=p_in.give("GRAPH.ADJACENCY");
   const int n_vertices=VIF.cols();

   if (v_cut_off < 0 || v_cut_off >= n_vertices) {
      throw std::runtime_error("vertex_figure: vertex number out of range");
   }

   IncidenceMatrix<> VIF_out=VIF.minor(VIF.col(v_cut_off), G.adjacent_nodes(v_cut_off));

   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.set_description() << "vertex figure of " << p_in.name() << " at vertex " << v_cut_off << endl;

   p_out.take("VERTICES_IN_FACETS") << VIF_out;

   if (options["no_coordinates"]) {
      if (p_in.exists("COMBINATORIAL_DIM")) {
         const int dim=p_in.give("COMBINATORIAL_DIM");
         p_out.take("COMBINATORIAL_DIM") << dim-1;
      }
   } else {
      
       Scalar cutoff_factor = Scalar(1)/Scalar(2);
       if (options["cutoff"] >> cutoff_factor && (cutoff_factor<=0 || cutoff_factor>=1))
           throw std::runtime_error("vertex_figure: cutoff factor must be within (0,1]");

      const Matrix<Scalar> V=p_in.give("VERTICES"),
                           F=p_in.give("FACETS"),
                           AH=p_in.give("AFFINE_HULL");

      to_interface::solver<Scalar> S;
      Matrix<Scalar> orth(AH);
      if (orth.cols()) orth.col(0).fill(0);
      Matrix<Scalar> basis(G.out_degree(v_cut_off), V.cols());

      const bool simple_vertex=basis.rows()+AH.rows()==V.cols()-1;
      typename Rows< Matrix<Scalar> >::iterator b=rows(basis).begin();
      for (Entire< Graph<>::adjacent_node_list >::const_iterator nb_v=entire(G.adjacent_nodes(v_cut_off)); !nb_v.at_end(); ++nb_v, ++b)
         *b = (1-cutoff_factor) * V[v_cut_off] + cutoff_factor * V[*nb_v];

      Vector<Scalar> cutting_plane;
      if (simple_vertex) {
         // calculate a hyperplane thru the basis points
         cutting_plane=null_space(basis/orth)[0];
      } else {
         // look for a valid separating hyperplane furthest from the vertex being cut off
         cutting_plane=S.solve_lp(basis, orth, V[v_cut_off], false).second;
      }

      p_out.take("FACETS") << F.minor(VIF.col(v_cut_off),All);
      p_out.take("AFFINE_HULL") << AH / cutting_plane;  //FIXME
   }

   if (options["relabel"]) {
      Array<std::string> labels(n_vertices);
      read_labels(p_in, "VERTEX_LABELS", labels);
      Array<std::string> labels_out(select(labels, G.adjacent_nodes(v_cut_off)));
      p_out.take("VERTEX_LABELS") << labels_out;
   }

   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct the vertex figure of the vertex //n// of a polyhedron."
                          "# The vertex figure is dual to a facet of the dual polytope."
                          "# @param Polytope p"
                          "# @param Int n number of the chosen vertex"
                          "# @option Scalar cutoff controls the exact location of the cutting hyperplane."
                          "#   It should lie between 0 and 1."
                          "#   Value 0 would let the hyperplane go through the chosen vertex,"
                          "#   thus degenerating the vertex figure to a single point."
                          "#   Value 1 would let the hyperplane touch the nearest neighbor vertex of a polyhedron."
                          "#   Default value is 1/2."
                          "# @option Bool no_coordinates skip the coordinates computation, producing a pure combinatorial description."
                          "# @option Bool relabel inherit vertex labels from the corresponding neighbor vertices of the original polytope."
                          "# @return Polytope"
                          "# @example This produces a vertex figure of one vertex of a 3-dimensional cube with the origin as its center"
                          "# and side length 2. The result is a 2-simplex."
                          "# > $p = vertex_figure(cube(3),5);"
                          "# > print $p->VERTICES;"
                          "# | 1 1 -1 0"
                          "# | 1 1 0 1",
                          "vertex_figure<Scalar>(Polytope<Scalar> $ {cutoff => undef, no_coordinates => undef, relabel => 0})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
