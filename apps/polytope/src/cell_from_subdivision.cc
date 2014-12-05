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
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace polytope {

namespace {

template <typename Scalar, typename _Set>
perl::Object full_dim_cell(perl::Object p_in, const GenericSet<_Set, int>& cell_verts, perl::OptionSet options)
{
   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));

   if (p_in.exists("CONE_DIM")) {
      const int dim=p_in.CallPolymakeMethod("DIM");
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

   if (options["relabel"]) {
      const int n_vertices = V.rows();
      Array<std::string> labels(n_vertices);
      read_labels(p_in, "VERTEX_LABELS", labels);
      const Array<std::string> labels_out(select(labels, cell_verts.top()));
      p_out.take("VERTEX_LABELS") << labels_out;
   }
  
   return p_out;
}

}

template <typename Scalar>
perl::Object cell_from_subdivision(perl::Object p_in, int cell_number, perl::OptionSet options)
{
  const IncidenceMatrix<> subdivision = p_in.give("POLYTOPAL_SUBDIVISION.MAXIMAL_CELLS");
  if (cell_number < 0 || cell_number >= subdivision.rows())
    throw std::runtime_error("cell number out of range");
  
  perl::Object p_out = full_dim_cell<Scalar>(p_in, subdivision[cell_number], options);
  p_out.set_description() << "cell " << cell_number << " of subdivision of " << p_in.name() << endl;
  return p_out;
}

template<typename Scalar>
perl::Object cells_from_subdivision(perl::Object p_in, const Set<int>& cells, perl::OptionSet options)
{
  const IncidenceMatrix<> subdivision = p_in.give("POLYTOPAL_SUBDIVISION.MAXIMAL_CELLS");
  Set<int> cell_verts;
  for (Entire< Set<int> >::const_iterator i=entire(cells); !i.at_end(); ++i) {
    int cell_number = *i;
    if (cell_number < 0 || cell_number >= subdivision.rows())
      throw std::runtime_error("cell number out of range");
    cell_verts += subdivision[cell_number];
  }
  perl::Object p_out = full_dim_cell<Scalar>(p_in, cell_verts, options);
  p_out.set_description() << "cells " << cells << " of subdivision of " << p_in.name() << endl;
  return p_out;
} 


UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Extract the given //cell// of the subdivision of a polyhedron and write it as a new polyhedron."
                          "# @param Polytope P"
                          "# @param Int cell"
                          "# @option Bool relabel copy the vertex labels from the original polytope"
                          "# @return Polytope",
                          "cell_from_subdivision<Scalar>(Polytope<Scalar> $ { relabel => 0})");

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Extract the given //cells// of the subdivision of a polyhedron and write their union"
                          "# as a new polyhedron."
                          "# @param Polytope<Scalar> P"
                          "# @param Set<Int> cells"
                          "# @option Bool relabel copy the vertex labels from the original polytope"
                          "# @return Polytope<Scalar>",
                          "cells_from_subdivision<Scalar>(Polytope<Scalar> $ { relabel => 0})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
