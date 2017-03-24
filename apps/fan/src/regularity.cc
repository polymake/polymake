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
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/common/lattice_tools.h"

namespace polymake { namespace fan {

   // Compute the root of a scalar, we just want the integral part. This is not
   // really important.
   template <typename Scalar>
   Integer pseudo_root(Scalar in){
      Integer result = convert_to<Integer>(floor(in));
      return sqrt(result);
   }

   // Rescale the ray generators to have almost the same length. As desired
   // length we take the maximum length of all rays multiplied with a factor.
   template <typename Scalar>
   Matrix<Scalar> rescale_rays(const Matrix<Scalar> input_rays, const Vector<Scalar> lengths, Scalar desired_len)
   {
      Matrix<Scalar> result(input_rays);
      Integer scaling;
      for(auto rowit = ensure(rows(result), (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); !rowit.at_end(); ++rowit){
         scaling = pseudo_root(desired_len / lengths[rowit.index()]);
         if(scaling == 0) { scaling = 1;}
         *rowit *= scaling;
      }
      return result;
   }
  
   
   // We want to determine whether the ray generators form the vertices of a
   // polytope. In this method we use the vertices as hyperplanes as well. Fix
   // a ray generator p, then we check whether <p,p> > <p,v> for all other ray
   // generators v. We do this for all p. If this is true, then the ray
   // generators form the vertices of a polytope.
   // This can be solved without double dualizing a polytope, the only backdraw
   // is that this condition is a little stronger than what we need.
   template <typename Scalar>
   bool check_rays(const Matrix<Scalar> input_rays)
   {
      Matrix<Scalar> check = input_rays * T(input_rays);
      for(int i=0; i<check.cols(); i++){
         for(int j=0; j<check.rows(); j++){
            if(i!=j) {
               if(check(j,i) >= check(i,i)){
                  return false;
               }
            }
         }
      }
      return true;
   }
   
   
   /*   
   // This method takes the rays (or more precise the ray generators) of the
   // fan and constructs a polytope with these rays as points. It then checks
   // whether the number of vertices of the resulting polytope is the same as
   // the number of rays.
   template <typename Scalar>
   bool check_rays(const Matrix<Scalar> input_rays)
   {
      Matrix<Scalar> points(input_rays);
      int n = points.rows();
      points = ones_vector<Scalar>(n) | points;
      perl::Object p(perl::ObjectType::construct<Scalar>("Polytope"));
      p.take("POINTS") << points;
      int check = p.give("N_VERTICES");
      return check == n;
   }
   */
   

   // Check whether the given ray generators of a fan satisfy the condition of
   // Shephard that they form the vertices of a polytope. If not, rescale them
   // until they do. See
   // Shephard: SPHERICAL COMPLEXES AND RADIAL PROJECTIONS OF POLYTOPES
   // (Shephard, G.C. Israel J. Math. (1971) 9: 257. doi:10.1007/BF02771591)
   template <typename Scalar>
   const Matrix<Scalar> prepare_rays(const Matrix<Scalar> input_rays)
   {
      bool check = check_rays(input_rays);
      if(check) { return input_rays;}
      const int debug_level = perl::get_debug_level();
      const Vector<Scalar> lengths(attach_operation( rows(input_rays), rows(input_rays), operations::mul()));
      Matrix<Scalar> result(input_rays);
      const Scalar max = accumulate(lengths, operations::max());
      Integer factor = 2;
      while(!check){
         if (debug_level > 1){
            cerr << "Ray generators do not form vertices of polytope. Rescaling." << endl;
         }
         result = rescale_rays(input_rays, lengths, factor * max);
         check = check_rays(result);
         factor *= 2;
      }
      if (debug_level > 1){
         cerr << "Found proper rescaling." << endl;
      }
      return result;
   }
   

    // decide whether a fan is the face fan (equivalently: normal fan) of a polytope
    // we use the method of Theorem 4.8 in Chapter V of Ewald
    // Let K be the Gale dual of the rays of the fan
    // and C_j the polytope spanned by the colums of the Gale dual whose index 
    // corresponds to rays not in the maximal cone sigma_j of the fan
    // The fan is polytopal if and only if there is a point in the intersection of the 
    // relative interiors of the polytopes C_j
    // 
    // we use a linear programming approach:
    // if b+A_jx\ge 0, l+L_jx=0 is an outer description of C_j,
    // then we consider the linear system of equations and inequalities
    // given by b+A_jx-s\ge 0, l+L_jx=0 for all j
    // if this has a feasible solution with s>0 then the fan is polytopal
    //
    template <typename Scalar>
    bool regular(perl::Object fan)
    {
       perl::ObjectType polytope_type=perl::ObjectType::construct<Scalar>("Polytope");
       perl::ObjectType linear_program_type=perl::ObjectType::construct<Scalar>("LinearProgram");

       const Matrix<Scalar> input_rays = fan.give("RAYS");
       const Matrix<Scalar> rays = prepare_rays(input_rays);
       const IncidenceMatrix<> max_cones = fan.give("MAXIMAL_CONES");
       const Matrix<Scalar> ker = T(null_space(T(ones_vector<Scalar>() | rays)));

       perl::Object c(polytope_type);
       const Matrix<Scalar> Pt = ones_vector<Scalar>() | ker.minor(~max_cones[0], All);
       c.take("INPUT_RAYS") << Pt;

       Matrix<Scalar> L = c.give("LINEAR_SPAN");
       Matrix<Scalar> At = c.give("FACETS");

       Matrix<Scalar> A = At | -(ones_vector<Scalar>(At.rows()));

       for (int i = 1; i < max_cones.rows(); ++i) {

          perl::Object c2(polytope_type);
          Matrix<Scalar> Pt = ker.minor(~max_cones[i], All);
          Pt = ones_vector<Scalar>() | Pt;
          c2.take("INPUT_RAYS") << Pt;
          Matrix<Scalar> Lt = c2.give("LINEAR_SPAN");
          L /= Lt;

          c2.give("FACETS") >> At;
          A /= At | -(ones_vector<Scalar>(At.rows()));
       }

       L = L | zero_vector<Scalar>();

       perl::Object p(polytope_type);
       p.take("INEQUALITIES") << A;
       p.take("EQUATIONS") << L;

       bool feasible = p.give("FEASIBLE");
       if (feasible) {
          perl::Object lp(linear_program_type);
          lp.take("LINEAR_OBJECTIVE") << unit_vector<Scalar>(A.cols(), A.cols()-1);
          p.add("LP", lp);

          const Scalar max = lp.give("MAXIMAL_VALUE");

          if (max > 0)
             return true;
       }

       return false;
    }

    FunctionTemplate4perl("regular<Scalar>(PolyhedralFan<Scalar>)");

} }
