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

#include "polymake/client.h"
#include "polymake/polytope/soplex_interface.h"
#include "polymake/polytope/generic_lp_client.h"

namespace polymake { namespace polytope { namespace soplex_interface {

template <typename Scalar=Rational>
auto create_LP_solver()
{
   return cached_LP_solver<Rational>(new Solver(), true);
}

}

void soplex_lp_client(BigObject p, BigObject lp, bool maximize, OptionSet options)
{
   const Matrix<Rational> H = p.give("FACETS | INEQUALITIES");
   const Matrix<Rational> E = p.lookup("AFFINE_HULL | EQUATIONS");
   const Vector<Rational> Obj = lp.give("LINEAR_OBJECTIVE");

   const Set<Int> initial_basis = options["initial_basis"];
   const soplex_interface::Solver solver{};
   store_LP_Solution(p, lp, maximize, solver.solve(H, E, Obj, maximize, initial_basis));
}

Function4perl(&soplex_lp_client, "soplex_lp_client(Polytope<Rational>, LinearProgram<Rational>, $; {initial_basis => undef})");

InsertEmbeddedRule("function soplex.simplex: create_LP_solver<Scalar> [Scalar==Rational] () : c++ (name => 'soplex_interface::create_LP_solver') : returns(cached);\n");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
