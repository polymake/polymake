/* Copyright (c) 1997-2018
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

namespace polymake { namespace polytope {

template<typename Scalar>
perl::Object tensor(perl::Object p_in1, perl::Object p_in2)
{
   const bool bounded1=p_in1.give("BOUNDED"),
      bounded2=p_in2.give("BOUNDED");
   if (!bounded1 || !bounded2)
      throw std::runtime_error("tensor: both input polyhedra must be bounded");
  
   const Matrix<Scalar> V1=p_in1.give("VERTICES | POINTS"),
      V2=p_in2.give("VERTICES | POINTS");
   const int
      adim1=V1.cols()-1,
      adim2=V2.cols()-1,
      adim_out=adim1*adim2,
      n_points_out=V1.rows()*V2.rows();
  
   Matrix<Scalar> V_out(n_points_out, adim_out+1);

   copy_range(entire(product(rows(V1.minor(All,range(1,adim1))), rows(V2.minor(All,range(1,adim2))), operations::tensor())),
              rows(V_out.minor(All,range(1,adim_out)).top()).begin());
   V_out.col(0).fill(1);

   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.set_description() << "Tensor-product of " << p_in1.name() << " and " << p_in2.name() << endl;;
  
   p_out.take("POINTS") << V_out;
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct a new polytope as the convex hull of the tensor products of the vertices of two"
                          "# polytopes //P1// and //P2//."
                          "# Unbounded polyhedra are not allowed. Does depend on the vertex coordinates of the input."
                          //  "# The option @c -relabel creates an additional section @see VERTEX_LABELS.\n"
                          //  "# The label of a new vertex $v_1 &otimes; v_2$ will have the form LABEL_1xLABEL_2\n"
                          //FIXME: Implement an option relabel or delete description!"
                          "# @param Polytope P1"
                          "# @param Polytope P2"
                          "# @return Polytope"
                          "# @example The following creates the tensor product polytope of two squares and then prints its vertices."
                          "# > $p = tensor(cube(2),cube(2));"
                          "# > print $p->VERTICES;"
                          "# | 1 1 1 1 1"
                          "# | 1 -1 1 -1 1"
                          "# | 1 1 -1 1 -1"
                          "# | 1 -1 1 1 -1"
                          "# | 1 1 1 -1 -1"
                          "# | 1 1 -1 -1 1"
                          "# | 1 -1 -1 1 1"
                          "# | 1 -1 -1 -1 -1",
                          "tensor<Scalar>(Polytope<type_upgrade<Scalar>> Polytope<type_upgrade<Scalar>>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
