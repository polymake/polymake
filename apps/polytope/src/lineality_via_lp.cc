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
#include "polymake/Matrix.h"
#include "polymake/GenericMatrix.h"
#include "polymake/linalg.h"
#include "polymake/polytope/solve_LP.h"

namespace polymake { namespace polytope {

/*
   If the system has no implicit linearity, the function returns  1;
   If the system defines the empty cone,    the function returns -1; but the reverse implication does not hold!
   If there is some implicit linearity,     the function returns  0.
*/
template <typename Scalar, typename TMatrix1, typename TMatrix2>
Int implicit_linearity_decision(const GenericMatrix<TMatrix1, Scalar>& inequalities,  // or input_rays
                                const GenericMatrix<TMatrix2, Scalar>& equations)     // or input_lineality
{
   /*
     We copy the following lp from bundled/cdd/external/cdd/lib-src/cddlp.c , lines 3124 ff:

       f* = maximize    z
       subject to
         b_I x_0 + A_I x - 1 z >= 0 (all nonlinearity generators in one side)
         b_L x_0 + A_L x  = 0       (linearity generators)
         z <= 1.
    
    Any implicit linearity exists if and only if the optimal value f* is nonpositive.
    More precisely, as stated above,

    if f* > 0,  the system has no implicit linearity;
    if f* < 0,  the system defines the empty cone; but the reverse implication does not hold.
    if f* == 0, there is some implicit linearity.

    The polymake formulation uses variables (x_c, x, z), where x_c is a dummy variable for the constants,
    and reads
    
      [ 0 inequalities -1 ] [ x_c ]
      [ 1 0            -1 ] [ x   ]   >= 0
                            [ z   ]
                            
      [ 0 equations     0 ]  ...       = 0

      maximize [ 0 0 1 ]
    */
   const Int n_ineqs = inequalities.rows();
   const Int n_eqs   =   equations.rows();
   const Int d       = inequalities.cols();

   const Matrix<Scalar> ineqs( unit_vector<Scalar>(n_ineqs+1, n_ineqs) | ( inequalities / zero_vector<Scalar>(d) ) | -ones_vector<Scalar>(n_ineqs+1) );

   const Matrix<Scalar> eqs = n_eqs
      ? Matrix<Scalar>( zero_vector<Scalar>(n_eqs) | equations | zero_vector<Scalar>(n_eqs) )
      : Matrix<Scalar>(); // to_simplex likes its zero matrices to have zero columns

   const auto sol = solve_LP(ineqs, eqs, unit_vector<Scalar>(d+2, d+1), true); // true: maximize
   if (sol.status != LP_status::valid)
      throw std::runtime_error("lineality_via_lp: wrong LP");
   return sign(sol.objective_value);
}

// determine the indices of generators of lineality among the given inequalities
template <typename Scalar, typename TMatrix1, typename TMatrix2>
Set<Int>
lineality_indices_among_inequalities(const GenericMatrix<TMatrix1, Scalar>& inequalities,  // or input_rays
                                     const GenericMatrix<TMatrix2, Scalar>& equations)     // or input_lineality
{
   /*
     According to cddlp.c lines 2046ff, the test to be performed for the i-th inequality ineq_i is

     f* = maximize ineq_i
     s.t.
     ineq_j >= 0     for all j distinct from i
     eq_k = 0        for all k

     The inequality ineq_i is an implicit linearity if and only if the optimal value f* is nonpositive.
    */

   Set<Int> lineality_indices;
   if (1 == implicit_linearity_decision(inequalities, equations))
      return lineality_indices;

   const Matrix<Scalar> eqs = equations.rows()
      ? Matrix<Scalar>(zero_vector<Scalar>(equations.rows()) | equations)
      : Matrix<Scalar>(); // to_simplex likes its zero matrices to have zero columns

   for (Int i = 0; i < inequalities.rows(); ++i) {
      const Matrix<Scalar> ineqs(zero_vector<Scalar>(inequalities.rows()-1) | inequalities.minor(~scalar2set(i), All));
      const Vector<Scalar> obj(Scalar(0) | inequalities[i]);

      const auto S = solve_LP(ineqs, eqs, obj, true);
      if (S.status == LP_status::valid) {
         if (S.objective_value <= 0) 
            lineality_indices += i;
      } else if (S.status == LP_status::infeasible) {
         throw std::runtime_error("lineality_indices_among_inequalities: infeasible LP");
      }
      // unbounded is ok, it just means that ineq_i is not an implicit linearity
   }
   return lineality_indices;
}
      
template <typename Scalar, typename TMatrix1, typename TMatrix2>
Matrix<Scalar>
lineality_via_lp(const GenericMatrix<TMatrix1, Scalar>& inequalities,  // or input_rays
                 const GenericMatrix<TMatrix2, Scalar>& equations)     // or input_lineality
{
   if (! inequalities.rows()) {
      Matrix<Scalar> eqs(equations);
      return eqs.minor(basis_rows(eqs),All);
   }

   Matrix<Scalar> lin = equations.rows()
      ? Matrix<Scalar>(equations /
                       inequalities.minor(lineality_indices_among_inequalities(inequalities, equations), All) )
      : inequalities.minor(lineality_indices_among_inequalities(inequalities, equations), All);

   return lin.minor(basis_rows(lin),All);
}
      
FunctionTemplate4perl("implicit_linearity_decision<Scalar>(Matrix<type_upgrade<Scalar>,_>,Matrix<type_upgrade<Scalar>,_>)");
   
FunctionTemplate4perl("lineality_via_lp<Scalar>(Matrix<type_upgrade<Scalar>,_>,Matrix<type_upgrade<Scalar>,_>)");
   
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
