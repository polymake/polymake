/*
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA  02110-1301, USA.

   ---
   Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

   ---
   Copyright (c) 2016-2019
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   Contains functions to compute the affine transform of a cycle 
   */

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace tropical {

/**
 * @brief Computes the sum of two matroid ring cycles 
 * 
 */
template <typename Addition>
perl::Object matroid_ring_sum(perl::Object c1, perl::Object c2)
{
  Array<IncidenceMatrix<> > np1 = c1.give("NESTED_PRESENTATIONS");
  Array<IncidenceMatrix<> > np2 = c2.give("NESTED_PRESENTATIONS");
  Array<int> nc1 = c1.give("NESTED_COEFFICIENTS");
  Array<int> nc2 = c2.give("NESTED_COEFFICIENTS");
  int n_elements = c1.give("N_ELEMENTS");

  Vector<IncidenceMatrix<> > result_presentation(np1);
  Vector<int> result_coefficients(nc1);

  int index = 0;
  for (auto p = entire(np2); !p.at_end(); ++p, ++index) {
    bool found_it = false;
    int other_index =0;
    for (auto other_p = entire(np1); !other_p.at_end(); ++other_p, ++other_index) {
      if (*p == *other_p) {
        // If no exception is thrown, they're equal
        found_it = true;
        result_coefficients[other_index] += nc2[index];
        break;
      }
    }
    if (!found_it) {
      // It's new!
      result_presentation |= *p;
      result_coefficients |= nc2[index];
    }
  }

  // Check for zero entries
  Set<int> supp = support(result_coefficients);

  perl::Object result("MatroidRingCycle", mlist<Addition>());
  result.take("N_ELEMENTS") << n_elements;
  result.take("NESTED_PRESENTATIONS") << result_presentation.slice(supp);
  result.take("NESTED_COEFFICIENTS") << result_coefficients.slice(supp);

  return result;
}

template <typename Addition>
Matrix<Rational> matroid_ring_linear_space(const Array<perl::Object>& cycles)
{
  Matrix<Rational> result;
  // FIXME: misuse of vector concatenation
  Vector<IncidenceMatrix<>> existing_nested;
  for (const perl::Object& c : cycles) {
    result /= zero_vector<Rational>(result.cols());
    Array<IncidenceMatrix<> > rep = c.give("NESTED_PRESENTATIONS");
    Array<int> coeff = c.give("NESTED_COEFFICIENTS");
    int repindex =0;
    for (auto r_it = entire(rep); !r_it.at_end(); ++r_it, ++repindex) {
      int index = 0;
      int max_index = existing_nested.dim();
      // Check if it already exists
      bool found_it = false;
      for (auto en = entire(existing_nested); index < max_index; ++index, ++en) {
        if (*r_it == *en) {
          found_it = true;
          result(result.rows()-1, index) = coeff[repindex];
          break;
        }
      }
      if (!found_it) {
        if (result.rows() == 0)
          result = Matrix<Rational>(1,1);
        else
          result |= zero_vector<Rational>(result.rows());
        result(result.rows()-1, result.cols()-1) = coeff[repindex];
        existing_nested |= *r_it;
      }
    }
  }

  return result;
}

UserFunctionTemplate4perl("# @category Matroid ring cycle arithmetics"        
                          "# Computes the sum of two matroid ring cycles"
                          "# @param MatroidRingCycle A"
                          "# @param MatroidRingCycle B"
                          "# @return MatroidRingCycle A + B",
                          "matroid_ring_sum<Addition>(MatroidRingCycle<Addition>, MatroidRingCycle<Addition>)");

UserFunctionTemplate4perl("# @category Matroid ring cycle arithmetics"
                          "# Given a list of MatroidRingCycle objects (of the same rank r,"
                          "# on the same ground set), computes a matrix that represents the"
                          "# linear space spanned by these cycles in the rank r graded part"
                          "# of the matroid ring. Rows correspond to the cycles, columns "
                          "# correspond to the set of all the nested matroid occuring in "
                          "# all basis presentations of the cycles. Entries are linear coefficients."
                          "# @param MatroidRingCycle L A list of matroid ring cycles."
                          "# @return Matrix<Rational> A matrix representation of the linear space"
                          "# spanned by L"
                          "# @example The following computes 4 cycles of matroids of rank 2 on 4 elements."
                          "# It then computes the corresponding linear space representation, which shows "
                          "# immediately that M1 + M2 = M3 + M4"
                          "# > $m1 = new matroid::Matroid(N_ELEMENTS=>4,BASES=>[[0,1],[0,2],[1,3],[2,3]]);"
                          "# > $m2 = matroid::uniform_matroid(2,4);"
                          "# > $m3 = new matroid::Matroid(N_ELEMENTS=>4,BASES=>[[0,1],[0,2],[0,3],[1,3],[2,3]]);"
                          "# > $m4 = new matroid::Matroid(N_ELEMENTS=>4,BASES=>[[0,1],[0,2],[1,2],[1,3],[2,3]]);"
                          "# > @r = map { matroid_ring_cycle<Min>($_)} ($m1,$m2,$m3,$m4);"
                          "# > print matroid_ring_linear_space(@r);"
                          "# | 1 1 -1"
                          "# | 0 0 1 "
                          "# | 0 1 0"
                          "# | 1 0 0",
                          "matroid_ring_linear_space<Addition>(MatroidRingCycle<Addition>+)");
} }
