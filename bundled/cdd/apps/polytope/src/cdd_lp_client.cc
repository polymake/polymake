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
#include "polymake/polytope/cdd_interface.h"
#include "polymake/polytope/generic_lp_client.h"

namespace polymake { namespace polytope { namespace cdd_interface {

template <typename Scalar>
auto create_LP_solver()
{
   return cached_LP_solver<Scalar>(new LP_Solver<Scalar>(), true);
}

}

template <typename Scalar>
void cdd_lp_client(BigObject p, BigObject lp, bool maximize)
{
   generic_lp_client<Scalar>(p, lp, maximize, cdd_interface::LP_Solver<Scalar>());
}

FunctionTemplate4perl("cdd_lp_client<Scalar> [Scalar==Rational || Scalar==Float] (Polytope<Scalar>, LinearProgram<Scalar>, $)");

InsertEmbeddedRule("function cdd.simplex: create_LP_solver<Scalar> [Scalar==Rational || Scalar==Float] ()"
                   " : c++ (name => 'cdd_interface::create_LP_solver') : returns(cached);\n");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
