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
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {

perl::Object lattice_bipyramid_vv(perl::Object p_in, const Vector<Rational>& v, const Vector<Rational>& v_prime, const Rational& z, const Rational& z_prime, perl::OptionSet options)
{
   const bool pointed=p_in.give("POINTED");
   if (!pointed)
      throw std::runtime_error("lattice_bipyramid: input polyhedron not pointed");

   if (z*z_prime >= 0) 
      throw std::runtime_error("lattice_bipyramid: z and z' must have opposite signs and be non-zero");

   perl::Object p_out("Polytope<Rational>");
   p_out.set_description() << "Lattice Bipyramid over " << p_in.name() << endl;

   const bool relabel = options["relabel"];

   int n_vertices=0;
   if (p_in.exists("VERTICES_IN_FACETS")) {
      const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
      n_vertices=VIF.cols();
      const int n_facets=VIF.rows();
      const IncidenceMatrix<> VIF_out= (VIF / VIF) | sequence(0,n_facets) | sequence(n_facets,n_facets);

      p_out.take("N_VERTICES") << n_vertices+2;
      p_out.take("VERTICES_IN_FACETS") << VIF_out;
   }
   
   const Matrix<Rational> V=p_in.give("VERTICES");
   n_vertices=V.rows();

   const Matrix<Rational> V_out=(V | zero_vector<Rational>()) /
                                (v | z) /
                                (v_prime | z_prime);
   p_out.take("VERTICES") << V_out;
   const Matrix<Rational> empty;
   p_out.take("LINEALITY_SPACE") << empty;

   if (relabel) {
      std::vector<std::string> labels(n_vertices);
      read_labels(p_in, "VERTEX_LABELS", labels);
      labels.resize(n_vertices+2);
      labels[n_vertices]="Apex";
      labels[n_vertices+1]="Apex'";
      p_out.take("VERTEX_LABELS") << labels;
   }
   return p_out;
}

perl::Object lattice_bipyramid_v(perl::Object p_in, const Vector<Rational>& v, const Rational& z, const Rational& z_prime, perl::OptionSet options)
{
    return lattice_bipyramid_vv(p_in, v, v, z, z_prime, options);
}

perl::Object lattice_bipyramid_innerpoint(perl::Object p_in, const Rational& z, const Rational& z_prime, perl::OptionSet options)
{
    const Matrix<Rational> v = p_in.give("INTERIOR_LATTICE_POINTS");
    if (is_zero(v)) {
        throw std::runtime_error("lattice_bipyramid: if P is a simplex and no apex is given, P must contain at least one interior lattice point. (And 4ti2 or normaliz must be installed.)");
    }
    const Vector<Rational> v0 = v.row(0);
    return lattice_bipyramid_vv(p_in, v0, v0, z, z_prime, options);
}

perl::Object lattice_bipyramid(perl::Object p_in, const Rational& z, const Rational& z_prime, perl::OptionSet options)
{

   const bool pointed=p_in.give("POINTED");
   if (!pointed)
      throw std::runtime_error("lattice_bipyramid: input polyhedron not pointed");

    const int n_vert = p_in.give("N_VERTICES");
    const int dim = p_in.CallPolymakeMethod("DIM");
    if(n_vert > dim+1) {
        const Matrix<Rational> F = p_in.give("FACETS");
        const Matrix<Rational> V = p_in.give("VERTICES");
        const Vector<Rational> zeros(2);
        Matrix<Rational> vert;
        Matrix<Rational> val;
        for (Entire< Subsets_of_k<const sequence&> >::const_iterator v=entire(all_subsets_of_k(sequence(0,V.rows()),2)); !v.at_end(); ++v) {
            vert = V.minor(*v,All);
            val = F * T(vert);
            for (Entire< Rows< Matrix<Rational> > >::const_iterator row=entire(rows(val)); !row.at_end(); ++row) {
                if (*row == zeros) {
                    goto endloop;
                }
            }
            return lattice_bipyramid_vv(p_in, vert.row(0), vert.row(1), z, z_prime, options);
            endloop:;
        }
        //throw std::runtime_error("lattice_bipyramid: could not find two vertices !!!");
        return lattice_bipyramid_innerpoint(p_in,z,z_prime,options);
    } else {
        return lattice_bipyramid_innerpoint(p_in,z,z_prime,options);
    }
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
                  "# @option Bool relabel copy the vertex labels from the original polytope,"
                  "# label the new vertices with \"Apex\" and \"Apex'\"."
                  "# @return Polytope",
                  &lattice_bipyramid_vv, "lattice_bipyramid(Polytope, Vector, Vector; $=1, $=-$_[3], {relabel => 0})");

Function4perl(&lattice_bipyramid_v, "lattice_bipyramid(Polytope, Vector; $=1, $=-$_[2], {relabel => 0})");

Function4perl(&lattice_bipyramid, "lattice_bipyramid(Polytope; $=1, $=-$_[1], {relabel => 0})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
