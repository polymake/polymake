/* Copyright (c) 1997-2022
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
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/common/labels.h"

namespace polymake { namespace polytope {

BigObject lattice_bipyramid_vv(BigObject p_in, const Vector<Rational>& v, const Vector<Rational>& v_prime, const Rational& z, const Rational& z_prime, OptionSet options)
{
   const bool pointed=p_in.give("POINTED");
   if (!pointed)
      throw std::runtime_error("lattice_bipyramid: input polyhedron not pointed");

   if (z*z_prime >= 0)
      throw std::runtime_error("lattice_bipyramid: z and z' must have opposite signs and be non-zero");

   BigObject p_out("Polytope<Rational>");
   p_out.set_description() << "Lattice Bipyramid over " << p_in.name() << endl;

   const bool relabel = !options["no_labels"];

   Int n_vertices = 0;
   if (p_in.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF = p_in.give("VERTICES_IN_FACETS");
      n_vertices = VIF.cols();
      const Int n_facets = VIF.rows();
      const IncidenceMatrix<> VIF_out = (VIF / VIF) | sequence(0,n_facets) | sequence(n_facets,n_facets);

      p_out.take("N_VERTICES") << n_vertices+2;
      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }

   const Matrix<Rational> V=p_in.give("VERTICES");
   n_vertices=V.rows();

   const Matrix<Rational> V_out=(V | zero_vector<Rational>()) /
                                (v | z) /
                                (v_prime | z_prime);
   p_out.take("VERTICES") << V_out;

   if (relabel) {
      std::vector<std::string> labels = common::read_labels(p_in, "VERTEX_LABELS", n_vertices);
      labels.emplace_back("Apex");
      labels.emplace_back("Apex'");
      p_out.take("VERTEX_LABELS") << labels;
   }
   return p_out;
}

BigObject lattice_bipyramid_v(BigObject p_in, const Vector<Rational>& v, const Rational& z, const Rational& z_prime, OptionSet options)
{
    return lattice_bipyramid_vv(p_in, v, v, z, z_prime, options);
}

BigObject lattice_bipyramid_innerpoint(BigObject p_in, const Rational& z, const Rational& z_prime, OptionSet options)
{
    const Matrix<Rational> v = p_in.give("INTERIOR_LATTICE_POINTS");
    if (is_zero(v)) {
        throw std::runtime_error("lattice_bipyramid: if P is a simplex and no apex is given, P must contain at least one interior lattice point. (And 4ti2 or normaliz must be installed.)");
    }
    const Vector<Rational> v0 = v.row(0);
    return lattice_bipyramid_vv(p_in, v0, v0, z, z_prime, options);
}

BigObject lattice_bipyramid(BigObject p_in, const Rational& z, const Rational& z_prime, OptionSet options)
{
   const bool pointed=p_in.give("POINTED");
   if (!pointed)
      throw std::runtime_error("lattice_bipyramid: input polyhedron not pointed");

    const Int n_vert = p_in.give("N_VERTICES");
    const Int dim = p_in.call_method("DIM");
    if (n_vert > dim+1) {
       const Matrix<Rational> V = p_in.give("VERTICES");
       const IncidenceMatrix<> VIF = p_in.give("VERTICES_IN_FACETS");
       for (auto v_pair=entire(all_subsets_of_k(sequence(0, V.rows()), 2)); !v_pair.at_end(); ++v_pair) {
           const Int v0 = (*v_pair)[0];
           const Int v1 = (*v_pair)[1];
           if ((VIF.col(v0) * VIF.col(v1)).empty()) {
              return lattice_bipyramid_vv(p_in, V.row(v0), V.row(v1), z, z_prime, options);
           }
        }
    }
    return lattice_bipyramid_innerpoint(p_in,z,z_prime,options);
}

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Make a lattice bipyramid over a polyhedron."
                  "# The bipyramid is the convex hull of the input polyhedron //P//"
                  "# and two points (//v//, //z//), (//v_prime//, //z_prime//)"
                  "# on both sides of the affine span of //P//."
                  "# @param Polytope P"
                  "# @param Vector v basis point for the first apex"
                  "# @param Vector v_prime basis for the second apex"
                  "#  If //v_prime// is omitted, //v// will be used for both apices."
                  "#  If both //v// and //v_prime// are omitted, it tries to find two vertices which don't lie in a common facet."
                  "#  If no such vertices can be found or //P// is a simplex, it uses an interior lattice point as"
                  "#  both //v// and //v_prime//."
                  "# @param Rational z height for the first apex, default value is 1"
                  "# @param Rational z_prime height for the second apex, default value is -//z//"
                  "# @option Bool no_labels Do not copy [[VERTEX_LABELS]] from the original polytope. default: 0"
                  "# label the new vertices with \"Apex\" and \"Apex'\"."
                  "# @return Polytope"
                  "# @example To create the bipyramid over a square and keep the vertex labels, do this:"
                  "# > $p = lattice_bipyramid(cube(2),new Vector(1,0,0));"
                  "# > print $p->VERTICES;"
                  "# | 1 -1 -1 0"
                  "# | 1 1 -1 0"
                  "# | 1 -1 1 0"
                  "# | 1 1 1 0"
                  "# | 1 0 0 1"
                  "# | 1 0 0 -1"
                  "# > print $p->VERTEX_LABELS;"
                  "# | 0 1 2 3 Apex Apex'",
                  &lattice_bipyramid_vv, "lattice_bipyramid(Polytope, Vector, Vector; $=1, $=-$_[3], {no_labels => 0})");

Function4perl(&lattice_bipyramid_v, "lattice_bipyramid(Polytope, Vector; $=1, $=-$_[2], {no_labels => 0})");

Function4perl(&lattice_bipyramid, "lattice_bipyramid(Polytope; $=1, $=-$_[1], {no_labels => 0})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
