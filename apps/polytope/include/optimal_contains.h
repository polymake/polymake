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

#pragma once

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/polytope/convex_hull.h"

#include <cstdlib>
#include <vector>
#include <list>
#include <iterator>


#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include "Miniball/Miniball.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace polymake{ namespace polytope{

// now comes the part of optimal containment
// so we want to solve for to polytopes P1 and P2 the problem
// max s such that sP1+t is a subset of P2
// this makes now sences for cones since positive scaling didn't change cones
// in generell the sum of a  point and a cone is now cone animore

template <typename Scalar>
std::pair<Scalar,Vector<Scalar>> optimal_contains_primal_dual(BigObject p_in, BigObject p_out)
{
  // get the outer description of p_out
  Matrix<Scalar> F_out = p_out.lookup("FACETS | INEQUALITIES");
  Matrix<Scalar> E_out;
  
  // check if a solution exist
  if (!p_out.give("FEASIBLE")){
    if(p_in.give("FEASIBLE")){
      Scalar inf = std::numeric_limits<Scalar>::infinity();
      Vector<Scalar> t(F_out.cols());
      t[0] = 1;
      return std::make_pair( inf,t);
    }else{
      Vector<Scalar> t(F_out.cols());
      t[0] = 1;
      return std::make_pair(0, t); 
    }
  }

  if (p_out.lookup("AFFINE_HULL | EQUATIONS") >> E_out){
    // write equations as inequalities
    F_out /= E_out/(-E_out);
  }

  // get the vertex descrition of p_in
  Matrix<Scalar> V_in = p_in.give("VERTICES | POINTS");
  Matrix<Scalar> L_in;
  
  
  // check lineality space of V_in
  if (p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_in){
    Matrix<Scalar> b = F_out * T(L_in);

    for(int i=0; i<b.rows();++i){
      for(int j=0;j<b.cols();++j){
        if (b(i,j) != 0){
          // p_in has to become a single point in p_out
          Vector<Scalar> t = p_out.give("ONE_VERTEX");
          return std::make_pair(Scalar(0), t);
        }
      }
    }
  }
  
  // check if rays of p_in in p_out
  // and build set of vertices of p_in
  Set<Int> vertices_in;
  Vector<Scalar> b_vector;
  
  for(int i=0;i<V_in.rows();++i){
    // check of i-th row is a ray or a vertex
    if ( is_zero( V_in(i,0) ) ){
      // check if F_out * Ray >= 0
      b_vector= F_out * V_in.row(i);
      
      for(int j=0;j<F_out.rows();++j){
        if(b_vector[j]<0){
          Vector<Scalar> t = p_out.give("ONE_VERTEX");
          return std::make_pair(Rational(0), t);
        }
      }
    }else{
      vertices_in += i;
    }
  }
  
  Matrix<Scalar> Vertices_in = V_in.minor(vertices_in,All);
  
  // set the additional 1 of each vertex to zero
  for(int i=0; i<Vertices_in.rows(); ++i){
    Vertices_in(i,0) = 0;
  }
  
  // now construct a polytop on t, s and minimize s over it
  Matrix<Scalar> b = F_out*T(Vertices_in);
  Matrix<Scalar> F_new( F_out.rows() * Vertices_in.rows(), 
                        1 + F_out.cols() );


  for(int i=0;i<F_out.rows();i++){
    for(int j=0;j<Vertices_in.rows();j++){
    
      for(int l=0;l<F_out.cols();l++){
        F_new( i * Vertices_in.rows() + j, l ) = F_out(i,l);
      }
      F_new( i * Vertices_in.rows() + j, F_out.cols() ) = b(i,j);
    }
  }

  

  Vector<Scalar> costfkt(F_out.cols()+1);
  costfkt[F_out.cols()] = 1;

  BigObject p_new(p_out.type());
  p_new.take("INEQUALITIES") << F_new/costfkt;
  p_new.take("LP.LINEAR_OBJECTIVE") << costfkt;
  
  Vector<Scalar> t_s = p_new.give("LP.MAXIMAL_VERTEX");
  Vector<Scalar> t = t_s.slice( sequence(0, F_out.cols()) );

  return std::make_pair(t_s[F_out.cols()], t);
}

