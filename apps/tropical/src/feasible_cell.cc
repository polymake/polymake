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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include "polymake/tropical/dual_addition_version.h"
#include "polymake/tropical/covectors.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/canonicalize.h"
#include "polymake/tropical/arithmetic.h"
#include "polymake/tropical/double_description.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical {

// auxiliary function for trop_witness
// checks the infeasibility of the generalized covector y
// w.r.t. the signs given by negative_indices of coordinates in D
// @return apex node of degree one which is incident with a negative edge
Int infeasible(const IncidenceMatrix<>& y, const Array<Int>& negative_indices, const Set<Int> D)
{
   for (Int t = 0; t < y.rows(); ++t) {
      // check if the incident edge of an apex node of degree one is negative
      if (((y[t]).size() == 1) && ((y[t]).contains(negative_indices[t])) && (D.contains(negative_indices[t])))
         return t+1;
   }
   // this encodes the case that y is not infeasible
   return 0;
}

// auxiliary function for trop_witness
// checks if delta is incident with a negative edge in y with the signs given by negative_indices
Int negatively_covered(const IncidenceMatrix<>& y, const Array<Int>& negative_indices, Int delta)
{
   for (Int t = 0; t < y.rows(); ++t) {
      // check if the incident edge of an apex node of degree one is negative and incident with delta
      if (((y[t]).size() == 1) && ((y[t]).contains(negative_indices[t])) && (negative_indices[t] == delta))
         return t+1;
   }
   return 0;
}

// determine a certificate of the (in-)feasibility of a tropical inequality system
// coeff is the coefficient matrix, the array negative_indices contains the entry where the corresponding row of coeff is tropically negative
// the algorithm iteratively increases the coordinates on which a solution is defined
// start_coord denotes the coordinate to start with
template <typename Addition, typename Scalar>
std::pair<Vector<TropicalNumber<Addition, Scalar>>, Int>
trop_witness(const Matrix<TropicalNumber<Addition,Scalar>>& coeff, const Array<Int>& negative_indices, Int start_coord = 0)
{
   //Int counter = 0;

   // initialization
   using TNumber = TropicalNumber<Addition, Scalar>;
   Int j = -1;
   Int jold = -1;
   Int k = -1;
   Int i;
   Int delta = start_coord; 
   Set<Int> D; // current set of coordinate nodes
   Set<Int> N; // current set of defining apex nodes; "j --> negative_indices[j]" for j in N defines a bijection
   Set<Int> coords{delta};
   Set<Int> sup = sequence(0, coeff.cols()) - coords;

   // compute the (|N|+1)-dimensional vector of (|N| times |N|)-minors of the submatrix of coeff defined by (N times coords)
   Vector<TNumber> x = subcramer(coeff, N,coords);
   // compute the generalized covector (graph) of x w.r.t. the coefficient matrix coeff
   IncidenceMatrix<> y = generalized_apex_covector(x,coeff);

   enum { Feasible, Infeasible, TotallyInfeasible} check;

   // the loop terminates if a certificate of feasibility or infeasibility is returned
   while (true) { 
      //++counter;

      // set the variable check;
      // determines the degree of feasibility

      // (j-1) is the minimal index of an apex node which has degree 1 and
      // is incident with a negative edge to D
      // there is none if j == 0
      j = infeasible(y,negative_indices, D);
      if (j--) {
         check = Infeasible;
      } else {
         // (k-1) is the minimal index of an apex node which is incident to
         // delta via a negative edge
         // there is none if k == 0
         k = negatively_covered(y,negative_indices,delta);
         if (k--) {
            check = TotallyInfeasible;
         } else {
            check = Feasible;
         }
      }

      // determine the next point or terminate -- depending on the feasibility status

      switch (check) {
      case Feasible:
         return std::make_pair(x,1);
      case TotallyInfeasible: {
         // increase the coordinate set or
         // terminate with certificate of infeasibility
         if (sup.empty()) {
            return std::make_pair(x,0);
         }

         N += k;
         D += delta;
         delta = accumulate(sup, operations::min());
         sup -= delta;
         coords = D + delta;
         break;
      }
      case Infeasible: {
         // i is the coordinate node which is incident, via a negative edge,
         // with the apex node j of degree one
         i = negative_indices[j];

         for (Int a : N) {
            if (negative_indices[a] == i) {
               jold = a;
               break;
            }
         }
         N -= jold; N += j;
         break;
      }}
            
      // update the current point and generalized covector defined by the modified index sets

      x = subcramer(coeff, N, coords);
      y = generalized_apex_covector(x,coeff);            
   }
}

