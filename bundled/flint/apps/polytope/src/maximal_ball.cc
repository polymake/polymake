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
#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Map.h"
#include "polymake/polytope/convex_hull.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/Rational.h"
#include "polymake/common/factorization.h"

namespace polymake{ namespace polytope{

// this programm just work for special cases, since the data type
// QuadraticExtension don't allow addition and multiplication of
// different roots

std::pair<QuadraticExtension<Rational>, Vector<QuadraticExtension<Rational>>> optimal_contains_ball_dual_Rational(Vector<Rational> c, Rational r, BigObject p_out, bool in_hull){
  // her we solve the problem max s such that s*B(c,r)+t a 
  // subset of p_out
  c = c/c[0];
  c[0] = 0;
  
  
  // get the outer description of p_out
  Matrix<Rational> F_out;
  if(in_hull){
    Matrix<Rational> F = p_out.give("FACETS");
    F_out = F;
  }else{
    Matrix<Rational> F =p_out.lookup("FACETS | INEQUALITIES");
    F_out = F;
    Matrix<Rational> E_out;


    if (p_out.lookup("AFFINE_HULL | EQUATIONS") >> E_out){
      // work with inequalities
      if(E_out.rows()>0){
        QuadraticExtension<Rational> s = 
          QuadraticExtension<Rational>(0,0,0);
        Vector<QuadraticExtension<Rational>> t =
          p_out.give("ONE_VERTEX");
        return std::make_pair(s,t);
      }
    }
  }
  
  // build a vector which entries are the sum of the squared entries 
  // of the connected row o F_out
  Vector<QuadraticExtension<Rational>> F_out_norms(F_out.rows());
      
  for(int i=0; i<F_out.rows(); ++i){
    Rational sqrnorm = 0;
    for(int j=1; j<F_out.cols(); ++j){
      sqrnorm += sqr(F_out(i,j));
    }
    std::pair<Integer, Integer> 
      ir_sqrt_num = pm::flint::factor_out_squares( 
        numerator(sqrnorm)),
      ir_sqrt_den = pm::flint::factor_out_squares( 
        denominator(sqrnorm));
    
    F_out_norms[i] = QuadraticExtension<Rational>(0, 
      Rational(ir_sqrt_num.second, ir_sqrt_den.second),
      Rational(ir_sqrt_num.first, ir_sqrt_den.first));    
  }
  
  // get last columof Matrix A of the description b + Ax>=0
  Vector<QuadraticExtension<Rational>> a = (F_out*c-r*F_out_norms);
  
  // define inequalities
  Matrix<QuadraticExtension<Rational>> F_new =
                  Matrix<QuadraticExtension<Rational>>(F_out)|a;
    //(F_out*c-r*F_out_norms)|
    //Matrix<QuadraticExtension<Rational>>(F_out).minor(All,Cols);
  
  
  // define costfunktion
  Vector<QuadraticExtension<Rational>> costfkt =
    zero_vector<QuadraticExtension<Rational>>(F_new.cols());
  costfkt[F_out.cols()] = QuadraticExtension<Rational>(1,0,0);
  
  // define new Polytope
  BigObject p_new("Polytope<QuadraticExtension<Rational>>");
  p_new.take("INEQUALITIES") << F_new/costfkt;
  p_new.take("LP.LINEAR_OBJECTIVE") << costfkt;
  
  // cout << "d" << endl;
  
  // check if p_new is feasible
  if(!p_new.give("FEASIBLE")){
    QuadraticExtension<Rational> s = QuadraticExtension<Rational>(0,0,0);
    Vector<QuadraticExtension<Rational>> t = p_out.give("ONE_VERTEX");
    t[0] = 2;
    return std::make_pair( s, t-c );
  }

  // check solution
  if (p_new.give("LP.MAXIMAL_VALUE") == std::numeric_limits<QuadraticExtension<Rational>>::infinity()){
    // the LP is unbounded
    QuadraticExtension<Rational> s = std::numeric_limits<QuadraticExtension<Rational>>::infinity();
    
    Vector<QuadraticExtension<Rational>> t =\
      zero_vector<QuadraticExtension<Rational>>( F_out.cols());
    
    t[0] = QuadraticExtension<Rational>(1,0,0);
    
    return std::make_pair(s,t);
  }else{

    // get the solution of the LP
    Vector<QuadraticExtension<Rational>> s_t = p_new.give("LP.MAXIMAL_VERTEX");
    
    QuadraticExtension<Rational> s =s_t[F_out.cols()];
    
    Vector<QuadraticExtension<Rational>> t= s_t.slice(range(0,F_out.cols()-1));
    
    return std::make_pair(s, t);
  }
}


std::pair<QuadraticExtension<Rational>, Vector<QuadraticExtension<Rational>>> optimal_contains_ball_primal_Rational(Vector<Rational> c, Rational r, BigObject p_out, bool in_hull){
  // Since this problem is co-NP complete it will be changed to
  // it wil be changed to the case of ball_dual
  
  // load the inner description of p_out
  Matrix<Rational> V_out = p_out.lookup("RAYS | INPUT_RAYS"); 
  Matrix<Rational> L_out;
  convex_hull_result<Rational> hull_out;
  std::string got_property;
  
  if(p_out.lookup_with_property_name(
            "LINEALITY_SPACE | INPUT_LINEALITY",
            got_property) >> L_out){
    if(got_property == "INPUT_LINEALITY"){
      L_out = L_out.minor(basis(L_out).first,All);
    }
  }else{
    L_out = zero_matrix<Rational>(0, V_out.cols());
  }
  
  // get a outer description of p_out
  hull_out = enumerate_facets(V_out, L_out, true);
  
  
  BigObject p_out_new(p_out.type());
  p_out_new.take("INEQUALITIES") << hull_out.first;
  p_out_new.take("EQUATIONS") << hull_out.second;
  
  
  return optimal_contains_ball_dual_Rational(c, r, p_out_new, in_hull);
}


// Compute a vector c and the maximal scalar r, such that for 
// a  Ball B(c,r) is a subset of a given Polytope p_out.
// For each combination of discriptions (by Vertices or by 
// Facets) it use another algorithm.
// @param BigObject p_out   the outer Polytope
// @return pair<Scalar,Vector>
std::pair<QuadraticExtension<Rational>,Vector<QuadraticExtension<Rational>>> maximal_ball(BigObject p_out)
{ 

  Matrix<Rational> F;
  
  // check in which way p_out was given
  if (p_out.lookup("FACETS | INEQUALITIES")>>F){
    
    Rational r = 1;
    Vector<Rational> c = zero_vector<Rational>(F.cols());
    c[0] = 1;
    return optimal_contains_ball_dual_Rational(\
              c, r, p_out, true);
    
  }else{
    // p_out was given by vertices
    Matrix<Rational> V = p_out.lookup("VERTICES | POINTS");
    Rational r = 1;
    Vector<Rational> c = zero_vector<Rational>(V.cols());
    c[0] = 1;
    return optimal_contains_ball_primal_Rational(\
              c, r, p_out, true);
  }
};



UserFunction4perl(
                  "# @category Geometry"
                  "# Finds for a given rational Polytope //P// the maximal ball B(//r//,//c//)"
                  "# which is contained in //P//. Here //r// is the radius of the maximal ball"
                  "# and //c// is it center. Since is can happen, that r is not a rational number"
                  "# or c is not a rational, does this function only work for polytopes for which in"
                  "# the norm of each can be written as QuadraticExtension to the same root."
                  "# @param Polytope<Rational> P input polytope with rational coordinates"
                  "# @return Pair <QuadraticExtension<Rational>>, Vector<QuadraticExtension<Rational>>>> "
                  "# @example"
                  "# > $S = simplex(2);"
                  "# > print maximal_ball($S);"
                  "# | 1-1/2r2 <1 1-1/2r2 1-1/2r2>",
                  &maximal_ball,
                  "maximal_ball(Polytope<Rational>)");

}}


