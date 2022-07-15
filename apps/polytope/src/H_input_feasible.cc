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
#include "polymake/polytope/solve_LP.h"

namespace polymake { namespace polytope {

template <typename Scalar>
bool H_input_feasible(BigObject p)
{
   const Matrix<Scalar> I = p.lookup("FACETS | INEQUALITIES"),
                        E = p.lookup("LINEAR_SPAN | EQUATIONS");
   return H_input_feasible(I, E);
}

FunctionTemplate4perl("H_input_feasible<Scalar> (Polytope<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
