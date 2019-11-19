/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_POLYTOPE_SOPLEX_INTERFACE_H
#define POLYMAKE_POLYTOPE_SOPLEX_INTERFACE_H

#include "polymake/polytope/solve_LP.h"
#include "polymake/Rational.h"
#if POLYMAKE_DEBUG
#include "polymake/client.h"
#endif

namespace polymake { namespace polytope { namespace soplex_interface {

class Solver : public LP_Solver<Rational> {
public:
   Solver()
#if POLYMAKE_DEBUG
      : debug_print(perl::get_debug_level() > 1)
#endif
   {}

   LP_Solution<Rational>
   solve(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,
         const Vector<Rational>& Objective, bool maximize, const Set<int>& initial_basis) const;

   LP_Solution<Rational>
   solve(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,
         const Vector<Rational>& Objective, bool maximize, bool=false) const override
   {
      return Solver::solve(Inequalities, Equations, Objective, maximize, Set<int>());
   }

private:
#if POLYMAKE_DEBUG
   const bool debug_print;
#endif
};

} } }

#endif // POLYMAKE_POLYTOPE_SOPLEX_INTERFACE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
