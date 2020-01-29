/* Copyright (c) 1997-2020
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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/polytope/lrs_interface.h"
#include "polymake/polytope/generic_lp_client.h"

namespace polymake { namespace polytope { namespace lrs_interface {

template <typename Scalar=Rational>
auto create_LP_solver()
{
   return cached_LP_solver<Rational>(new LP_Solver(), true);
}

}

void lrs_lp_client(BigObject p, BigObject lp, bool maximize)
{
   generic_lp_client<Rational>(p, lp, maximize, lrs_interface::LP_Solver());
}

void lrs_valid_point(BigObject p)
{
   const lrs_interface::LP_Solver solver{};
   const Matrix<Rational> H=p.give("FACETS | INEQUALITIES"),
      E=p.lookup("LINEAR_SPAN | EQUATIONS");
   Vector<Rational> P;
   if (H.rows() && solver.check_feasibility(H,E,P)) {  // if H has no rows then the polytope is empty by definition
      p.take("VALID_POINT", temporary) << P;
   } else {
      p.take("VALID_POINT") << Undefined();
   }
}

Function4perl(&lrs_lp_client, "lrs_lp_client(Polytope<Rational>, LinearProgram<Rational>, $)");
Function4perl(&lrs_valid_point, "lrs_valid_point(Cone<Rational>)");

InsertEmbeddedRule("function lrs.simplex: create_LP_solver<Scalar> [Scalar==Rational] () : c++ (name => 'lrs_interface::create_LP_solver') : returns(cached);\n");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
