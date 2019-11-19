/* Copyright (c) 1997-2019
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
#include "polymake/polytope/poly2lp.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace polymake { namespace polytope {

namespace {

template<typename Scalar>
void write_output(const perl::Object& q, const perl::Object& lp, const std::string& filename)
{
   if (filename.empty()) {
      return;
   }
   else if (filename == "-") {
      print_lp<Scalar>(q, lp, false, perl::cout);
   } else {
      std::ofstream os(filename.c_str());
      print_lp<Scalar>(q, lp, false, os);
   }
}

} // end anonymous namespace

template <typename Scalar, typename SetType>
perl::Object universal_polytope_impl(int d, 
                                     const Matrix<Scalar>& points, 
                                     const Array<SetType>& facet_reps, 
                                     const Scalar& vol, 
                                     const SparseMatrix<Rational>& cocircuit_equations)
{
   const int 
      n_reps = facet_reps.size(), 
      n_cols = cocircuit_equations.cols();
   if (n_reps > n_cols)
      throw std::runtime_error("Need at least #{simplex reps} many columns in the cocircuit equation matrix");

   Vector<Scalar> volume_vect(n_reps);
   auto vit = volume_vect.begin();
   for (const auto& f: facet_reps)
      *vit = abs(det(points.minor(f, All))), ++vit;

   const SparseMatrix<Scalar> Inequalities = zero_vector<Scalar>(n_reps) | unit_matrix<Scalar>(n_reps) | zero_matrix<Scalar>(n_reps, n_cols - n_reps);
   const SparseMatrix<Scalar> Equations(((-Integer::fac(d) * vol) | volume_vect | zero_vector<Scalar>(n_cols - n_reps))
                                       / (zero_vector<Scalar>(cocircuit_equations.rows()) | Matrix<Scalar>(cocircuit_equations)));

   perl::Object q("Polytope", mlist<Scalar>());
   q.take("FEASIBLE") << true;
   q.take("INEQUALITIES") << Inequalities;
   q.take("EQUATIONS") << Equations;
   return q;
}

template <typename Scalar, typename SetType>
perl::Object simplexity_ilp(int d, 
                            const Matrix<Scalar>& points, 
                            const Array<SetType>& facet_reps, 
                            Scalar vol, 
                            const SparseMatrix<Rational>& cocircuit_equations)
{
   const int 
      n_reps = facet_reps.size(), 
      n_cols = cocircuit_equations.cols();
   if (n_reps > n_cols)
      throw std::runtime_error("Need at least #{simplex reps} many columns in the cocircuit equation matrix");

   perl::Object lp("LinearProgram", mlist<Scalar>());
   lp.attach("INTEGER_VARIABLES") << Array<bool>(n_reps, true);
   lp.take("LINEAR_OBJECTIVE") << Vector<Scalar>(0 | ones_vector<Scalar>(n_reps) | zero_vector<Scalar>(n_cols-n_reps));

   perl::Object q = universal_polytope_impl(d, points, facet_reps, vol, cocircuit_equations);
   q.take("LP") << lp;
   return q;
}

template <typename SetType, typename EquationsType>
perl::Object foldable_max_signature_ilp(int d,
                                        const Matrix<Rational>& points,
                                        const Array<SetType>& max_simplices,
                                        const Rational& vol,
                                        const EquationsType& foldable_cocircuit_equations)
{
   const int n = max_simplices.size();
   Vector<Integer> volume_vect(2*n);
   SparseMatrix<Integer> selection(n,2*n);
   Vector<Integer>::iterator vit = volume_vect.begin();
   int s = 0;
   for (const auto& f: max_simplices) {
      // points have integer coordinates. This is ensured by the check for $p->LATTICE in universal_polytope.rules
      const Integer max_simplex_vol (numerator(abs(det(points.minor(f, All)))));
      *vit = max_simplex_vol; ++vit; // black maximal simplex
      *vit = max_simplex_vol; ++vit; // white maximal simplex
      selection(s, 2*s) = selection(s, 2*s+1) = -1; // select one of either black or white
      ++s;
   }

   const SparseMatrix<Integer> 
      Inequalities = (zero_vector<Integer>(2*n) | unit_matrix<Integer>(2*n)) /
                     (ones_vector<Integer>(n)   | selection),
      Equations    = (zero_vector<Integer>(foldable_cocircuit_equations.rows()) | Matrix<Integer>(foldable_cocircuit_equations)) /
                     ((-Integer::fac(d) * vol) | volume_vect);
   
   // signature = absolute difference of normalized volumes of black minus white maximal simplices
   // (provided that normalized volume is odd)
   for (int i = 0; i < n; ++i)
      if (volume_vect[2*i].even()) 
         volume_vect[2*i] = volume_vect[2*i+1] = 0;
      else 
         volume_vect[2*i+1].negate();
   
   perl::Object lp("LinearProgram<Rational>");
   lp.attach("INTEGER_VARIABLES") << Array<bool>(2*n,true);
   lp.take("LINEAR_OBJECTIVE") << Vector<Rational>(0|volume_vect);

   perl::Object q("Polytope<Rational>");
   q.take("FEASIBLE") << true;
   q.take("INEQUALITIES") << Inequalities;
   q.take("EQUATIONS") << Equations;
   q.take("LP") << lp;
   return q;
}

template <typename Scalar, typename SetType>
Integer simplexity_lower_bound(int d, 
                               const Matrix<Scalar>& points, 
                               const Array<SetType>& max_simplices, 
                               Scalar vol, 
                               const SparseMatrix<Rational>& cocircuit_equations)
{
   perl::Object q = simplexity_ilp(d, points, max_simplices, vol, cocircuit_equations);
   const Scalar sll=q.give("LP.MINIMAL_VALUE");
   const Integer int_sll(floor(sll));
   return sll==int_sll? int_sll : int_sll+1;
}

template <typename SetType>
Integer foldable_max_signature_upper_bound(int d, 
                                           const Matrix<Rational>& points, 
                                           const Array<SetType>& max_simplices, 
                                           const Rational& vol, 
                                           const SparseMatrix<Rational>& foldable_cocircuit_equations)
{
   perl::Object q = foldable_max_signature_ilp(d, points, max_simplices, vol, foldable_cocircuit_equations);
   const Rational sll=q.give("LP.MAXIMAL_VALUE");
   return floor(sll);
}

FunctionTemplate4perl("universal_polytope_impl<Scalar>($ Matrix<Scalar> Array<Set> $ SparseMatrix)");


UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Set up an ILP whose MINIMAL_VALUE is the minimal number of simplices needed to triangulate a polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix points the input points or vertices "
                          "# @param Array<Set> MIS the representatives of maximal interior simplices "
                          "# @param Scalar volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @return LinearProgram an LP that provides a lower bound",
                          "simplexity_ilp<Scalar,SetType>($ Matrix<Scalar> Array<SetType> $ SparseMatrix)");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Calculate the LP relaxation lower bound for the minimal number of simplices needed to triangulate a polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix points the input points or vertices "
                          "# @param Scalar volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @return Integer the optimal value of an LP that provides a lower bound",
                          "simplexity_lower_bound<Scalar,SetType>($ Matrix<Scalar> Array<SetType> $ SparseMatrix)");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Set up an ILP whose MAXIMAL_VALUE is the maximal signature of a foldable triangulation of a polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix points the input points or vertices "
                          "# @param Rational volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @return LinearProgram<Rational> an ILP that provides the result",
                          "foldable_max_signature_ilp<SetType, EquationsType>($ Matrix Array<SetType> $ EquationsType)");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Calculate the LP relaxation upper bound to the maximal signature of a foldable triangulation of polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix points the input points or vertices "
                          "# @param Rational volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @return Integer the optimal value of an LP that provides a bound",
                          "foldable_max_signature_upper_bound<SetType>($ Matrix Array<SetType> $ SparseMatrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