template <typename Addition = Min, typename Scalar>
bool check_witness(const Vector<TropicalNumber<Addition, Scalar>>& trop_witness, const Matrix<TropicalNumber<Addition, Scalar>>& m,
                   const Array<Int>& negative_indices, bool feasible)
{
   using TNumber = TropicalNumber<Addition, Scalar>;
   Set<Int> fulfilled;
   Set<Int> covered_coord; // negatively covered coordinate nodes
   Set<Int> row_set = sequence(0, m.rows());
   Set<Int> col_set = sequence(0, m.cols());
   Int i;
   Vector<TNumber> v;
   TNumber neg_entry;
   for (Int j = 0; j < m.rows(); ++j) {
      i = negative_indices[j];
      v = m[j];
      neg_entry = v[i];
      v[i] = zero_value<TNumber>();
      if (v*trop_witness <= neg_entry*trop_witness[i])
         fulfilled += j;
      if (v*trop_witness >= neg_entry*trop_witness[i])
         covered_coord += i;
   }
   return feasible ? fulfilled == row_set : covered_coord == col_set;
}


template <typename Addition, typename Scalar>
std::pair<Vector<TropicalNumber<Addition, Scalar>>, bool> H_trop_input_feasible(BigObject p)
{
   const std::pair<Matrix<TropicalNumber<Addition, Scalar>>, Matrix<TropicalNumber<Addition, Scalar>>> Ineq = p.lookup("INEQUALITIES");
   std::pair<Matrix<TropicalNumber<Addition, Scalar>>, Array<Int>> split_apices = matrixPair2splitApices(Ineq.first, Ineq.second);
   return trop_witness(split_apices.first, split_apices.second);
}

FunctionTemplate4perl("H_trop_input_feasible<Addition,Scalar> (Polytope<Addition,Scalar>)");

UserFunctionTemplate4perl("# @category Tropical operations"
                          "# checks feasibility of tropical inequality system"
                          "# @tparam Addition"
                          "# @tparam Scalar"
                          "# @param Matrix<TropicalNumber<Addition, Scalar> > m"
                          "# @param Array<Int > t"
                          "# @param Int start"
                          "# @return Vector<TropicalNumber<Addition, Scalar> > ",
                          "trop_witness<Addition,Scalar>(Matrix<TropicalNumber<Addition,Scalar> >, Array<Int>)");

UserFunctionTemplate4perl("# @category Tropical operations"
                          "# checks feasibility of tropical inequality system"
                          "# @tparam Addition"
                          "# @tparam Scalar"
                          "# @param Matrix<TropicalNumber<Addition, Scalar> > m"
                          "# @param Array<Int > t"
                          "# @param Int start"
                          "# @return Vector<TropicalNumber<Addition, Scalar> > ",
                          "trop_witness<Addition,Scalar>(Matrix<TropicalNumber<Addition,Scalar> >, Array<Int>,Int)");


UserFunctionTemplate4perl("# @category Tropical operations"
                          "# computes Cramer bracket"
                          "# |I| = |J| + 1 is required."
                          "# @param Matrix<TropicalNumber<Addition, Scalar> > m"
                          "# @param Set<Int> J"
                          "# @param Set<Int> I"
                          "# @return Vector<TropicalNumber<Addition, Scalar> > ",
                          "subcramer<Addition,Scalar>(Matrix<TropicalNumber<Addition,Scalar> >, Set<Int>, Set<Int>)");

FunctionTemplate4perl("check_witness<Addition, Scalar>(Vector<TropicalNumber<Addition,Scalar> >,Matrix<TropicalNumber<Addition,Scalar> >,Array<Int>,$)");      
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
