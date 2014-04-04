/* Copyright (c) 1997-2014
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
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

perl::Object tensor(perl::Object p_in1, perl::Object p_in2)
{
   const bool bounded1=p_in1.give("BOUNDED"),
      bounded2=p_in2.give("BOUNDED");
   if (!bounded1 || !bounded2)
      throw std::runtime_error("tensor: both input polyhedra must be bounded");
  
   const Matrix<Rational> V1=p_in1.give("VERTICES | POINTS"),
      V2=p_in2.give("VERTICES | POINTS");
   const int
      adim1=V1.cols()-1,
      adim2=V2.cols()-1,
      adim_out=adim1*adim2,
      n_points_out=V1.rows()*V2.rows();
  
   Matrix<Rational> V_out(n_points_out, adim_out+1);

   copy(entire(product(rows(V1.minor(All,range(1,adim1))), rows(V2.minor(All,range(1,adim2))), operations::tensor())),
        rows(V_out.minor(All,range(1,adim_out)).top()).begin());
   V_out.col(0).fill(1);

   perl::Object p_out("Polytope<Rational>");
   p_out.set_description() << "Tensor-product of " << p_in1.name() << " and " << p_in2.name() << endl;;
  
   p_out.take("POINTS") << V_out;
   const Matrix<Rational> empty;
   p_out.take("INPUT_LINEALITY") << empty;
   return p_out;
}

UserFunction4perl("# @category Producing a polytope from polytopes"
                  "# Construct a new polytope as the convex hull of the tensor products of the vertices of two"
                  "# polytopes //P1// and //P2//."
                  "# Unbounded polyhedra are not allowed. Does depend on the vertex coordinates of the input."
                  //  "# The option @c -relabel creates an additional section @see VERTEX_LABELS.\n"
                  //  "# The label of a new vertex $v_1 &otimes; v_2$ will have the form LABEL_1xLABEL_2\n"
                  //FIXME: Implement an option relabel or delete description!"
                  "# @param Polytope P1"
                  "# @param Polytope P2"
                  "# @return Polytope",
                  &tensor,"tensor(Polytope Polytope)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
