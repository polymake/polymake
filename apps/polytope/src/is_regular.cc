/* Copyright (c) 1997-2021
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
#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include "polymake/polytope/is_regular.h"
#include <fstream>

namespace polymake { namespace polytope {


template <typename Scalar>
BigObject regularity_lp(const Matrix<Scalar> &verts, const Array<Set<Int>>& subdiv, OptionSet options)
{
   if (subdiv.size() < 2)
      throw std::runtime_error("Subdivision is trivial.");   

   const auto mats = secondary_cone_ineq(full_dim_projection(verts), subdiv, options);
   const auto& inequs = mats.first;
   const auto& equats = mats.second;
   
   const Int n_vertices = verts.rows();

   Scalar epsilon = options["epsilon"];
   BigObject q("Polytope", mlist<Scalar>());
   q.take("FEASIBLE") << true;
   if (equats.rows() > 0)
      q.take("EQUATIONS") << (zero_vector<Scalar>() | equats);
   q.take("INEQUALITIES") << 
      (zero_vector<Scalar>(n_vertices) | unit_matrix<Scalar>(n_vertices)) /
      ((-epsilon * ones_vector<Scalar>(inequs.rows())) | inequs);

   BigObject lp = q.add("LP", "LINEAR_OBJECTIVE", 0 | ones_vector<Scalar>(n_vertices));
   lp.attach("INTEGER_VARIABLES") << Array<bool>(n_vertices, true);
   return q;
}



template <typename Scalar>
std::pair<bool,Vector<Scalar>>
is_regular(const Matrix<Scalar> &verts, const Array<Set<Int>>& subdiv, OptionSet options)
{
   const auto mats = secondary_cone_ineq(full_dim_projection(verts), subdiv, options);

   BigObject res("Cone", mlist<Scalar>(),
                 "INEQUALITIES", mats.first,
                 "EQUATIONS", mats.second);

   Vector<Scalar> w;
   try {
      const Vector<Scalar> wt = res.give("REL_INT_POINT");
      w = wt;
   } catch (const Undefined& e) {
      // there is no relative internal point. If the trivial subdivision is asked for, we are ok; else the subdivision is not regular
      if (subdiv.size() == 1 &&
          subdiv[0] == sequence(0, verts.rows())) {
         w = zero_vector<Scalar>(mats.first.cols());
      } else return std::pair<bool,Vector<Scalar>>(false,Vector<Scalar>());
   }
   const Vector<Scalar> slack = mats.first*w;

   for (auto it=entire(slack); !it.at_end(); ++it) {
      if (*it==0)
         return std::pair<bool,Vector<Scalar>>(false,Vector<Scalar>());
   }
   return std::pair<bool,Vector<Scalar>>(true,w);
}

FunctionTemplate4perl("secondary_cone_ineq<Scalar>(Matrix<Scalar> Array<Set>; {equations => undef, lift_to_zero=>undef, lift_face_to_zero => undef, test_regularity=>0})");

FunctionTemplate4perl("full_dim_projection<Scalar>(Matrix<Scalar>)");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# For a given subdivision //subdiv// of //points// tests"
                          "# if the subdivision is regular and if yes computes a weight"
                          "# vector inducing this subdivsion."
                          "# The output is a pair of Bool and the weight vector."
                          "# Options can be used to ensure properties of the resulting vector."
                          "# The default is having 0 on all vertices of the first face of //subdiv//."
                          "# @param Matrix points in homogeneous coordinates"
                          "# @param Array<Set<Int> > subdiv"
                          "# @return Pair<Bool,Vector>"
                          "# @option Matrix<Scalar> equations system of linear equation the cone is cut with."
                          "# @option Set<Int> lift_to_zero gives only lifting functions lifting the designated vertices to 0"
                          "# @option Int lift_face_to_zero gives only lifting functions lifting all vertices of the designated face to 0"
                          "# @example A regular subdivision of the square, with the first cell lifted to zero:"
                          "# > $points = cube(2)->VERTICES;"
                          "# > print is_regular($points,[[0,1,3],[1,2,3]],lift_to_zero=>[0,1,3]);"
                          "# | 1 <0 0 1 0>"
                          "# @author Sven Herrmann fixed by Benjamins with Georg",
                          "is_regular<Scalar>(Matrix<Scalar>,$;{equations => undef, lift_to_zero=>undef, lift_face_to_zero => 0})");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# For a given subdivision //subdiv// of //points// determines"
                          "# a //LinearProgram// to decide whether the subdivision is regular."
                          "# The output a Polytope with an attached LP."
                          "# Options can be used to ensure properties of the resulting LP."
                          "# The default is having 0 on all vertices of the first face of //subdiv//."
                          "# @param Matrix points in homogeneous coordinates"
                          "# @param Array<Set<Int> > subdiv"
                          "# @return Polytope<Scalar>"
                          "# @option Matrix<Scalar> equations system of linear equation the cone is cut with."
                          "# @option Set<Int> lift_to_zero gives only lifting functions lifting the designated vertices to 0"
                          "# @option Int lift_face_to_zero gives only lifting functions lifting all vertices of the designated face to 0"
                          "# @option Scalar epsilon minimum distance from all inequalities"
                          "# @author Sven Herrmann",
                          "regularity_lp<Scalar>(Matrix<Scalar>,$;{equations => undef, lift_to_zero=>undef, lift_face_to_zero => 0, epsilon => 1/100})");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