template <typename Scalar>
std::pair<Scalar,Vector<Scalar>> optimal_contains_dual_dual(BigObject p_in, BigObject p_out){
  // to solve the problem we solve problem 
  // min s such that p_in subset s*p_out + t
  
  // get the outer description of p_out
  Matrix<Scalar> F_out = p_out.lookup("FACETS | INEQUALITIES");
  Matrix<Scalar> E_out;

  if (p_out.lookup("AFFINE_HULL | EQUATIONS") >> E_out){
    // write equations as inequalities
    F_out /= E_out/(-E_out);
  }
  
  // get the outer description of p_out
  Matrix<Scalar> F_in = p_in.lookup("FACETS | INEQUALITIES");
  Matrix<Scalar> E_in;

  if (p_in.lookup("AFFINE_HULL | EQUATIONS") >> E_in){
    // write equations as inequalities
    F_in /= E_in/(-E_in);
  }
  
  
  // construct a polyope on the matrix D, the vector t and s,
  // With D*A_in = A_out, D*b_in <= b_out*s + A_out*t for
  // [b_in,A_in]=F_in and [b_out,A_out]=F_out.
  
  // define equations
  Matrix<Scalar> E_new( (F_out.cols()-1) * F_out.rows(),
                        1 + F_out.cols() + F_out.rows() *
                          F_in.rows() );
                                         
  int k=0;                                
  for(int i=0; i<F_out.rows(); ++i){
    for(int j=1; j<F_out.cols(); ++j){
      E_new(k,0) = - F_out(i,j);
      

      for(int l=0; l<F_in.rows(); ++l){
        E_new(k, 1 + F_out.cols() + i*F_in.rows() + l) = F_in(l,j);
      }
      k++;
    }
  }


  
  
  // define inequalities 
  Matrix<Scalar> F_new(  F_out.rows(),
                         1 + F_out.cols() + F_out.rows() *
                           F_in.rows() );
  
  for(int i=0; i<F_out.rows(); i++){
    //the coefficient for s
    F_new(i,1) = F_out(i,0);
    
    for(int j=1; j<F_out.cols(); ++j){
      // the coefficients for t
      F_new(i,1+j) = -F_out(i,j);
    }
    
    for(int l=0; l<F_in.rows(); ++l){
      // the coefficients for D
      F_new(i, 1 + F_out.cols() + i*F_in.rows() +l ) = - F_in(l,0);
    }
  }
    
  // add positiv conditions
  Matrix<Scalar> Pos = zero_matrix<Scalar>(F_out.rows() * 
                         F_in.rows(), 1 + F_out.cols())|
                       unit_matrix<Scalar>(F_out.rows() *
                         F_in.rows());
  
  // define cost funktion
  Vector<Scalar> costfkt(1 + F_out.cols() + F_out.rows() *
                          F_in.rows());
  costfkt[1] = 1;
  
  
  // define new Polytope
  BigObject p_new(p_out.type());
  p_new.take("INEQUALITIES") << F_new/Pos/costfkt;
  p_new.take("EQUATIONS") << E_new.minor(basis(E_new).first,All);
  p_new.take("LP.LINEAR_OBJECTIVE") << costfkt;
  
  
  // check if the are a D as wanted
  if(!p_new.give("FEASIBLE")){
    Scalar s = 0;
    if(p_out.give("FEASIBLE")){
      Vector<Scalar> t = p_out.give("ONE_VERTEX");
      return std::make_pair(s,t);
    }else{
      Vector<Scalar> t(F_out.cols());
      t[0] = 1;
      return std::make_pair(\
              - std::numeric_limits<Scalar>::infinity(), t);
    }
  }
    
  Vector<Scalar> t_s_D = p_new.give("LP.MINIMAL_VERTEX");

  // check solution
  if (t_s_D[1]>0){
    // convert solution
    Scalar s = 1/t_s_D[1];
    Vector<Scalar> t( F_out.cols() );
    t[0] = 1;
    
    for(int i= 1; i<F_out.cols(); ++i){
      t[i] = - s*t_s_D[1+i];      
    }
    return std::make_pair(s, t);
  }else{
    // s*p_in is a subset of p_out for all s>0
    Scalar inf = std::numeric_limits<Scalar>::infinity();
    Vector<Scalar> t(F_out.cols());
    t[0] = 1;
    return std::make_pair(inf, t);
  }
  
}

