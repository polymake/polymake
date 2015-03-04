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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {

perl::Object lattice_pyramid(perl::Object p_in, const Rational& z, const Vector<Rational>& v, perl::OptionSet options)
{
   const bool pointed=p_in.give("POINTED");
   if (!pointed)
      throw std::runtime_error("lattice_bipyramid: input polyhedron not pointed");

   const bool relabel = options["relabel"];

   if (z==0)
      throw std::runtime_error("lattice_pyramid: z must be non-zero");
  
   int n_vertices=0;
   perl::Object p_out("Polytope<Rational>");
   p_out.set_description() << "lattice pyramid over " << p_in.name() << endl;

   if (p_in.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
      n_vertices=VIF.cols();
      const int n_facets=VIF.rows();
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
   const Matrix<Rational> empty;
   p_out.take("LINEALITY_SPACE") << empty;

   if (relabel) {
      std::vector<std::string> labels(n_vertices);
      read_labels(p_in, "VERTEX_LABELS", labels);
      labels.resize(n_vertices+1);
      labels[n_vertices]="Apex";
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
                  "# @option Bool relabel copy the original vertex labels,"
                  "#   label the new top vertex with \"Apex\"."
                  "# @return Polytope",
                  &lattice_pyramid, "lattice_pyramid(Polytope; $=1, Vector<Rational>=$_[0]->VERTICES->row(0), { relabel => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
