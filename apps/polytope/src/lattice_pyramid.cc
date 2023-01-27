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
#include "polymake/vector"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/common/labels.h"

namespace polymake { namespace polytope {

BigObject lattice_pyramid(BigObject p_in, const Rational& z, const Vector<Rational>& v, OptionSet options)
{
   const bool pointed=p_in.give("POINTED");
   if (!pointed)
      throw std::runtime_error("lattice_bipyramid: input polyhedron not pointed");

   const bool relabel = !options["no_labels"];

   if (z==0)
      throw std::runtime_error("lattice_pyramid: z must be non-zero");
  
   Int n_vertices = 0;
   BigObject p_out("Polytope<Rational>");
   p_out.set_description() << "lattice pyramid over " << p_in.name() << endl;

   if (p_in.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF = p_in.give("VERTICES_IN_FACETS");
      n_vertices = VIF.cols();
      const Int n_facets = VIF.rows();
      const IncidenceMatrix<> VIF_out= (VIF | sequence(0,n_facets))    // original vertices + the new top vertex
                                     / sequence(0, n_vertices);        // original polytope becomes the bottom facet

      p_out.take("N_VERTICES") << n_vertices+1;
      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }
   const Matrix<Rational> V=p_in.give("VERTICES");
   n_vertices=V.rows();

   if (v.dim() != V.cols())
      throw std::runtime_error("v: wrong dimension");

   p_out.take("VERTICES") << (V | zero_vector<Rational>()) /
                             (v | z);

   if (relabel) {
      std::vector<std::string> labels = common::read_labels(p_in, "VERTEX_LABELS", n_vertices);
      labels.emplace_back("Apex");
      p_out.take("VERTEX_LABELS") << labels;
   }

   return p_out;
}

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Make a lattice pyramid over a polyhedron."
                  "# The pyramid is the convex hull of the input polyhedron //P// and a point //v//"
                  "# outside the affine span of //P//."
                  "# @param Polytope P"
                  "# @param Rational z the height for the apex (//v//,//z//), default value is 1."
                  "# @param Vector v the lattice point to use as apex, default is the first vertex of //P//."
                  "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytope. default: 0"
                  "#   label the new top vertex with \"Apex\"."
                  "# @return Polytope"
                  "# @example To create the pyramid of height 5 over a square and keep the vertex labels, do this:"
                  "# > $p = lattice_pyramid(cube(2),5,new Vector(1,0,0));"
                  "# > print $p->VERTICES;"
                  "# | 1 -1 -1 0"
                  "# | 1 1 -1 0"
                  "# | 1 -1 1 0"
                  "# | 1 1 1 0"
                  "# | 1 0 0 5"
                  "# > print $p->VERTEX_LABELS;"
                  "# | 0 1 2 3 Apex",
                  &lattice_pyramid, "lattice_pyramid(Polytope; $=1, Vector<Rational>=$_[0]->VERTICES->row(0), { no_labels => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