template <typename Scalar>
std::pair<Scalar,Vector<Scalar>> optimal_contains_primal_primal(BigObject p_in, BigObject p_out){
   
  // load the vertex discription of p_out
  Matrix<Scalar> V_out = p_out.lookup("VERTICES | POINTS");
  Matrix<Scalar> L_out;

  if ( p_out.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_out){
    // add lines to vertices and rays
    V_out /= L_out/(-L_out);
  }
  
  // load the vertex discription of p_in
  Matrix<Scalar> V_in = p_in.lookup("VERTICES | POINTS");
  Matrix<Scalar> L_in;

  if ( p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_in){
    // add lines to vertices and rays
    V_in /= L_in/(-L_in);
  }

  // build set of vertices of p_in
  Set<Int> vertices_in;

  for(int i=0;i<V_in.rows();++i){
    // check of i-th row is a ray or a vertex
    if ( !is_zero(V_in(i,0)) ){
      vertices_in += i;
    }
  }
  
  // construct a polyope on the matrix D, the vector t and s,
  // With 
  // [s*V + t^T ones_Vector]/[R]  = D*V_out 
  // where V is the matrix, which contains all Vertices of 
  // p_in, and R is the matrix, which contains all Rays of 
  // p_in
  Matrix<Scalar> E_new( V_in.rows()*V_in.cols(),
                        1 + V_in.cols() + V_in.rows()*V_out.rows() );
                            
  
  for(int i=0; i<V_in.rows(); ++i){
  
    for(int j=0; j<V_in.cols(); ++j){
      
      
      // check is V_in.row(i) is a vertex or ray
      if(vertices_in.contains(i)){
        if(j==0){
          // set condition for convex combination
          E_new(i*V_in.cols() + j, 0) = -V_in(i,j);
        }else{
          // set factor for s
          E_new(i*V_in.cols() + j, 1) = -V_in(i,j);
        
        
          // add minus t
          for(int k=1; k<V_in.cols(); ++k){
            E_new(i*V_in.cols() + j, 1+k) = -1;
          }
        }
      }else{
        // add conditions for the rays
        E_new(i*V_in.cols() + j, 0) = -V_in(i,j);
        
      }
      
      // add factors for D
      for(int l=0; l<V_out.rows(); ++l){
        E_new(i*V_in.cols() + j, 
              1 + V_in.cols() + i*V_out.rows() + l) = V_out(l,j);
      }
    }
  }
  
  // add non-negativity conditions for D
  Matrix<Scalar> Pos = zero_matrix<Scalar>(V_in.rows()*V_out.rows(),
                                           1 + V_in.cols())|
                       unit_matrix<Scalar>(V_in.rows()*V_out.rows());
  
  // define cost funktion
  Vector<Scalar> costfkt(1 + V_out.cols() + 
                         V_out.rows() * V_in.rows());
  costfkt[1] = 1;
  
  
  // define new Polytope
  BigObject p_new(p_out.type());
  p_new.take("INEQUALITIES") << Pos;
  p_new.take("EQUATIONS") << E_new.minor(basis(E_new).first,All);
  p_new.take("LP.LINEAR_OBJECTIVE") << costfkt;
  
  // check if p_new is feasible
  if(!p_new.give("FEASIBLE")){
    Scalar s = 0;
    Vector<Scalar> t = p_out.give("ONE_VERTEX");
    return std::make_pair(s, t);
  }
  
  Vector<Scalar> s_t_D = p_new.give("LP.MAXIMAL_VERTEX");

  Vector<Scalar> t(V_out.cols());
  t[0] = 1;
  
  t.slice(sequence(1,V_out.cols()-1)) = s_t_D.slice(
                                         sequence(2,V_out.cols()-1));


  return std::make_pair(s_t_D[1], t);
}

