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
#include "polymake/SparseVector.h"
#include "polymake/AccurateFloat.h"
#include "polymake/ListMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/hash_map"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/group/permlib.h"
#include "polymake/common/lattice_tools.h"
//#include "polymake/common/boost_dynamic_bitset.h"
#include <iostream>
#include <fstream>
#include <vector>

namespace polymake { namespace polytope {

void print_lp(perl::Object p, perl::Object lp, const bool maximize, std::ostream& os);

typedef Set<int> SetType;
      //typedef common::boost_dynamic_bitset SetType;

namespace  {

template<typename Scalar>
AccurateFloat solid_angle_from_inhomogeneous_normal_vectors(const Vector<Scalar>& v1, const Vector<Scalar>& v2)
{
   return acos(conv<Scalar,AccurateFloat>()(-v1*v2)/sqrt(conv<Scalar,AccurateFloat>()(sqr(v1)*sqr(v2))));
   // --------------------------------------^----
   // the minus sign implements subtracting the angle from pi.
   // This is necessary because the input vectors are normal vectors to the hyperplanes, 
   // but we want to return the solid angle between the hyperplanes
}

template<typename Scalar>
AccurateFloat total_angle(const SetType& codim_2_rep, 
                          const Matrix<Scalar>& F, 
                          const IncidenceMatrix<>& VIF)
{
   Set<int> containing_facets;
   for (int i=0; i<F.rows(); ++i) {
      const Set<int> facet(VIF.row(i));
      if (incl(facet, codim_2_rep)==1) 
         containing_facets += i;
   }
   const int codim = containing_facets.size();
   if (codim == 0) {  // can't use switch statement because of variable initialization in case 2
      return 2 * AccurateFloat::pi();
   } else if (codim == 1) {
      return AccurateFloat::pi();
   } else if (codim == 2) {
      const Vector<Scalar> 
         n1 = dehomogenize(F[containing_facets.front()]),
         n2 = dehomogenize(F[containing_facets.back()]);
      return solid_angle_from_inhomogeneous_normal_vectors(n1,n2);
   } else {
      throw std::runtime_error("Did not expect a codim 2 face to be contained in more than two facets.");
   }
}

template<typename Scalar>
AccurateFloat angle_at_e(const Matrix<Scalar>& V, const SetType& simplex, const SetType& e)
{
   const SetType f(simplex - e);
   assert(f.size()==2);
   const int aind = f.front(), bind = f.back();
   const SetType 
      facet1(simplex - scalar2set(aind)),
      facet2(simplex - scalar2set(bind));
   
   Vector<Scalar>
      n1 = null_space(V.minor(facet1, All))[0],
      n2 = null_space(V.minor(facet2, All))[0],
      a = V[aind],
      b = V[bind];
   if (n1[0] < 0) n1.negate(); // the null_space() operation might return a negative first coefficient
   if (n2[0] < 0) n2.negate();
   const bool 
      negate_n1 = n1 * a < 0,
      negate_n2 = n2 * b < 0;
   n1 = dehomogenize(n1); if (negate_n1) n1.negate();
   n2 = dehomogenize(n2); if (negate_n2) n2.negate();
   return solid_angle_from_inhomogeneous_normal_vectors(n1,n2);
}

}

template<typename Scalar>
SparseMatrix<double> 
symmetrized_codim_2_angle_sums(int d,
                               const Matrix<Scalar>& V,
                               const Matrix<Scalar>& F,
                               const IncidenceMatrix<>& VIF,
                               const Array<Array<int> >& generators,
                               const Array<SetType>& codim_2_reps,
                               const Array<SetType>& facet_reps,
                               perl::OptionSet options) 
{
   const std::string filename = options["filename"];
   std::ofstream outfile(filename.c_str(), std::ios_base::trunc);
   const group::PermlibGroup sym_group(generators);
   const int group_order = sym_group.order();

   hash_map<SetType, int> index_of_facet_rep;
   int ct(-1);
   for (Entire<Array<SetType> >::const_iterator ait = entire(facet_reps); !ait.at_end(); ++ait)
      index_of_facet_rep[*ait] = ++ct;

   hash_map<SetType, int> index_of_codim_2_rep;
   ct = -1;
   for (Entire<Array<SetType> >::const_iterator ait = entire(codim_2_reps); !ait.at_end(); ++ait)
      index_of_codim_2_rep[*ait] = ++ct;

   std::vector<SetType> faces_on_hull;
   for (Entire<Array<SetType> >::const_iterator cd2it = entire(codim_2_reps); !cd2it.at_end(); ++cd2it) {
      const SetType face (*cd2it);
      int ct(0), i(0);
      while (ct<2 && i<VIF.rows())
         if (incl(VIF.row(i++), face)==1) ++ct;
      if (ct>=2) faces_on_hull.push_back(face);
   }

   const int
      n_facet_reps  (index_of_facet_rep.size()),
      n_codim_2_reps (index_of_codim_2_rep.size()),
      n_hull_reps   (faces_on_hull.size());

   // inhomogeneous equations
   SparseMatrix<AccurateFloat> codim_2_angle_sums(n_codim_2_reps + n_hull_reps,
                                                  n_facet_reps + n_codim_2_reps + 1);

   for (Entire<Array<SetType> >::const_iterator fit = entire(facet_reps); !fit.at_end(); ++fit) {
      const SetType simplex(*fit);
      for (Entire<Subsets_of_k<const SetType&> >::const_iterator eit = entire(all_subsets_of_k(simplex, d-1)); !eit.at_end(); ++eit) {
         const SetType e(*eit), e_rep(sym_group.lex_min_representative(e));
         codim_2_angle_sums(index_of_codim_2_rep[e_rep], index_of_facet_rep[simplex] + 1) += angle_at_e(V, simplex, e);
      }
   }

   for (int i=0; i<n_codim_2_reps; ++i) {
      codim_2_angle_sums(i, n_facet_reps + i + 1) = - total_angle(codim_2_reps[i], F, VIF);
   }

   int i=0;
   for (Entire<std::vector<SetType> >::const_iterator vit = entire(faces_on_hull); !vit.at_end(); ++vit, ++i) {
      codim_2_angle_sums(n_codim_2_reps + i, n_facet_reps + index_of_codim_2_rep[*vit] + 1) = AccurateFloat(1);
      codim_2_angle_sums(n_codim_2_reps + i, 0) = AccurateFloat(-group_order/sym_group.setwise_stabilizer(*vit).order());
   }

   if (filename.size()) wrap(outfile) << codim_2_angle_sums << endl;
   return SparseMatrix<double>(codim_2_angle_sums);
}


template <typename Scalar>
perl::Object simplexity_ilp_with_angles(int d, 
                                        const Matrix<Scalar>& points, 
                                        const Array<SetType >& facets, 
                                        Scalar vol, 
                                        const SparseMatrix<Rational>& cocircuit_equations, 
                                        const SparseMatrix<double>& angle_equations,
                                        perl::OptionSet options)
{
   cerr << "entered simplexity_ilp_with_angles" << endl;
   const int 
       n = cocircuit_equations.cols(),
      an = angle_equations.cols()-1; // these equations are non-homogeneous
   cerr << "n=" << n << ", an=" << an << endl;
   Vector<Scalar> volume_vect(n);
   typename Vector<Scalar>::iterator vit = volume_vect.begin();
   for (Entire<Array<Set<int> > >::const_iterator fit = entire(facets); !fit.at_end(); ++fit, ++vit) 
      *vit = abs(det(points.minor(*fit, All)));

   cerr << "will construct matrices" << endl;
   const SparseMatrix<Scalar> 
      Inequalities = zero_vector<Scalar>(an) | unit_matrix<Scalar>(an),
      Equations    = 
      (zero_vector<Scalar>(cocircuit_equations.rows()) | SparseMatrix<Scalar>(cocircuit_equations) | SparseMatrix<Scalar>(cocircuit_equations.rows(), an - n)) / 
      SparseMatrix<Scalar>(angle_equations)  /
      ((-Integer::fac(d) * vol) | volume_vect | zero_vector<Scalar>(an - n));

   cerr << "done" << endl;
   perl::Object lp(perl::ObjectType::construct<Scalar>("LinearProgram"));
   lp.attach("INTEGER_VARIABLES") << Array<bool>(an, true);
   lp.take("LINEAR_OBJECTIVE") << Vector<Scalar>(0 | ones_vector<Scalar>(n) | zero_vector<Scalar>(an - n));

   perl::Object q(perl::ObjectType::construct<Scalar>("Polytope"));
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


FunctionTemplate4perl("symmetrized_codim_2_angle_sums<Scalar>($ Matrix<Scalar> Matrix<Scalar> IncidenceMatrix Array<Array<Int>> Array<Set> Array<Set> { filename=>'', reduce_rows=>0, log_frequency=>0 })");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Set up an ILP whose MINIMAL_VALUE is the minimal number of simplices needed to triangulate a polytope, point configuration or quotient manifold"
                          "# @param Int d the dimension of the input polytope, point configuration or quotient manifold "
                          "# @param Matrix points the input points or vertices "
                          "# @param Array<Set> the (representative) maximal interior simplices "
                          "# @param Scalar volume the volume of the convex hull "
                          "# @param SparseMatrix cocircuit_equations the matrix of cocircuit equations "
                          "# @option filename a name for a file in .lp format to store the linear program"
                          "# @return an LP that provides a lower bound",
                          "simplexity_ilp_with_angles<Scalar>($ Matrix<Scalar> Array<Set> $ SparseMatrix SparseMatrix<Float> { filename=>'' })");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

 
