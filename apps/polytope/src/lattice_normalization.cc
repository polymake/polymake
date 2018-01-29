/* Copyright (c) 1997-2018
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
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Smith_normal_form.h"
#include "polymake/linalg.h"
#include "polymake/common/lattice_tools.h"

namespace polymake { namespace polytope {
namespace {

perl::Object apply_lattice_normalization(perl::Object p, bool ambient, bool store_transform)
{
  if (!p.give("LATTICE") || !p.give("BOUNDED"))     // only for lattice polytopes
    throw std::runtime_error("not a lattice polytope");
        
  const Matrix<Integer> V=p.give("VERTICES");
  SmithNormalForm<Integer> SNF = smith_normal_form(V);
        
  for (int i = 0; i < SNF.rank; ++i) { // adjust orientation of the transformation
    if (SNF.form(i,i) < 0) {           // all entries of M >= 0
      SNF.form(i,i).negate();
      SNF.left_companion.col(i).negate();
    }
  }
  if (det(SNF.right_companion) < 0) {             // det(R) > 0
    SNF.left_companion.col(SNF.rank-1).negate();
    SNF.right_companion.row(SNF.rank-1).negate();
  }
        
  perl::Object q("Polytope<Rational>");
  Matrix<Rational> F;
  if (ambient) {
    q.set_description() << "transformation of "
                        << p.name()
                        << " in the lattice spanned by Z^n intersected with the affine hull of "
                        << p.name()
                        << " to a full dimensional polytope in the lattice Z^n" << endl;
    q.take("VERTICES") <<  (SNF.left_companion * SNF.form).minor(All,range(0, SNF.rank-1));
    if (p.lookup("FACETS") >> F) {
      q.take("FACETS") << common::divide_by_gcd((common::primitive(F) * T(SNF.right_companion)).minor(All, range(0, SNF.rank-1)));
    }
    if ( store_transform ) {
      Matrix<Integer> RLP;
      if (q.get_attachment("REVERSE_LATTICE_PROJECTION") >> RLP)
        RLP =  SNF.right_companion.minor(range(0, SNF.rank-1),All) * RLP;
      else
        RLP =  SNF.right_companion.minor(range(0, SNF.rank-1),All);
      q.attach("REVERSE_LATTICE_PROJECTION") << RLP;
    }
  } else {
    q.set_description() << "transformation of "
                        << p.name()
                        << " in the lattice spanned by the vertices of "
                        << p.name()
                        << " to a full dimensional polytope in the lattice Z^n" << endl;
    q.take("VERTICES") <<  SNF.left_companion.minor(All, range(0, SNF.rank-1));
    if (p.lookup("FACETS") >> F) {
      q.take("FACETS") << common::divide_by_gcd((common::primitive(F) * T(SNF.right_companion) * T(SNF.form)).minor(All, range(0, SNF.rank-1)));
    }
    if ( store_transform ) {
      Matrix<Integer> RLP;
      if (q.get_attachment("REVERSE_LATTICE_PROJECTION") >> RLP)
        RLP =  (SNF.form * SNF.right_companion).minor(range(0, SNF.rank-1),All) * RLP;
      else
        RLP =  (SNF.form * SNF.right_companion).minor(range(0, SNF.rank-1),All);
      q.attach("REVERSE_LATTICE_PROJECTION") << RLP;
    }
  }
  q.take("LATTICE") << true;
  q.take("BOUNDED") << true;

  Array<std::string> labels;
  if (p.lookup("VERTEX_LABELS") >> labels) q.take("VERTEX_LABELS") << labels;
  if (p.lookup("FACET_LABELS")  >> labels) q.take("FACET_LABELS")  << labels;
  return q;
}
}
    
perl::Object ambient_lattice_normalization(perl::Object p , perl::OptionSet options)
{
  const bool store_transform=options["store_transform"];
  return apply_lattice_normalization(p, true, store_transform);
}
    
perl::Object vertex_lattice_normalization(perl::Object p, perl::OptionSet options)
{
  const bool store_transform=options["store_transform"];
  return apply_lattice_normalization(p, false, store_transform);
}
    
Matrix<Integer> induced_lattice_basis(perl::Object p)
{
  if (!p.give("LATTICE") || !p.give("BOUNDED"))     // only for lattice polytopes
    throw std::runtime_error("not a lattice polytope");
      
  const Matrix<Integer> V=p.give("VERTICES");
  SmithNormalForm<Integer> SNF = smith_normal_form(V);
  return (SNF.form * SNF.right_companion).minor(range(1, SNF.rank-1),All);
}

UserFunction4perl("# @category Transformations"
                  "# Transform to a full-dimensional polytope while preserving"
                  "# the ambient lattice Z^n"
                  "# @param Polytope p the input polytope,"
                  "# @option Bool store_transform store the reverse transformation as an attachement"
                  "# @return Polytope - the transformed polytope defined by its vertices."
                  "#  Facets are only written if available in //p//."
                  "# @example Consider a line segment embedded in 2-space containing three lattice points:"
                  "# > $p = new Polytope(VERTICES=>[[1,0,0],[1,2,2]]);"
                  "# > print ambient_lattice_normalization($p)->VERTICES;"
                  "# | 1 0"
                  "# | 1 2"
                  "# The ambient lattice of the projection equals the intersection of the affine hull of $p with Z^2."
                  "# @example Another line segment containing only two lattice points:"
                  "# > $p = new Polytope(VERTICES=>[[1,0,0],[1,1,2]]);"
                  "# > $P = ambient_lattice_normalization($p,store_transform=>1);"
                  "# > print $P->VERTICES;"
                  "# | 1 0"
                  "# | 1 1"
                  "# To get the transformation, do the following:"
                  "# > $M = $P->get_attachment('REVERSE_LATTICE_PROJECTION');"
                  "# > print $M;"
                  "# | 1 0 0"
                  "# | 0 1 2"
                  "# > print $P->VERTICES * $M;"
                  "# | 1 0 0"
                  "# | 1 1 2",
                  &ambient_lattice_normalization, "ambient_lattice_normalization(Polytope<Rational> {store_transform => 0})");

UserFunction4perl("# @category Transformations"
                  "# Transform to a full-dimensional polytope while preserving"
                  "# the lattice spanned by vertices"
                  "# induced lattice of new vertices = Z^dim"
                  "# @param Polytope p the input polytope,"
                  "# @option Bool store_transform store the reverse transformation as an attachement"
                  "# @return Polytope - the transformed polytope defined by its vertices."
                  "#  Facets are only written if available in //p//.",
                  &vertex_lattice_normalization, "vertex_lattice_normalization(Polytope<Rational> {store_transform => 0})");

UserFunction4perl("# @category Geometry"
                  "# Returns a basis of the affine lattice spanned by the vertices"
                  "# @param Polytope p the input polytope"
                  "# @return Matrix<Integer> - the lattice basis."
                  "# @example The vertices of the 2-simplex span all of Z^2..."
                  "# > print induced_lattice_basis(simplex(2));"
                  "# | 0 1 0"
                  "# | 0 0 1"
                  "# ...but if we scale it with 2, we get only every second lattice point."
                  "# > print induced_lattice_basis(scale(simplex(2),2));"
                  "# | 0 2 0"
                  "# | 0 0 2",
                  &induced_lattice_basis, "induced_lattice_basis(Polytope<Rational>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:2
// indent-tabs-mode:nil
// End:
