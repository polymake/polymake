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

#pragma once

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/polytope/convex_hull.h"


namespace polymake{ namespace polytope{

template <typename Scalar>
bool contains_primal_dual(BigObject p_in, BigObject p_out){
   Matrix<Scalar> F_out = p_out.lookup("FACETS | INEQUALITIES");
   Matrix<Scalar> E_out;

   if (p_out.lookup("LINEAR_SPAN | EQUATIONS") >> E_out){
      // write equations as inequalities
      F_out /= E_out/(-E_out);
   }

   // get the Vertex description of the second polytopr
   Matrix<Scalar> V_in = p_in.give("RAYS | INPUT_RAYS");
   Matrix<Scalar> L_in;

   // check if vertices fulfill the inequalities
   Matrix<Scalar> b = F_out * T(V_in);
   for(int i=0;i<b.rows();++i){
      for(int j=0;j<b.cols();++j){
         if (b(i,j) < 0){return false;}
      }
   }

   if (p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_in){
      b = F_out * T(L_in);
      for(int i=0; i<b.rows();++i){
         for(int j=0;j<b.cols();++j){
            if (!is_zero(b(i,j))){return false;}
         }
      }
   }
   return true;
};

template <typename Scalar>
bool contains_dual_dual(BigObject p_in, BigObject p_out){
   Matrix<Scalar> F_out = p_out.lookup("FACETS | INEQUALITIES");
   Matrix<Scalar> E_out;

   if (p_out.lookup("LINEAR_SPAN | EQUATIONS") >> E_out){
      // write equations as inequalities
      F_out /= E_out/(-E_out);
   }

   Matrix<Scalar> F_in = p_in.lookup("FACETS | INEQUALITIES");
   Matrix<Scalar> E_in;

   if (p_in.lookup("LINEAR_SPAN | EQUATIONS") >> E_in){
      // write equations as inequalities
      F_in /= E_in/(-E_in);
   }
   // now we have to check if the is a matrix D with possitiv entries for which D * F_in = F_out
   // by the homogenization it means for polyeder D*A_in = A_out and D*b_in <= b_out
   // to find this we define a new H-polyhedra where D is represented by a vector
   
   Matrix<Scalar> E_new( F_out.rows() * F_out.cols(),
                         F_out.rows() * F_in.rows() + 1 );
                         
   int k = 0;

   for(int i=0; i<F_out.rows(); ++i){
      for(int j=0; j<F_in.cols(); ++j){
         E_new(k,0) = - F_out(i,j);

         for(int l=0; l<F_in.rows(); ++l){
            E_new(k, i*F_in.rows() + l + 1) = F_in(l,j);
         }

         k++;
      }
   }

   // add non-negativity constrains
   Matrix<Scalar> pos = zero_vector<Scalar>(F_out.rows() * 
                          F_in.rows())
                        |unit_matrix<Scalar>(F_out.rows() *
                          F_in.rows());

   BigObject p_new("Polytope", mlist<Scalar>());
   p_new.take("EQUATIONS") << E_new.minor(basis(E_new).first,All);
   p_new.take("INEQUALITIES") << pos;

   return p_new.give("FEASIBLE");
};

template <typename Scalar>
bool contains_primal_primal(BigObject p_in, BigObject p_out)
{
   // load the vertex discription of p_out
   Matrix<Scalar> V_out = p_out.lookup("RAYS | INPUT_RAYS");
   Matrix<Scalar> L_out;

   if ( p_out.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_out){
      // add lines to vertices and rays
      V_out /= L_out/(-L_out);
   }

   // load the vertex discription of p_in
   Matrix<Scalar> V_in = p_in.lookup("RAYS | INPUT_RAYS");
   Matrix<Scalar> L_in;

   if (p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_in){
      V_in /= L_in/(-L_in);
   }

   // so p_in is a subset of p_out iff the exists a  matrix D>=0 with
   // V_in = D V_out

   Matrix<Scalar> E_new( V_in.rows() * V_in.cols(),
                         V_in.rows() * V_out.rows() + 1);
   
   for(int i=0; i<V_in.rows(); ++i){
      for(int j=0; j<V_in.cols(); ++j){
         E_new(i*V_in.cols() +j, 0) = - V_in(i,j);

         for(int l=0; l<V_out.rows(); ++l){
            E_new(i*V_in.cols() +j, i*V_out.rows() + l + 1) = 
		V_out(l,j);
         }
      }
   }

   // non-negativity conditions
   Matrix<Scalar> pos( V_in.rows() * V_out.rows(),
                       V_in.rows() * V_out.rows() + 1);

   for(int i=0; i<V_in.rows()*V_out.rows(); ++i){
      pos(i,i+1) = 1;
   }
   
   BigObject p_new("Polytope", mlist<Scalar>());
   p_new.take("EQUATIONS") << E_new.minor(basis(E_new).first,All);
   p_new.take("INEQUALITIES") << pos;

   return p_new.give("FEASIBLE");

};

template <typename Scalar>
bool contains_dual_primal(BigObject p_in, BigObject p_out){
  // Since this problem is co-NP complete it will be changed to
  // it wil be changed to the case of primal_dual
  
  // load the outer description of p_in
  Matrix<Scalar> F_in = p_in.lookup("FACETS | INEQUALITIES");
  Matrix<Scalar> E_in;
  convex_hull_result<Scalar> hull_in;
  std::string got_property;
  
  if(p_in.lookup_with_property_name(
            "LINEAR_SPAN | EQUATIONS", got_property) >> E_in){
    if(got_property == "EQUATIONS"){
      E_in = E_in.minor(basis(E_in).first,All);
    }
  }else{
    E_in = zero_matrix<Scalar>(0, F_in.cols());
  }
  
  // get a inner description of p_in
  hull_in = enumerate_vertices(F_in, E_in, true);
  
  
  BigObject p_in_new(p_in.type());
  p_in_new.take("INPUT_RAYS") << hull_in.first;
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
  
  
  return contains_primal_dual<Scalar>(p_in_new, p_out_new);
}

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
   if(dim_in != dim_out) return false;

