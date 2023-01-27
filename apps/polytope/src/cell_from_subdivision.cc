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
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"
#include "polymake/common/labels.h"

namespace polymake { namespace polytope {

namespace {

template <typename Scalar, typename _Set>
BigObject full_dim_cell(BigObject p_in, const GenericSet<_Set, Int>& cell_verts, OptionSet options)
{
   BigObject p_out("Polytope", mlist<Scalar>());

   if (p_in.exists("CONE_DIM")) {
      const Int dim = p_in.call_method("DIM");
      p_out.take("CONE_DIM") << dim+1;
   }
   const Matrix<Scalar> V=p_in.give("VERTICES");
   const Matrix<Scalar> L=p_in.give("LINEALITY_SPACE");
   p_out.take("VERTICES") << V.minor(cell_verts.top(), All);
   p_out.take("LINEALITY_SPACE") << L;

   /* writing AFFINE_HULL without FACETS is not allowed (see #519)
      if (p_in.exists("AFFINE_HULL")) {
      const Matrix<Scalar> AH=p_in.give("AFFINE_HULL");
      p_out.take("AFFINE_HULL") << AH;
      }
   */

   if (!options["no_labels"]) {
      const Int n_vertices = V.rows();
      std::vector<std::string> labels = common::read_labels(p_in, "VERTEX_LABELS", n_vertices);
      const Array<std::string> labels_out(select(labels, cell_verts.top()));
      p_out.take("VERTEX_LABELS") << labels_out;
   }

   return p_out;
}

}

template <typename Scalar>
BigObject cell_from_subdivision(BigObject p_in, Int cell_number, OptionSet options)
{
   const IncidenceMatrix<> subdivision = p_in.give("POLYTOPAL_SUBDIVISION.MAXIMAL_CELLS");
   if (cell_number < 0 || cell_number >= subdivision.rows())
      throw std::runtime_error("cell number out of range");

   BigObject p_out = full_dim_cell<Scalar>(p_in, subdivision[cell_number], options);
   p_out.set_description() << "cell " << cell_number << " of subdivision of " << p_in.name() << endl;
   return p_out;
}

template<typename Scalar>
BigObject cells_from_subdivision(BigObject p_in, const Set<Int>& cells, OptionSet options)
{
   const IncidenceMatrix<> subdivision = p_in.give("POLYTOPAL_SUBDIVISION.MAXIMAL_CELLS");
   Set<Int> cell_verts;
   for (auto i=entire(cells); !i.at_end(); ++i) {
      Int cell_number = *i;
      if (cell_number < 0 || cell_number >= subdivision.rows())
         throw std::runtime_error("cell number out of range");
      cell_verts += subdivision[cell_number];
   }
   BigObject p_out = full_dim_cell<Scalar>(p_in, cell_verts, options);
   p_out.set_description() << "cells " << cells << " of subdivision of " << p_in.name() << endl;
   return p_out;
}


UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Extract the given //cell// of the subdivision of a polyhedron and write it as a new polyhedron."
                          "# @param Polytope P"
                          "# @param Int cell"
                          "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytope. default: 0"
                          "# @return Polytope"
                          "# @example [application fan]"
                          "# First we create a nice subdivision for our favourite 2-polytope, the square:"
                          "# > $p = cube(2);"
                          "# > $p->POLYTOPAL_SUBDIVISION(MAXIMAL_CELLS=>[[0,1,3],[1,2,3]]);"
                          "# Then we extract the [1,2,3]-cell, copying the vertex labels."
                          "# > $c = cell_from_subdivision($p,1);"
                          "# > print $c->VERTICES;"
                          "# | 1 1 -1"
                          "# | 1 -1 1"
                          "# | 1 1 1"
                          "# > print $c->VERTEX_LABELS;"
                          "# | 1 2 3",
                          "cell_from_subdivision<Scalar>(Polytope<Scalar> $ { no_labels => 0})");

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Extract the given //cells// of the subdivision of a polyhedron and create a"
                          "# new polyhedron that has as vertices the vertices of the cells."
                          "# @param Polytope<Scalar> P"
                          "# @param Set<Int> cells"
                          "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytope. default: 0"
                          "# @return Polytope<Scalar>"
                          "# @example [application fan]"
                          "# First we create a nice subdivision for a small polytope:"
                          "# > $p = new Polytope(VERTICES=>[[1,0,0],[1,0,1],[1,1,0],[1,1,1],[1,3/2,1/2]]);"
                          "# > $p->POLYTOPAL_SUBDIVISION(MAXIMAL_CELLS=>[[0,1,3],[1,2,3],[2,3,4]]);"
                          "# Then we create the polytope that has as vertices the vertices from cell 1 and 2,"
                          "# while keeping their labels."
                          "# > $c = cells_from_subdivision($p,[1,2]);"
                          "# > print $c->VERTICES;"
                          "# | 1 0 1"
                          "# | 1 1 0"
                          "# | 1 1 1"
                          "# | 1 3/2 1/2"
                          "# > print $c->VERTEX_LABELS;"
                          "# | 1 2 3 4",
                          "cells_from_subdivision<Scalar>(Polytope<Scalar> $ { no_labels => 0})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