template <typename Scalar>
std::pair<Scalar,Vector<Scalar>> optimal_contains_dual_primal(BigObject p_in, BigObject p_out){
  // Since this problem is co-NP complete it will be changed to
  // it wil be changed to the case of primal_dual
  
  // load the outer description of p_in
  Matrix<Scalar> F_in = p_in.lookup("FACETS | INEQUALITIES");
  Matrix<Scalar> E_in;
  convex_hull_result<Scalar> hull_in;
  std::string got_property;
  
  if(p_in.lookup_with_property_name(
            "AFFINE_HULL | EQUATIONS", got_property) >> E_in){
    if(got_property == "EQUATIONS"){
      E_in = E_in.minor(basis(E_in).first,All);
    }
  }else{
    E_in = zero_matrix<Scalar>(0, F_in.cols());
  }
  
  // get a inner description of p_in
  hull_in = enumerate_vertices(F_in, E_in, true);
  
  
  BigObject p_in_new(p_in.type());
  p_in_new.take("POINTS") << hull_in.first;
  p_in_new.take("EQUATIONS") << hull_in.second;
  
  // load the inner description of p_out
  Matrix<Scalar> V_out = p_out.lookup("RAYS | INPUT_RAYS"); 
  Matrix<Scalar> L_out;
  convex_hull_result<Scalar> hull_out;
  
  if(p_out.lookup_with_property_name(
            "LINEALITY_SPACE | INPUT_LINEALITY",
            got_property) >> L_out){
    if(got_property == "INPUT_LINEALITY"){
      L_out = L_out.minor(basis(L_out).first,All);
    }
  }else{
    L_out = zero_matrix<Scalar>(0, V_out.cols());
  }
  
  // get a outer description of p_out
  hull_out = enumerate_facets(V_out, L_out, true);
  
  
  BigObject p_out_new(p_out.type());
  p_out_new.take("INEQUALITIES") << hull_out.first;
  p_out_new.take("EQUATIONS") << hull_out.second;
  
  
  return optimal_contains_primal_dual<Scalar>(p_in_new, p_out_new);
}

// Compute a vector t and the minimal scalar s, such that for 
// a given Polytope p_in, s*p_in + t is a subset of a other 
// given  Polytope p_out.
// For each combination of discriptions (by Vertices or by Facets)
// it use another algorithm.
// @param BigObject p_in    The inner Polytope
// @param BigObject p_out   the outer Polytope
// @return pair<Scalar,Vector>
template <typename Scalar>
std::pair<Scalar,Vector<Scalar>> optimal_contains(BigObject p_in, BigObject p_out)
{
   // check in which way p_out was given
   if (p_out.exists("FACETS | INEQUALITIES")){
    
      // check in which way p_in was given
      if (p_in.exists("RAYS | INPUT_RAYS")){
         //p_in is given by vertices
         
         return optimal_contains_primal_dual<Scalar>(p_in,p_out);

      }else{
         // p_in is given by inequalities
         
         return optimal_contains_dual_dual<Scalar>(p_in,p_out);
      }

   }else{
      // p_out is given by vertices
      // check in which way p_in was given 
      if (p_in.exists("RAYS | INPUT_RAYS")){
         return optimal_contains_primal_primal<Scalar>(p_in, p_out);


      }else{
         // p_in is given by inequalities
         return optimal_contains_dual_primal<Scalar>(p_in,p_out);

      }
   }

}

// now comes the part of optimal containment for polytopes
// with balls




