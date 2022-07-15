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

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/polytope/convex_hull.h"
#include "polymake/polytope/separating_hyperplane.h"


namespace polymake{ 
namespace polytope{

template <typename Scalar>
bool contains_V_H(BigObject p_in, BigObject p_out){
   // get the Vertex description of the second polytopr
   Matrix<Scalar> V_in = p_in.give("RAYS | INPUT_RAYS");
   const OptionSet opt;
   for(const auto& v : rows(V_in)){
      if(!cone_H_contains_point(p_out, v, opt)) return false;
   }

   Matrix<Scalar> L_in;
   if (p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_in){
      for(const auto& l : rows(L_in)){
         if(!cone_H_contains_point(p_out, l, opt)) return false;
         if(!cone_H_contains_point(p_out, -l, opt)) return false;
      }
   }
   return true;
};


template<typename Scalar>
bool solve_same_description_LPs(const Matrix<Scalar>& Arays, const Matrix<Scalar>& Alin, const Matrix<Scalar>& Brays, const Matrix<Scalar>& Blin){
   // Check whether the cone A generated by the rays Arays and lineality Alin
   // is contained in the cone B (= Brays +- Blin).

   // Lineality must be contained in lineality, and this can be checked via
   // linear algebra.
   if(rank(Blin) != rank(Blin / Alin)) return false;

   // Now we only have to check whether every ray of Arays is contained in the
   // cone B, i.e. we make an LP to find a linear combination in terms of Brays
   // and Blin. This is already implemented elsewhere.
   BigObject container("Cone", mlist<Scalar>());
   container.take("INPUT_RAYS") << Brays;
   container.take("INPUT_LINEALITY") << Blin;
   const OptionSet opt;

   for(const auto& a : rows(Arays)){
      if(!cone_contains_point(container, a, opt)) return false;
   }
   return true;
}



template <typename Scalar>
bool contains_H_H_via_LP(BigObject p_in, BigObject p_out){
   Matrix<Scalar> F_out = p_out.lookup("FACETS | INEQUALITIES");
   Matrix<Scalar> E_out;

   if (!(p_out.lookup("LINEAR_SPAN | EQUATIONS") >> E_out)){
      E_out = zero_matrix<Scalar>(0, F_out.cols());
   }

   Matrix<Scalar> F_in = p_in.lookup("FACETS | INEQUALITIES");
   Matrix<Scalar> E_in;

   if (!(p_in.lookup("LINEAR_SPAN | EQUATIONS") >> E_in)){
      E_out = zero_matrix<Scalar>(0, F_out.cols());
   }

   // solve_same_description_LPs tries to find a linear combination for every
   // ray/lineality of the small cone in terms of the rays/linealities of the
   // larger cone. To apply this here we check the opposite containment of the
   // dual cones.
   return solve_same_description_LPs(F_out, E_out, F_in, E_in);
};


template <typename Scalar>
bool contains_V_V_via_LP(BigObject p_in, BigObject p_out)
{
   Matrix<Scalar> V_out = p_out.lookup("RAYS | INPUT_RAYS");
   Matrix<Scalar> L_out;

   if (!(p_out.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_out)){
      L_out = zero_matrix<Scalar>(0, V_out.cols());
   }

   Matrix<Scalar> V_in = p_in.lookup("RAYS | INPUT_RAYS");
   Matrix<Scalar> L_in;

   if (!(p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_in)){
      L_in = zero_matrix<Scalar>(0, V_out.cols());
   }

   // solve_same_description_LPs tries to find a linear combination for every
   // ray/lineality of the small cone in terms of the rays/linealities of the
   // larger cone.
   return solve_same_description_LPs(V_in, L_in, V_out, L_out);
};


// Checks if a given Polytope p_in is a subset of a other given  Polytope p_out.
// For each combination of discriptions (by Vertices or by Facets) it use another algorithm.
// @param BigObject p_in    The inner Polytope
// @param BigObject p_out   the outer Polytope
// @return Bool
template <typename Scalar>
bool contains(BigObject p_in, BigObject p_out)
{
   // Small sanity check to avoid segfaults
   const Int dim_in = p_in.give("CONE_AMBIENT_DIM");
   const Int dim_out = p_out.give("CONE_AMBIENT_DIM");
   if(dim_in != dim_out) throw std::runtime_error("Cones/Polytopes do no live in the same ambient space.");

   if(p_in.isa("Polytope") && p_out.isa("Polytope")){
      const bool feasible_in = p_in.give("FEASIBLE");
      if(!feasible_in) return true;
      const bool feasible_out = p_out.give("FEASIBLE");
      if(!feasible_out) return false;
   }

   // Ensure that we have a V-description of p_in and an H-description of
   // p_out.
   p_in.give("RAYS | INPUT_RAYS");
   p_out.give("FACETS | INEQUALITIES");
   return contains_V_H<Scalar>(p_in, p_out);
}

// now comes the contains function for balls und polytopes

template <typename Scalar>
bool contains_ball_dual(Vector<Scalar> c, Scalar r, BigObject p_out){
  
  // homogenize center of ball
  c = c/c[0];
  
  // get the outer description of p_out
  Matrix<Scalar> F_out = p_out.lookup("FACETS | INEQUALITIES");
  Matrix<Scalar> E_out;

  if (p_out.lookup("AFFINE_HULL | EQUATIONS") >> E_out){
    // work with inequalities
    if(E_out.rows()>0){
      return false;
    }
  }
  
  // scalar product with worst case direktion for each
  // inequality
  Vector<Scalar> F_out_norms = zero_vector<Scalar>(
          F_out.rows());
  for(int i=0; i<F_out.rows(); ++i){
    for(int j=1; j<F_out.cols(); ++j){
      F_out_norms[i] += sqr(F_out(i,j));
    }
  }
  
  Vector<Scalar> b = F_out*c;
  
  // compute (F_out*c)^2 - r^2 * F_out_norm  
  // and use this to check if F_out*c >= r * F_out_norm^(1/2)
  for(int i=0; i<b.size(); ++i){
    b[i] = sqr(b[i]) - sqr(r)*F_out_norms[i];
    
    if(b[i]<0){
      return false;
    }
  }

  return true;
}

template <typename Scalar>
bool contains_ball_primal(Vector<Scalar> c, Scalar r, BigObject p_out){
  // Since this problem is co-NP complete it will be changed to
  // the case of ball_dual
  p_out.give("FACETS | LINEAR_SPAN");
  return contains_ball_dual<Scalar>(c, r, p_out);
}


// Checks if a given Ball B(c,r) is a subset of a other given  Polytope p_out.
// For each combination of discriptions (by Vertices or by Facets) it use another algorithm.
// @param Vector c          the center of the ball
// @param Scalar r          the radius of the ball
// @param BigObject p_out   the outer Polytope
// @return Bool
template <typename Scalar>
bool polytope_contains_ball(Vector<Scalar> c, Scalar r, BigObject p_out)
{
  // check in which way p_out was given
  if (p_out.exists("FACETS | INEQUALITIES")){
  	return contains_ball_dual<Scalar>(c, r, p_out);
  }else{
    // p_out is given by vertices
    return contains_ball_primal<Scalar>(c, r, p_out);
  }
}

template <typename Scalar>
bool contains_primal_ball(BigObject p_in, Vector<Scalar> c, Scalar r){
  // get the vertex descrition of p_in
  Matrix<Scalar> V_in = p_in.give("RAYS | INPUT_RAYS");
  Matrix<Scalar> L_in;
  
  // check if p_in has rays
  for(int i=0; i<V_in.rows(); ++i){
    if( is_zero(V_in(i,0)) ){
      return false;
    }
  }
  
  // check p_in has a not empty lineality space
  if (p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_in){
    if (L_in.rows()>0){
      return false;
    }
  }
  
  // check if the ball contains all vertices of p_in
  // so we check if for each vertex v
  // ||c-v||² <= r² 
  r = sqr(r);
  c= c/c[0];
  Scalar r_c;
  for(int i=0; i<V_in.rows(); ++i){
  	r_c = sqr(c-V_in.row(i));
    if(r_c > r){
      return false;
    }
  }
  
  return true; 
}


template <typename Scalar>
bool contains_dual_ball(BigObject p_in, Vector<Scalar> c, Scalar r){
  // Since this problem is co-NP complete it will be changed to
  // the case of primal_ball
  p_in.give("RAYS | INPUT_RAYS");
  return contains_primal_ball<Scalar>(p_in, c, r);
}


// Checks if a given Polytope p_in is a subset of a given Ball B(c,r).
// For each combination of discriptions (by Vertices or by Facets) it use another algorithm.
// @param BigObject p_in    the inner Polytope
// @param Vector c          the center of the ball
// @param Scalar r          the radius of the ball
// @return Bool
template <typename Scalar>
bool polytope_contained_in_ball(BigObject p_in, Vector<Scalar> c, Scalar r)
{
  // check in which way p_in was given
  if (p_in.exists("RAYS | INPUT_RAYS")){
    return contains_primal_ball<Scalar>(p_in, c, r);
  }else{
    // p_out is given by inequalities
    return contains_dual_ball<Scalar>(p_in, c, r);
  }
}


} // namespace polytope
} // namespace polymake
