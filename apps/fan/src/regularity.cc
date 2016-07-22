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

       const Matrix<Scalar> rays = fan.give("RAYS");
       const IncidenceMatrix<> max_cones = fan.give("MAXIMAL_CONES");
       const Matrix<Scalar> ker = T(null_space(T(ones_vector<Scalar>() | rays)));
       const int dim = rays.rows() - rank(rays) + 1;
       // catch degenerate cases early on to avoid problems with empty matrices
       if (ker.rows() == 0 || ker.cols() == 0)
          return true;

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

       if (L.rows() > 0)
          L = L | zero_vector<Scalar>();

       perl::Object p(polytope_type);
       p.take("CONE_AMBIENT_DIM") << dim;
       p.take("INEQUALITIES") << A;
       p.take("EQUATIONS") << L;

       bool feasible = p.give("FEASIBLE");
       if (feasible) {
          perl::Object lp(linear_program_type);
          lp.take("LINEAR_OBJECTIVE") << unit_vector<Scalar>(dim, dim-1);
          p.add("LP", lp);

          const Scalar max = lp.give("MAXIMAL_VALUE");

          if (max > 0)
             return true;
       }

       return false;
    }

    FunctionTemplate4perl("regular<Scalar>(PolyhedralFan<Scalar>)");

} }
