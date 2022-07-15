/* Copyright (c) 1997-2022
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

#pragma once

#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/polytope/solve_LP.h"

namespace polymake { namespace polytope {

template <typename Scalar, typename VectorType, typename MatrixType>
Vector<Scalar> separating_hyperplane(const GenericVector<VectorType, Scalar>& q, 
                                     const GenericMatrix<MatrixType, Scalar>& points)
{
   /*
     construction of LP according to cdd redundancy check for points, see
     http://www.ifor.math.ethz.ch/~fukuda/polyfaq/node22.html#polytope:Vredundancy
    */
   const Matrix<Scalar> 
      ineqs(  (zero_vector<Scalar>() | (ones_vector<Scalar>() | -points.minor(All, range(1,points.cols()-1))))
              // z^t p_i - z_0 <= 0; CAUTION: p_i is affine part of i-th point! 
              / (ones_vector<Scalar>(2) | -q.slice(range_from(1))) ),
              // z^t q - z_0 <= 1, prevents unboundedness
      affine_hull(null_space(points/q)),
      extension2(affine_hull.rows(), 2);
   Matrix<Scalar>
      affine_hull_minor(affine_hull.rows(), affine_hull.cols()-1);
   if (affine_hull.cols() > 1) {
      affine_hull_minor = affine_hull.minor(All, range(1, affine_hull.cols()-1));
   }
   const Matrix<Scalar> eqs(extension2 | -affine_hull_minor);
   const Vector<Scalar> obj(zero_vector<Scalar>(1) | -ones_vector<Scalar>(1) | q.slice(range_from(1))); // z^t q - z_0

   const auto S = solve_LP(ineqs, eqs, obj, true);
   if (S.status != LP_status::valid || S.objective_value <= 0) //q non-red. <=> obj_val > 0
      throw infeasible();

   // H: z^t x = z_0, i.e., z_0 - z^t x = 0
   return S.solution[1] | -S.solution.slice(range_from(2));
}

template <typename Scalar, typename VectorType>
bool cone_H_contains_point(BigObject p, const GenericVector<VectorType, Scalar>& q, OptionSet options)
{
   bool in = options["in_interior"];
   if(in && !p.exists("FACETS")){
      throw std::runtime_error("This method can only check for interior points if FACETS are given");
   }
   const Matrix<Scalar> ineqs = p.give("FACETS | INEQUALITIES");
   for(const auto& ineq : rows(ineqs)){
      auto check = ineq * q;
      if(check < 0) return false;
      if(in && check == 0) return false;
   }
   Matrix<Scalar> eqs;
   if(p.lookup("LINEAR_SPAN | EQUATIONS") >> eqs){
      for(const auto& eq : rows(eqs)){
         if(eq * q != 0) return false;
      }
   }
   return true;
}

template <typename Scalar, typename VectorType>
bool cone_V_contains_point(BigObject p, const GenericVector<VectorType, Scalar>& q, OptionSet options) 
{
   // the LP constructed here is supposed to find a conical/convex combination of the rays/vertices
   // of cone/polytope p that equals q. if the in_interior flag is set, all coefficients are
   // required to be strictly positive.
   Matrix<Scalar> P = p.give("RAYS | INPUT_RAYS");

   Matrix<Scalar> eqs = -q | T(P) ; //sum z_i p_i = q
   //(if p_i are given in hom.coords, also sum z_i = 1 for all i whose p_i are not rays)

   //lookup prevents computation of this property in case it is not given.
   Matrix<Scalar> E = p.lookup("LINEALITY_SPACE | INPUT_LINEALITY");
   if (E.rows()) {
      // for polytopes, replace first col of E with 0, so we have sum z_i = 1 only for points
      if (p.isa("Polytope")) E = zero_vector<Scalar>() | E.minor(All,range(1,E.cols()-1));

      eqs = eqs | T(E) | T(-E);
   }
   eqs = eqs | zero_vector<Scalar>();

   Int n = P.rows();
   Matrix<Scalar> ineqs = zero_vector<Scalar>(n) | unit_matrix<Scalar>(n) | zero_matrix<Scalar>(n, 2*E.rows()) | -ones_vector<Scalar>(n); //z_i >= eps for rays

   Int c = n+2+2*E.rows();
   ineqs = ineqs / unit_vector<Scalar>(c, c-1) //eps >= 0
                 / (unit_vector<Scalar>(c, 0) - unit_vector<Scalar>(c, c-1)); // eps <= 1

   const auto S = solve_LP(ineqs, eqs, unit_vector<Scalar>(c, c-1), true);  // maximize eps
   if (S.status != LP_status::valid)
      return false;

   bool in = options["in_interior"];
   return !(in && S.objective_value == 0);  // the only separating plane is a weak one
}


template <typename Scalar, typename VectorType>
bool cone_contains_point(BigObject p, const GenericVector<VectorType, Scalar> & q, OptionSet options) {
   bool in = options["in_interior"];
   if(in){
      if(p.exists("FACETS")) return cone_H_contains_point(p, q, options);
      else return cone_V_contains_point(p, q, options);
   }
   if(p.exists("FACETS | INEQUALITIES")) return cone_H_contains_point(p, q, options);
   else return cone_V_contains_point(p, q, options);
}


} // namespace polytope
} // namespace polymake


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
