/* Copyright (c) 1997-2020
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

#ifndef __POLYMAKE_CONTAINS_H__
#define __POLYMAKE_CONTAINS_H__

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"


namespace polymake{ namespace polytope{

template <typename Scalar>
bool contains_primal_dual(BigObject p_in, BigObject p_out){
   Matrix<Scalar> F_out = p_out.lookup("FACETS | INEQUALITIES");
   Matrix<Scalar> E_out;

   if (p_out.lookup("LINEAR_SPAN | EQUATIONS") >> E_out){
      // write equations as inequalities
      F_out = F_out/E_out/(-E_out);
   }

   // get the Vertex description of the second polytopr
   Matrix<Scalar> V_in = p_in.give("RAYS | INPUT_RAYS");
   Matrix<Scalar> L_in;

   // check if vertices fulfill the inequalities
   Matrix<Scalar> b = F_out * T(V_in);
   for(int i=0;i<b.rows();++i){
      for(int j=0;j<b.cols();++j){
         if (b.row(i)[j] < 0){return false;}
      }
   }

   if (p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_in){
      b = F_out * T(L_in);
      for(int i=0; i<b.rows();++i){
         for(int j=0;j<b.cols();++j){
            if (b.row(i)[j] != 0){return false;}
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
      F_out = F_out/E_out/(-E_out);
   }

   Matrix<Scalar> F_in = p_in.lookup("FACETS | INEQUALITIES");
   Matrix<Scalar> E_in;

   if (p_in.lookup("LINEAR_SPAN | EQUATIONS") >> E_in){
      // write equations as inequalities
      F_in = F_in/E_in/(-E_in);
   }
   // now we have to check if the is a matrix D with possitiv entries for wich D * A_1 = A_2 and D * b1 <= b2
   // to find this we define a new H-polyhedra where D is represented by a vector
   Matrix<Scalar> E_new = zero_matrix<Scalar>( F_out.rows() * (F_out.cols()-1),\
                          F_out.rows() * F_in.rows() + 1 );
   int k = 0;

   for(int i=0; i<F_out.rows(); ++i){
      for(int j=1; j<F_in.cols(); ++j){
         E_new.row(k)[0] = - F_out.row(i)[j];

         for(int l=0; l<F_in.rows(); ++l){
            E_new.row(k)[i*F_in.rows() + l + 1] = F_in.row(l)[j];
         }

         k++;
      }
   }

   Matrix<Scalar> F_new = zero_matrix<Scalar>( F_out.rows(), \
                         F_out.rows() * F_in.rows() +1 );

   for(int i=0; i<F_out.rows(); i++){
      F_new.row(i)[0] = F_out.row(i)[0];

      for(int l=0; l<F_in.rows(); ++l){
          F_new.row(i)[i*F_in.rows() + l + 1] = F_in.row(l)[0];
      }
   }

   // add non-negativity constrains
   Matrix<Scalar> pos = zero_matrix<Scalar>( F_out.rows() * F_in.rows(), \
                       F_out.rows() * F_in.rows() + 1 );
   for(int i=0; i<F_out.rows()*F_in.rows(); ++i){
      pos.row(i)[i+1] = 1;
   }

   BigObject p_new("Polytope", mlist<Scalar>());
   p_new.take("EQUATIONS") << E_new.minor(basis(E_new).first,All);
   p_new.take("INEQUALITIES") << F_new/pos;

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
      V_out = V_out/L_out/(-L_out);
   }

   // load the 
   Matrix<Scalar> V_in = p_in.lookup("RAYS | INPUT_RAYS");
   Matrix<Scalar> L_in;

   if (p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> L_in){
      V_in = V_in/L_in/(-L_in);
   }

   // so p_in is a subset of p_out iff the exists a  matrix D>=0 with
   // V_in = D V_out

   Matrix<Scalar> E_new = zero_matrix<Scalar>(V_in.rows() * V_in.cols(), \
                          V_in.rows() * V_out.rows() + 1);
   for(int i=0; i<V_in.rows(); ++i){
      for(int j=0; j<V_in.cols(); ++j){
         E_new.row(i*V_in.cols() +j)[0] = - V_in.row(i)[j];

         for(int l=0; l<V_out.rows(); ++l){
            E_new.row(i*V_in.cols() +j)[i*V_out.rows() + l + 1] = \
                    V_out.row(l)[j];
         }
      }
   }

   // non-negativity conditions
   Matrix<Scalar> pos = zero_matrix<Scalar>(V_in.rows() * V_out.rows(),\
                                          V_in.rows() * V_out.rows() + 1);

   for(int i=0; i<V_in.rows()*V_out.rows(); ++i){
      pos.row(i)[i+1] = 1;
   }
   BigObject p_new("Polytope", mlist<Scalar>());
   p_new.take("EQUATIONS") << E_new.minor(basis(E_new).first,All);
   p_new.take("INEQUALITIES") << pos;

   return p_new.give("FEASIBLE");

};

// Checks if a given Polytope p_in is a subset of a other given  Polytope p_out.
// For each combination of discriptions (by Vertices or by Facets) it use another algorithm.
// @param perl::Object p_in    The inner Polytope
// @param perl::Object p_out   the outer Polytope
// @return Bool
template <typename Scalar>
bool contains(BigObject p_in, BigObject p_out)
{
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

         // this case isn't programmed yet
         p_out.give("FACETS");
         return contains_primal_dual<Scalar>(p_in,p_out);

      }
   }
}

}}
#endif // __POLYMAKE_CONTAINS_H__
