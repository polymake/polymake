/* Copyright (c) 1997-2020
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
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/group/permlib.h"
#include "polymake/polytope/poly2lp.h"
#include "polymake/Bitset.h"
#include "polymake/polytope/poly2lp.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace polymake { namespace polytope {

typedef Bitset SetType;

BigObject symmetrized_foldable_max_signature_ilp(Int d,
                                                 const Matrix<Rational>& points,
                                                 const Array<SetType>& facets,
                                                 const Rational& vol,
                                                 const Array<Array<Int>>& generators,
                                                 const SparseMatrix<Rational>& symmetrized_foldable_cocircuit_equations)
{
   // ONLY for lattice polytopes!
   const Int
      n2 = symmetrized_foldable_cocircuit_equations.cols(), 
      // this matrix contains one black and one white copy of each orbit
      // ideally, black simplices should have even column indices, white simplices odd column indices
      // in reality, this is the other way around because of the inhomogeneous zero-th column
      n = n2/2;
   
   const group::PermlibGroup sym_group(generators);
   SparseMatrix<Integer> selection_matrix(n, n2+1);

   Vector<Integer> volume_vect(n2+1);
   auto vit = volume_vect.begin();
   *vit = -Integer::fac(d) * vol;
   ++vit;

   Int s = 0;
   for (auto fit = entire(facets); !fit.at_end(); ++fit, ++vit, ++s) {
      // points required to have integer coordinates!
      const Rational rational_facet_vol = abs(det(points.minor(*fit, All)));
      assert (denominator(rational_facet_vol) == 1);
      const Integer facet_vol = convert_to<Integer>(rational_facet_vol);
      *vit = facet_vol; // black facet
      ++vit;
      *vit = facet_vol; // white facet
      selection_matrix(s, 2*s+1) = selection_matrix(s, 2*s+2) = -1; // either black or white
      selection_matrix(s, 0) = sym_group.orbit(Set<Int>(*fit)).size();
   }

   const SparseMatrix<Integer> 
      Inequalities = (zero_vector<Integer>(n2) | unit_matrix<Integer>(n2)) /
                      selection_matrix,
      Equations    = (zero_vector<Integer>(symmetrized_foldable_cocircuit_equations.rows()) | Matrix<Integer>(symmetrized_foldable_cocircuit_equations)) /
                     volume_vect;
   
   // signature = absolute difference of normalized volumes of black minus white maximal simplices
   // (provided that normalized volume is odd)
   for (Int i = 0; i < n; ++i) {
      if (volume_vect[2*i+1].even())
         volume_vect[2*i+1] = volume_vect[2*i+2] = 0;
      else
         volume_vect[2*i+2].negate();
   }
   
   BigObject q("Polytope<Rational>",
               "FEASIBLE", true,
               "INEQUALITIES", Inequalities,
               "EQUATIONS", Equations);
   BigObject lp = q.add("LP", "LINEAR_OBJECTIVE", 0 | volume_vect.slice(range_from(1)));
   lp.attach("INTEGER_VARIABLES") << Array<bool>(n2, true);
   return q;
}

Integer symmetrized_foldable_max_signature_upper_bound(Int d, 
                                                       const Matrix<Rational>& points, 
                                                       const Array<SetType>& facets, 
                                                       const Rational& vol, 
                                                       const Array<Array<Int>>& generators,
                                                       const SparseMatrix<Rational>& symmetrized_foldable_cocircuit_equations)
{
   BigObject q = symmetrized_foldable_max_signature_ilp(d, points, facets, vol, generators, symmetrized_foldable_cocircuit_equations);
   const Rational sll=q.give("LP.MAXIMAL_VALUE");
   const Integer int_sll = convert_to<Integer>(sll); // rounding down
   return int_sll;
}



UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Set up an ILP whose MAXIMAL_VALUE is the maximal signature of a foldable triangulation of a polytope, point configuration or quotient manifold"
                  "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                  "# @param Matrix points the input points or vertices "
                  "# @param Rational volume the volume of the convex hull "
                  "# @param Array<Array<Int>> generators the generators of the symmetry group "
                  "# @param SparseMatrix symmetrized_foldable_cocircuit_equations the matrix of symmetrized cocircuit equations "
                  "# @return LinearProgram<Rational> an ILP that provides the result",
                  &symmetrized_foldable_max_signature_ilp,
                  "symmetrized_foldable_max_signature_ilp($ Matrix Array<Bitset> $ Array<Array<Int>> SparseMatrix)");

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Calculate the LP relaxation upper bound to the maximal signature of a foldable triangulation of polytope, point configuration or quotient manifold"
                  "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                  "# @param Matrix points the input points or vertices "
                  "# @param Rational volume the volume of the convex hull "
                  "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                  "# @return Integer the optimal value of an LP that provides a bound",
                  &symmetrized_foldable_max_signature_upper_bound,
                  "symmetrized_foldable_max_signature_upper_bound($ Matrix Array<Bitset> $ Array<Array<Int>> SparseMatrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