   if(p_in.isa("Polytope") && p_out.isa("Polytope")){
      const bool feasible_in = p_in.give("FEASIBLE");
      const bool feasible_out = p_out.give("FEASIBLE");
      if(!feasible_out && feasible_in) return false;
   }

   // check in which way p_out was given
   if (p_out.exists("FACETS | INEQUALITIES")){

      // check in which way p_in was given
      if (p_in.exists("RAYS | INPUT_RAYS")){
         return contains_primal_dual<Scalar>(p_in,p_out);

      }else{
         // p_i is given by inequalities
         return contains_dual_dual<Scalar>(p_in,p_out);
      }

   }else{
      // p_out is given by vertices
      // check in which way p_in was given 
      if (p_in.exists("RAYS | INPUT_RAYS")){
         return contains_primal_primal<Scalar>(p_in, p_out);


      }else{
        // p_in is given by inequalities
        return contains_dual_primal<Scalar>(p_in,p_out);

      }
   }
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
  
  // load the inner description of p_out
  Matrix<Scalar> V_out = p_out.lookup("RAYS | INPUT_RAYS"); 
  Matrix<Scalar> L_out;
  convex_hull_result<Scalar> hull_out;
  
  std::string got_property;
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
  
  
  return contains_ball_dual<Scalar>(c, r, p_out_new);
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
  
  // load the outer description of p_in
  Matrix<Scalar> F_in = p_in.lookup("FACETS | INEQUALITIES");
  Matrix<Scalar> E_in;
  convex_hull_result<Scalar> hull_in;
  std::string got_property;
  
  if(p_in.lookup_with_property_name(\
            "LINEAR_SPAN | EQUATIONS", got_property) >> E_in){
    if(got_property == "EQUATIONS"){
      E_in = E_in.minor(basis(E_in).first,All);
    }
  }else{
    E_in = zero_matrix<Scalar>(0, F_in.cols());
  }
  
  // get a inner description of p_in
  hull_in = enumerate_vertices(F_in, E_in, true);
  
  
  BigObject p_in_new(p_in.type());
  p_in_new.take("INPUT_RAYS") << hull_in.first;
  p_in_new.take("EQUATIONS") << hull_in.second;
  
  return contains_primal_ball<Scalar>(p_in_new, c, r);
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


}}
