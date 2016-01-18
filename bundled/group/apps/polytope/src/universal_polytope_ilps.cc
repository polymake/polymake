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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace polymake { namespace polytope {

void print_lp(perl::Object p, perl::Object lp, const bool maximize, std::ostream& os);

namespace {

void write_output(const perl::Object& q, const perl::Object& lp, const std::string& filename)
{
   if (filename.empty()) {
      return;
   }
   else if (filename == "-") {
      print_lp(q, lp, false, perl::cout);
   } else {
      std::ofstream os(filename.c_str());
      print_lp(q, lp, false, os);
   }
}

} // end anonymous namespace

template <typename Scalar, typename SetType>
perl::Object universal_polytope_impl(int d, 
                                const Matrix<Scalar>& points, 
                                const Array<SetType>& facet_reps, 
                                Scalar vol, 
                                const SparseMatrix<Rational>& cocircuit_equations)
{
   const int n = facet_reps.size();
   Vector<Scalar> volume_vect(n);
   typename Vector<Scalar>::iterator vit = volume_vect.begin();
   for (typename Entire<Array<SetType> >::const_iterator fit = entire(facet_reps); !fit.at_end(); ++fit, ++vit) 
      *vit = abs(det(points.minor(*fit, All)));

   const SparseMatrix<Scalar> Inequalities = zero_vector<Scalar>(n) | unit_matrix<Scalar>(n);
   SparseMatrix<Scalar> Equations(0, n+1);
   Equations /= (-Integer::fac(d) * vol) | volume_vect;
   if (cocircuit_equations.cols())
      Equations /= (zero_vector<Scalar>(cocircuit_equations.rows()) 
                    | Matrix<Scalar>(cocircuit_equations));

   perl::Object q(perl::ObjectType::construct<Scalar>("Polytope"));
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
                            const SparseMatrix<Rational>& cocircuit_equations, 
                            perl::OptionSet options)
{
   const int n = facet_reps.size();

   perl::Object lp(perl::ObjectType::construct<Scalar>("LinearProgram"));
   lp.attach("INTEGER_VARIABLES") << Array<bool>(n,true);
   lp.take("LINEAR_OBJECTIVE") << Vector<Scalar>(0|ones_vector<Scalar>(n));

   perl::Object q = universal_polytope_impl(d, points, facet_reps, vol, cocircuit_equations);
   q.take("LP") << lp;

   const std::string filename = options["filename"];
   write_output(q, lp, filename);
   return q;
}

perl::Object foldable_max_signature_ilp(int d,
                                        const Matrix<Rational>& points,
                                        const Array<Set<int> >& facet_reps,
                                        const Rational& vol,
                                        const SparseMatrix<Rational>& foldable_cocircuit_equations,
                                        perl::OptionSet options)
{
   const int
      n2 = facet_reps.size(),
      n = n2/2;
   Vector<Integer> volume_vect(n2);
   SparseMatrix<Integer> selection(n,n2);
   Vector<Integer>::iterator vit = volume_vect.begin();
   int i=0;
   for (Entire<Array<Set<int> > >::const_iterator fit = entire(facet_reps); !fit.at_end(); ++fit, ++vit, ++i) {
      // points required to have integer coordinates!
      const Integer facet_vol = convert_to<Integer>(abs(det(points.minor(*fit, All))));
      *vit = facet_vol; // black facet
      ++vit;
      *vit = facet_vol; // white facet
      selection(i,2*i) = selection(i,2*i+1) = -1; // either black or white
   }

   const SparseMatrix<Integer> 
      Inequalities = (zero_vector<Integer>(n2) | unit_matrix<Integer>(n2)) /
                     (ones_vector<Integer>(n)  | selection),
      Equations    = (zero_vector<Integer>(foldable_cocircuit_equations.rows()) | Matrix<Integer>(foldable_cocircuit_equations)) /
                     ((-Integer::fac(d) * vol) | volume_vect);
   
   // signature = absolute difference of normalized volumes of black minus white maximal simplices
   // (provided that normalized volume is odd)
   for (int i=0; i<n; ++i)
      if (volume_vect[2*i].even()) volume_vect[2*i] = volume_vect[2*i+1] = 0;
      else volume_vect[2*i+1].negate();
   
   perl::Object lp("LinearProgram<Rational>");
   lp.attach("INTEGER_VARIABLES") << Array<bool>(n2,true);
   lp.take("LINEAR_OBJECTIVE") << Vector<Rational>(0|volume_vect);

   perl::Object q("Polytope<Rational>");
   q.take("FEASIBLE") << true;
   q.take("INEQUALITIES") << Inequalities;
   q.take("EQUATIONS") << Equations;
   q.take("LP") << lp;

   const std::string filename = options["filename"];
   write_output(q, lp, filename);
   return q;
}

template <typename Scalar, typename SetType>
Integer simplexity_lower_bound(int d, 
                               const Matrix<Scalar>& points, 
                               const Array<SetType>& facets, 
                               Scalar vol, 
                               const SparseMatrix<Rational>& cocircuit_equations, 
                               perl::OptionSet options)
{
   perl::Object q = simplexity_ilp(d, points, facets, vol, cocircuit_equations, options);
   const Scalar sll=q.give("LP.MINIMAL_VALUE");
   const Integer int_sll = convert_to<Integer>(sll); // rounding down
   return sll==int_sll? int_sll : int_sll+1;
}

Integer foldable_max_signature_upper_bound(int d, 
                                           const Matrix<Rational>& points, 
                                           const Array<Set<int> >& facets, 
                                           const Rational& vol, 
                                           const SparseMatrix<Rational>& foldable_cocircuit_equations, 
                                           perl::OptionSet options)
{
   perl::Object q = foldable_max_signature_ilp(d, points, facets, vol, foldable_cocircuit_equations, options);
   const Rational sll=q.give("LP.MAXIMAL_VALUE");
   const Integer int_sll = convert_to<Integer>(sll); // rounding down
   return int_sll;
}

FunctionTemplate4perl("universal_polytope_impl<Scalar>($ Matrix<Scalar> Array<Set> $ SparseMatrix)");


UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Set up an ILP whose MINIMAL_VALUE is the minimal number of simplices needed to triangulate a polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix points the input points or vertices "
                          "# @param Array<Set> the representatives of maximal interior simplices "
                          "# @param Scalar volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @option String filename a name for a file in .lp format to store the linear program"
                          "# @return LinearProgram an LP that provides a lower bound",
                          "simplexity_ilp<Scalar>($ Matrix<Scalar> Array<Set> $ SparseMatrix { filename=>'' })");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Calculate the LP relaxation lower bound for the minimal number of simplices needed to triangulate a polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix points the input points or vertices "
                          "# @param Scalar volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @return Integer the optimal value of an LP that provides a lower bound",
                          "simplexity_lower_bound<Scalar,SetType>($ Matrix<Scalar> Array<SetType> $ SparseMatrix { filename=>'' })");

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Set up an ILP whose MAXIMAL_VALUE is the maximal signature of a foldable triangulation of a polytope, point configuration or quotient manifold"
                  "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                  "# @param Matrix points the input points or vertices "
                  "# @param Rational volume the volume of the convex hull "
                  "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                  "# @option String filename a name for a file in .lp format to store the linear program"
                  "# @return LinearProgram<Rational> an ILP that provides the result",
                  &foldable_max_signature_ilp,
                  "foldable_max_signature_ilp($ Matrix Array<Set> $ SparseMatrix { filename=>'' })");

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Calculate the LP relaxation upper bound to the maximal signature of a foldable triangulation of polytope, point configuration or quotient manifold"
                  "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                  "# @param Matrix points the input points or vertices "
                  "# @param Rational volume the volume of the convex hull "
                  "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                  "# @return Integer the optimal value of an LP that provides a bound",
                  &foldable_max_signature_upper_bound,
                  "foldable_max_signature_upper_bound($ Matrix Array<Set> $ SparseMatrix { filename=>'' })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

