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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/polytope/ppl_interface.h"
#include "polymake/polytope/generic_convex_hull_client.h"

namespace polymake { namespace polytope { namespace ppl_interface {

template <typename Scalar=Rational>
auto create_convex_hull_solver()
{
   return cached_convex_hull_solver<Scalar>(new ConvexHullSolver<Scalar>(), true);
}

}

void ppl_ch_primal(BigObject p, bool isCone)
{
   generic_convex_hull_primal<Rational>(p, isCone, ppl_interface::ConvexHullSolver<Rational>());
}
    
void ppl_ch_dual(BigObject p, bool isCone)
{
   generic_convex_hull_dual<Rational>(p, isCone, ppl_interface::ConvexHullSolver<Rational>());
}

Function4perl(&ppl_ch_primal, "ppl_ch_primal(Cone<Rational>; $=true)");
Function4perl(&ppl_ch_dual, "ppl_ch_dual(Cone<Rational>; $=true)");

Function4perl(&ppl_ch_primal, "ppl_ch_primal(Polytope<Rational>; $=false)");
Function4perl(&ppl_ch_dual, "ppl_ch_dual(Polytope<Rational>; $=false)");

InsertEmbeddedRule("function ppl.convex_hull: create_convex_hull_solver<Scalar> [Scalar==Rational] ()"
                   " : c++ (name => 'ppl_interface::create_convex_hull_solver') : returns(cached);\n");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
