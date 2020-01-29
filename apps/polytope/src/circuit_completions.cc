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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

namespace {

// This implements Theorem 8.1.8 in de Loera, Rambau, Santos   
template<typename Scalar>
std::tuple<Set<Int>, Set<Int>, Set<Int>>
circuit_signature(const Matrix<Scalar>& C, Int hull_dim)
{
   const Int rank = C.rows()-hull_dim;
   bool counter_is_even = true;
   Set<Int> plus, minus, zero;
   for (Int i=0; i < rank; ++i, counter_is_even = !counter_is_even) {
      Int s = sign(det(C.minor(sequence(0,C.rows()) - scalar2set(i), All)));
      if (0 == s) {
         zero += i; continue;
      }
      if (!counter_is_even) s = -s;
      if (s > 0)
         plus += i;
      else
         minus += i;
   }
   return std::make_tuple(plus, minus, zero);
}

template<typename Scalar>
bool
completes_circuit(const Matrix<Scalar>& C, Int n_lhs, Int hull_dim)
{
   Set<Int> plus, minus, zero;
   std::tie(plus, minus, zero) = circuit_signature(C, hull_dim);
   return (plus  == sequence(0,n_lhs) ||
           minus == sequence(0,n_lhs));
}
   
} // end anonymous namespace

template<typename Scalar, typename Matrix1, typename Matrix2, typename Matrix3>
Array<Set<Int>>
circuit_completions_impl(const GenericMatrix<Matrix1,Scalar>& rhs,
                         const GenericMatrix<Matrix2,Scalar>& lhs_candidates,
                         const GenericMatrix<Matrix3,Scalar>& affine_hull_gens)
{
   const auto c_plus (affine_hull_gens / rhs);
   const Int
      ambient_dim = rhs.cols(),
      combinatorial_dim = ambient_dim - affine_hull_gens.rows(),
      n_lhs = combinatorial_dim+1-rhs.rows();

   std::vector<Set<Int>> left_hand_sides;
   for (auto c_minus = entire(all_subsets_of_k(sequence(0, lhs_candidates.rows()), n_lhs)); !c_minus.at_end(); ++c_minus) {
      if (completes_circuit(Matrix<Scalar>(lhs_candidates.minor(*c_minus, All) / c_plus), n_lhs, affine_hull_gens.rows()))
         left_hand_sides.emplace_back(Set<Int>(*c_minus));
   }
   return Array<Set<Int>>(left_hand_sides.size(), entire(left_hand_sides));
}

FunctionTemplate4perl("circuit_completions_impl(Matrix,Matrix,Matrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
