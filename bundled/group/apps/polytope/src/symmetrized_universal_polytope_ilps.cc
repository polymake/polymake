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
#include "polymake/group/permlib.h"
#include "polymake/common/boost_dynamic_bitset.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace polymake { namespace polytope {

void print_lp(perl::Object p, perl::Object lp, const bool maximize, std::ostream& os);

typedef common::boost_dynamic_bitset SetType;

perl::Object symmetrized_foldable_max_signature_ilp(int d,
                                                    const Matrix<Rational>& points,
                                                    const Array<SetType>& facets,
                                                    const Rational& vol,
                                                    const Array<Array<int> >& generators,
                                                    const SparseMatrix<Rational>& symmetrized_foldable_cocircuit_equations,
                                                    perl::OptionSet options)
{
   // ONLY for lattice polytopes!
   const int
      n2 = symmetrized_foldable_cocircuit_equations.cols(), 
      // this matrix contains one black and one white copy of each orbit
      // ideally, black simplices should have even column indices, white simplices odd column indices
      // in reality, this is the other way around because of the inhomogeneous zero-th column
      n = n2/2;
   cerr << n << ", " << n2 << endl;
   
   const group::PermlibGroup sym_group(generators);
   const int group_order(sym_group.order());
   cerr << "group_order: " << group_order << endl;
   SparseMatrix<Integer> selection_matrix(n,n2+1);

   Vector<Integer> volume_vect(n2+1);
   Vector<Integer>::iterator vit = volume_vect.begin();
   *vit = -Integer::fac(d) * vol;
   ++vit;

   int i=0;
   for (Entire<Array<SetType > >::const_iterator fit = entire(facets); !fit.at_end(); ++fit, ++vit, ++i) {
      // points required to have integer coordinates!
      const Rational rational_facet_vol = abs(det(points.minor(*fit, All)));
      assert (denominator(rational_facet_vol) == 1);
      const Integer facet_vol = convert_to<Integer>(rational_facet_vol);
      cerr << *fit << ", volume " << facet_vol << endl;
      *vit = facet_vol; // black facet
      ++vit;
      *vit = facet_vol; // white facet
      cerr << "(" << i << "," << 2*i+1 << "), (" << i << "," << 2*i+2 << ")" << endl;
      selection_matrix(i,2*i+1) = selection_matrix(i,2*i+2) = -1; // either black or white
      selection_matrix(i,0) = sym_group.orbit(Set<int>(*fit)).size();
   }

   const SparseMatrix<Integer> 
      Inequalities = (zero_vector<Integer>(n2) | unit_matrix<Integer>(n2)) /
                      selection_matrix,
      Equations    = (zero_vector<Integer>(symmetrized_foldable_cocircuit_equations.rows()) | Matrix<Integer>(symmetrized_foldable_cocircuit_equations)) /
                     volume_vect;
   
   // signature = absolute difference of normalized volumes of black minus white maximal simplices
   // (provided that normalized volume is odd)
   for (int i=0; i<n; ++i) {
      cerr << 2*i+1 << ", " << 2*i+2 << endl;
      if (volume_vect[2*i+1].even()) volume_vect[2*i+1] = volume_vect[2*i+2] = 0;
      else volume_vect[2*i+2].negate();
   }
   cerr << "volume_vect: " << volume_vect << endl;
   
   perl::Object lp(perl::ObjectType::construct<Rational>("LinearProgram"));
   lp.attach("INTEGER_VARIABLES") << Array<bool>(n2,true);
   lp.take("LINEAR_OBJECTIVE") << Vector<Rational>(0|volume_vect.slice(1));

   perl::Object q(perl::ObjectType::construct<Rational>("Polytope"));
   q.take("FEASIBLE") << true;
   q.take("INEQUALITIES") << Inequalities;
   q.take("EQUATIONS") << Equations;
   q.take("LP") << lp;

   const std::string filename = options["filename"];

   if (filename.size()) {
      std::ofstream os(filename.c_str());
      print_lp(q, lp, false, os);
   }
   return q;
}

Integer symmetrized_foldable_max_signature_upper_bound(int d, 
                                                       const Matrix<Rational>& points, 
                                                       const Array<SetType>& facets, 
                                                       const Rational& vol, 
                                                       const Array<Array<int> >& generators,
                                                       const SparseMatrix<Rational>& symmetrized_foldable_cocircuit_equations, 
                                                       perl::OptionSet options)
{
   perl::Object q = symmetrized_foldable_max_signature_ilp(d, points, facets, vol, generators, symmetrized_foldable_cocircuit_equations, options);
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
                  "# @option filename a name for a file in .lp format to store the linear program"
                  "# @return an ILP that provides the result",
                  &symmetrized_foldable_max_signature_ilp,
                  "symmetrized_foldable_max_signature_ilp($ Matrix Array<boost_dynamic_bitset> $ Array<Array<Int>> SparseMatrix { filename=>'' })");

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Calculate the LP relaxation upper bound to the maximal signature of a foldable triangulation of polytope, point configuration or quotient manifold"
                  "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                  "# @param Matrix points the input points or vertices "
                  "# @param Rational volume the volume of the convex hull "
                  "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                  "# @return the optimal value of an LP that provides a bound",
                  &symmetrized_foldable_max_signature_upper_bound,
                  "symmetrized_foldable_max_signature_upper_bound($ Matrix Array<boost_dynamic_bitset> $ Array<Array<Int>> SparseMatrix { filename=>'' })");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

