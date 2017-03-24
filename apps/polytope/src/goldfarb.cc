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
#include "polymake/Rational.h"
#include "polymake/SparseVector.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

template <typename Scalar>
perl::Object goldfarb(int d, const Scalar& e, const Scalar& g)
{
   // restriction on d, e, and g
   const int m=8 * sizeof(int) - 2; // maximal dimension that can be handled
   if (d < 1 || d > m)
      throw std::runtime_error("goldfarb: dimension ot of range (1.." + std::to_string(m) + ")");

   if (e>=Rational(1,2))
      throw std::runtime_error("goldfarb: e < 1/2");
   if (g>e/4)
      throw std::runtime_error("goldfarb: g <= e/4");

   perl::Object p(perl::ObjectType::construct<Scalar>("Polytope"));
   p.set_description() << "Goldfarb " << d << "-cube with parameters e=" << e << " and g=" << g << endl;

   Matrix<Scalar> IE(4+2*(d-2),d+1);

   // the first 4 inequalities
   IE(0,1)=1;
   IE(1,0)=1; IE(1,1)=-1;
   if(d > 1){
      IE(2,1)=-e; IE(2,2)=1;
      IE(3,0)=1; IE(3,1)=-e; IE(3,2)=-1;
   }
   for (int k=2; k<d; ++k) {
      int i=k*2;                  // row index
      IE(i,k-1)=e*g; IE(i,k)=-e; IE(i,k+1)=1;
      IE(i+1,0)=1; IE(i+1,k-1)=e*g; IE(i+1,k)=-e; IE(i+1,k+1)=-1;
   }
  
   p.take("INEQUALITIES") << IE;
   p.take("LP.LINEAR_OBJECTIVE") << unit_vector<Scalar>(d+1,d);
   p.take("FEASIBLE") << 1;
   p.take("BOUNDED") << 1;
   return p;
}

template <typename Scalar>
perl::Object goldfarb_sit(int d, const Scalar& eps, const Scalar& delta)
{
   // restriction on d, e, and g
   const int m=8 * sizeof(int) - 2; // maximal dimension that can be handled
   if (d < 2 || d > m)
      throw std::runtime_error("goldfarb_sit: dimension out of range (2.." + std::to_string(m) + ")");

   if (eps>=Rational(1,2)) // 1/theta
      throw std::runtime_error("goldfarb_sit: eps < 1/2");
   if (delta>Rational(1,2)) // 1/beta
      throw std::runtime_error("goldfarb_sit: delta <= 1/2");

   perl::Object p(perl::ObjectType::construct<Scalar>("Polytope"));
   Matrix<Scalar> IE(4+2*(d-2),d+1);

   // the last 2 inequalities
   IE(2*d-1,0)=delta; IE(2*d-1,d)=-delta; IE(2*d-1,d-1)=-1;
   IE(2*d-2,d)=delta; IE(2*d-2,d-1)=-1;

   for (int k=d-1; k>1; --k) {
      int i(2*k-1);
      IE(i,0)=eps*delta*IE(i+2,0); IE(i,k)=-delta; IE(i,k-1)=-1;
      IE(i-1,k)=delta; IE(i-1,k-1)=-1;
   }
   // the first 2 inequalities
   IE(1,0)=eps*IE(3,0); IE(1,1)=-1;
   IE(0,1)=1;

   Vector<Scalar> vec(d+1);
   vec[d]=1;
   for(int k=d-1;k>0;--k){
      vec[k]=delta*vec[k+1];
   }

   p.take("INEQUALITIES") << IE;
   p.take("LP.LINEAR_OBJECTIVE") << vec;
   p.take("FEASIBLE") << 1;
   p.take("ONE_VERTEX") << unit_vector<Scalar>(d+1,0);
   p.take("BOUNDED") << 1;
   return p;
}

UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                          "# Produces a //d//-dimensional variation of the Klee-Minty cube if //eps//<1/2 and //delta//<=1/2."
                          "# This Klee-Minty cube is scaled in direction x_(d-i) by (eps*delta)^i."
                          "# This cube is a combinatorial cube and yields a bad example"
                          "# for the Simplex Algorithm using the 'steepest edge' Pivoting Strategy."
                          "# Here we use a scaled description of the construction of Goldfarb and Sit."
                          "# @param Int d the dimension"
                          "# @param Scalar eps"
                          "# @param Scalar delta"
                          "# @return Polytope",
                          "goldfarb_sit<Scalar> [ is_ordered_field(type_upgrade<Scalar, Rational>) ](Int; type_upgrade<Scalar>=1/3, type_upgrade<Scalar>=((convert_to<Scalar>($_[1]))/4))");



UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                          "# Produces a //d//-dimensional Goldfarb cube if //e//<1/2 and //g//<=e/4."
                          "# The Goldfarb cube is a combinatorial cube and yields a bad example"
                          "# for the Simplex Algorithm using the Shadow Vertex Pivoting Strategy."
                          "# Here we use the description as a deformed product due to Amenta and Ziegler."
                          "# For //e//<1/2 and //g//=0 we obtain the Klee-Minty cubes."
                          "# @param Int d the dimension"
                          "# @param Scalar e"
                          "# @param Scalar g"
                          "# @return Polytope"
                          "# @author Nikolaus Witte",
                          "goldfarb<Scalar> [ is_ordered_field(type_upgrade<Scalar, Rational>) ](Int; type_upgrade<Scalar>=1/3, type_upgrade<Scalar>=((convert_to<Scalar>($_[1]))/4))");

InsertEmbeddedRule("# @category Producing a polytope from scratch"
                   "# Produces a //d//-dimensional Klee-Minty-cube if //e//<1/2."
                   "# Uses the [[goldfarb]] client with //g//=0."
                   "# @param Int d the dimension"
                   "# @param Scalar e"
                   "# @return Polytope\n"
                   "user_function klee_minty_cube<Scalar> [ is_ordered_field(type_upgrade<Scalar, Rational>) ](Int; type_upgrade<Scalar>=1/3) {\n"
                   "goldfarb<Scalar>($_[0],$_[1],0); }\n");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
