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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include "polymake/tropical/dual_addition_version.h"
#include "polymake/tropical/covectors.h"
#include "polymake/tropical/thomog.h"
#include "polymake/tropical/canonicalize.h"
#include "polymake/tropical/arithmetic.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace tropical {

      // auxiliary function for trop_witness
      // checks the infeasibility of the generalized covector y
      // w.r.t. the signs given by negative_indices
      // @return apex node of degree one which is incident with a negative edge
      int infeasible (const IncidenceMatrix<>& y, const Array<int> &negative_indices) {
         for (int t = 0; t < y.rows(); t++) {
            // check if the incident edge of an apex node of degree one is negative
            if (((y[t]).size() == 1) && ((y[t]).contains(negative_indices[t]))) { return (t+1); }
         }
         // this encodes the case that y is not infeasible
         return 0;
      }

      // auxiliary function for trop_witness
      // checks if delta is incident with a negative edge in y with the signs given by negative_indices
      int negatively_covered (const IncidenceMatrix<>& y, const Array<int> &negative_indices, int delta) {
         for (int t : y.col(delta)) {
            if (negative_indices[t] == delta) { return (t+1);}
         }
         return 0;
      }

      // determine a certificate of the (in-)feasibility of a tropical inequality system
      // coeff is the coefficient matrix, the array negative_indices contains the entry where the corresponding row of coeff is tropically negative
      // the algorithm iteratively increases the coordinates on which a solution is defined
      // start_coord denotes the coordinate to start with
      template <typename Addition = Min, typename Scalar>
      std::pair<Vector<TropicalNumber<Addition, Scalar> >, int> trop_witness(const Matrix<TropicalNumber<Addition,Scalar> > &coeff, const Array<int> &negative_indices, int start_coord = 0)
      {
         /* int COUNTER = 0; */

         // initialization
         typedef TropicalNumber<Addition, Scalar> TNumber;
         int j = -1;
         int jold = -1;
         int k = -1;
         int i;
         int delta = start_coord; 
         Set<int> D; // current set of coordinate nodes
         Set<int> N; // current set of defining apex nodes; "j --> negative_indices[j]" for j in N defines a bijection
         Set<int> coords = scalar2set(delta);
         Set<int> sup = sequence(0,coeff.cols()) - coords;
         // compute the (|N|+1)-dimensional vector of (|N| times |N|)-minors of the submatrix of coeff defined by (N times coords)
         Vector<TNumber> x = subcramer(coeff, N,coords);
         // compute the generalized covector (graph) of x w.r.t. the coefficient matrix coeff
         IncidenceMatrix<> y = generalized_apex_covector(x,coeff);
         int check;
         
         // the loop terminates if a certificate of feasibility or infeasibility is returned
         while (true) { 
            /* COUNTER += 1; */
                     
            // set the variable check;
            // determines the degree of feasibility
            
            // (j-1) is the minimal index of an apex node which has degree 1 and
            // is incident with a negative edge
            // there is none if j == 0
            j = infeasible(y,negative_indices);
            if (j--) {
               // (k-1) is the minimal index of an apex node which is incident to
               // delta via a negative edge
               // there is none if k == 0
               k = negatively_covered(y,negative_indices,delta);
               if (k--) {
                  check = 2; //TOTALLY-INFEASIBLE
               }
               else { check = 1; } //INFEASIBLE 
            } else { check = 0; } //FEASIBLE


            // determine the next point or terminate -- depending on the feasibility status

            // x is feasible
            if (check == 0) 
               {/* cout << COUNTER << endl;*/ return std::make_pair(x,1); }

            // x is totally infeasible
            // increase the coordinate set or
            // terminate with certificate of infeasibility
            else if (check == 2) { 
               if (sup.empty()) {/* cout << COUNTER << endl;*/ return std::make_pair(x,0);}
               N += scalar2set(k);
               D += scalar2set(delta);
               delta = accumulate(sup, operations::min()); 
               sup -= scalar2set(delta);
               coords = D + scalar2set(delta);
            }

            // x is infeasible but not totally infeasible
            // 
            else if (check == 1) {
               // i is the coordinate node which is incident, via a negative edge,
               // with the apex node j of degree one
               i = negative_indices[j];

               for (int a : N) {
                  if (negative_indices[a] == i) {jold = a; break;}
               }
               N -= jold; N += j;
            }
            
            // update the current point and generalized covector defined by the modified index sets

            x = subcramer(coeff, N, coords);
            y = generalized_apex_covector(x,coeff);
         }
      }

      template <typename Addition = Min, typename Scalar>
      bool check_witness(const Vector<TropicalNumber<Addition, Scalar> >& trop_witness, const Matrix<TropicalNumber<Addition,Scalar> > &m, const Array<int> &negative_indices, bool feasible) {
         typedef TropicalNumber<Addition, Scalar> TNumber;
         Set<int> fulfilled;
         Set<int> covered_coord; // negatively covered coordinate nodes
         Set<int> row_set = sequence(0, m.rows());
         Set<int> col_set = sequence(0, m.cols());
         int i;
         Vector<TNumber> v;
         TNumber neg_entry;
         for (int j = 0; j < m.rows(); j++) {
            i = negative_indices[j];
            v = m[j];
            neg_entry = v[i];
            v[i] = zero_value<TNumber>();
            if (v*trop_witness <= neg_entry*trop_witness[i]) fulfilled += j;
            if (v*trop_witness >= neg_entry*trop_witness[i]) covered_coord += i;
            
         }
         if ((fulfilled == row_set) && feasible)  return true;
         else { if ((covered_coord == col_set) && !feasible) return true; 
            else return false; }
      }
      


      
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
                                "# computes Cramer bracket"
                                "# |I| = |J| + 1 is required."
                                "# @param Matrix<TropicalNumber<Addition, Scalar> > m"
                                "# @param Set<Int> J"
                                "# @param Set<Int> I"
                                "# @return Vector<TropicalNumber<Addition, Scalar> > ",
                                "subcramer<Addition,Scalar>(Matrix<TropicalNumber<Addition,Scalar> >, Set<Int>, Set<Int>)");
          
      FunctionTemplate4perl("check_witness<Addition, Scalar>(Vector<TropicalNumber<Addition,Scalar> >,Matrix<TropicalNumber<Addition,Scalar> >,Array<Int>,$)");      
}}
      
// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