template <typename Scalar>
std::pair<Scalar,Vector<Scalar>> minimal_ball_primal(BigObject p_in){
  
  // load vertices and rays
  Matrix<Scalar> V_in = p_in.lookup("VERTICES | POINTS");
  
  // set dimension of the space (not homogenized)
  const int d = int(V_in.cols()) - 1;
  
  
  // store the vertices in a list of std vectors
  // and check that there are no rays
  std::list<std::vector<Scalar>> vertices;
  for(int i=0; i<V_in.rows(); ++i){
    std::vector<Scalar> v(d);
    for(int j=1; j<V_in.cols(); ++j){
      v[j-1] = V_in(i,j);
    }
    if(V_in.row(i)[0]!=0){
      vertices.push_back(v);
    }else{
      Scalar inf = std::numeric_limits<Scalar>::infinity();
      Vector<Scalar> t(d+1);
      t[0] = 1;
      return std::make_pair(inf, t);
    }
  }
  
  // check if the lineality space is empty
  Matrix<Scalar> L_in;
  if(p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_in){
    L_in = remove_zero_rows(L_in);
    if(L_in.rows()>0){
      Scalar inf = std::numeric_limits<Scalar>::infinity();
      return std::make_pair(inf,
                            zero_vector<Scalar>(d));
    }
  }
  
  // define the types of iterators through the points and 
  // their coordinates
  typedef typename std::list<std::vector<Scalar> >::const_iterator PointIterator; 
  typedef typename std::vector<Scalar>::const_iterator CoordIterator;

  // create an instance of Miniball
  typedef Miniball::Miniball <Miniball::CoordAccessor<
                      PointIterator, CoordIterator>> MB;
  MB mb (d, vertices.begin(), vertices.end());

  //get results
  //center
  const Scalar* center = mb.center();
  Vector<Scalar> c(d+1);
  c[0] = 1;
  for(int i=0; i<d; ++i){
    c[i+1] = Scalar(center[i]);
  }
  
  //radius
  Scalar r = Scalar(mb.squared_radius());
  
  // number of support points
  Scalar nr = Scalar(mb.nr_support_points());
  
  // relative error: by how much does the ball fail to contain all 
  //                 points? 
  //                 tiny positive numbers come from roundoff and
  //                 are ok
  // suboptimality: by how much does the ball fail to be the smallest
  //                enclosing ball of its support points? should 
  //                be 0 
  //                in most cases, but tiny positive numbers are
  //                again ok
  Scalar suboptimality;
  Scalar relative_error = Scalar(mb.relative_error (suboptimality));

  // check if the out put is not correct and warn the user
  if(suboptimality > 0 or relative_error>0 ){
    cout << "The solution is not correct";
    cout << endl << "Number of support points:\t";
    cout << nr << endl;
    cout << "Relative error:\t";
    cout << relative_error << endl;
    cout << "(how much does the ball fail to contain all points)" << endl;
    cout << "Suboptimality:\t";
    cout << suboptimality << endl;
    cout << "(how much does the ball fail to be the smallest enclosing ball of its support points)" << endl << endl;
  }
  return std::make_pair(r, c);
  //return std::make_pair(Scalar(1), zero_vector<Scalar>(2));

}

template <typename Scalar>
std::pair<Scalar,Vector<Scalar>> minimal_ball_dual(BigObject p_in){
  // Since this problem is co-NP complete it will be changed to
  // it wil be changed to the case of ball_dual
  
  // load the outer description of p_in
  Matrix<Scalar> F_in = p_in.lookup("FACETS | INEQUALITIES"); 
  Matrix<Scalar> E_in;
  convex_hull_result<Scalar> hull_in;
  std::string got_property;
  
  if(p_in.lookup_with_property_name("AFFINE_HULL | EQUATIONS",\
            got_property) >> E_in){
    if(got_property == "EQUATIONS"){
      E_in = E_in.minor(basis(E_in).first,All);
    }
  }else{
    E_in = zero_matrix<Scalar>(0, F_in.cols());
  }
  
  // get a outer description of p_out
  hull_in = enumerate_facets(F_in, E_in, true);
  
  
  BigObject p_in_new(p_in.type());
  p_in_new.take("POINTS") << hull_in.first;
  p_in_new.take("INPUT_LINEALITY") << hull_in.second;
  
  
  return minimal_ball_primal<Scalar>(p_in_new);
}



// Compute a vector c and the maximal scalar r, such that for 
// a given Polytope p_in is a subset of the Ball B(c,r).
// For each combination of discriptions (by Vertices or by 
// Facets) it use another algorithm.
// @param BigObject p_out   the outer Polytope
// @return pair<Scalar,Vector>
template <typename Scalar>
std::pair<Scalar,Vector<Scalar>> minimal_ball(BigObject p_in)
{   
  // check in which way p_in was given
  if (p_in.exists("VERTICES | POINTS")){    
    return minimal_ball_primal<Scalar>(p_in);
    
  }else{
    // p_in was given by facets
    return minimal_ball_dual<Scalar>(p_in);
  }
}

}}

