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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {

template<typename Scalar>
perl::Object pyramid(perl::Object p_in, const Scalar& z, perl::OptionSet options)
{
   const bool noc = options["noc"],
      relabel = options["relabel"];

   if (z==0 && !noc)
      throw std::runtime_error("pyramid: z must be non-zero");

   int n_vertices=0;
   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.set_description() << "pyramid over " << p_in.name() << endl;

   if (noc || p_in.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
      n_vertices=VIF.cols();
      const int n_facets=VIF.rows();
      const IncidenceMatrix<> VIF_out= (VIF | sequence(0,n_facets))     // original vertices + the new top vertex
         / sequence(0, n_vertices);     // original polytope becomes the bottom facet

      p_out.take("N_VERTICES") << n_vertices+1;
      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }
   if (noc) {
      if (p_in.exists("COMBINATORIAL_DIM")) {
         const int dim=p_in.give("COMBINATORIAL_DIM");
         p_out.take("COMBINATORIAL_DIM") << dim+1;
      }
   } else {
      const bool pointed=p_in.give("POINTED");
      if (!pointed)
         throw std::runtime_error("pyramid: input polyhedron not pointed");

      const Matrix<Scalar> V=p_in.give("VERTICES");
      n_vertices=V.rows();
      const Vector<Scalar> z0= p_in.give("BOUNDED") ? p_in.give("VERTEX_BARYCENTER") : p_in.give("REL_INT_POINT");
      p_out.take("VERTICES") << (V | zero_vector<Scalar>()) /
         (z0 | z);
      const Matrix<Scalar> empty;
      p_out.take("LINEALITY_SPACE") << empty;
   }

   if (relabel) {
      std::vector<std::string> labels(n_vertices+1);
      read_labels(p_in, "VERTEX_LABELS", labels);
      labels[n_vertices]="Apex";
      p_out.take("VERTEX_LABELS") << labels;
   }

   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Make a pyramid over a polyhedron."
                          "# The pyramid is the convex hull of the input polyhedron //P// and a point //v//"
                          "# outside the affine span of //P//. For bounded polyhedra, the projection of //v//"
                          "# to the affine span of //P// coincides with the vertex barycenter of //P//."
                          "# @param Polytope P"
                          "# @param Scalar z is the distance between the vertex barycenter and //v//,"
                          "#   default value is 1."
                          "# @option Bool noc don't compute new coordinates, produce purely combinatorial description."
                          "# @option Bool relabel copy vertex labels from the original polytope,"
                          "#   label the new top vertex with \"Apex\"."
                          "# @return Polytope",
                          "pyramid<Scalar>(Polytope<type_upgrade<Scalar>>; type_upgrade<Scalar>=1, { noc => 0, relabel => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
