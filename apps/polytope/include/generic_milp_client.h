/* Copyright (c) 1997-2021
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

#pragma once

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/polytope/solve_MILP.h"

namespace polymake { namespace polytope {


template <typename Scalar>
void store_MILP_Solution(BigObject& p, BigObject& milp, bool maximize, const MILP_Solution<Scalar>& S)
{
   if (S.status == LP_status::valid) {
      milp.take(maximize ? Str("MAXIMAL_VALUE") : Str("MINIMAL_VALUE")) << S.objective_value;
      milp.take(maximize ? Str("MAXIMAL_SOLUTION") : Str("MINIMAL_SOLUTION")) << S.solution;

   } else if (S.status == LP_status::unbounded) {
      if (maximize)
         milp.take("MAXIMAL_VALUE") << std::numeric_limits<Scalar>::infinity();
      else
         milp.take("MINIMAL_VALUE") << -std::numeric_limits<Scalar>::infinity();
   }
}


template <typename Scalar, typename Solver>
void generic_milp_client(BigObject& p, BigObject& milp, bool maximize, const Solver& MILP_solver)
{
   std::string H_name;
   const Matrix<Scalar> H = p.give("FACETS | INEQUALITIES"),
                        E = p.lookup("AFFINE_HULL | EQUATIONS");
   const Vector<Scalar> Obj = milp.give("LINEAR_OBJECTIVE");
   Set<Int> integerVariables;
   milp.lookup("INTEGER_VARIABLES") >> integerVariables;
   // Default to taking all variables as integer, since otherwise one should be
   // using a LP solver.
   if(integerVariables.size() == 0){
      integerVariables = pm::sequence(0,Obj.dim());
   }


   if (H.cols() != E.cols() &&
       H.cols() && E.cols())
      throw std::runtime_error("milp_client - dimension mismatch between Inequalities and Equations");

   const MILP_Solution<Scalar> S = MILP_solver.solve(H, E, Obj, integerVariables, maximize);
   store_MILP_Solution(p, milp, maximize, S);
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
