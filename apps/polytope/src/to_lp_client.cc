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
#include "polymake/linalg.h"
#include "polymake/polytope/to_interface.h"
#include "polymake/polytope/generic_lp_client.h"

namespace polymake { namespace polytope { namespace to_interface {

template <typename Scalar>
auto create_LP_solver()
{
   return cached_LP_solver<Scalar>(new Solver<Scalar>(), true);
}

}

template <typename Scalar>
void to_lp_client(perl::Object p, perl::Object lp, bool maximize)
{
   generic_lp_client<Scalar>(p, lp, maximize, to_interface::Solver<Scalar>());
}

FunctionTemplate4perl("to_lp_client<Scalar> (Polytope<Scalar>, LinearProgram<Scalar>, $)");

InsertEmbeddedRule("function to.simplex: create_LP_solver<Scalar> [is_ordered_field_with_unlimited_precision(Scalar)] ()"
                   " : c++ (name => 'to_interface::create_LP_solver') : returns(cached);\n");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
