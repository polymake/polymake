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

#ifndef POLYMAKE_POLYTOPE_SOPLEX_INTERFACE_H
#define POLYMAKE_POLYTOPE_SOPLEX_INTERFACE_H

#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/polytope/lpch_dispatcher.h"
#include "polymake/Set.h"
#include "polymake/Rational.h"

namespace polymake { namespace polytope { namespace soplex_interface {

class solver
{
public:
   solver()
   {
#if POLYMAKE_DEBUG
      debug_print = perl::get_debug_level() > 1;
#endif
   };

   Set<int> initial_basis;

   typedef std::pair<Rational, Vector<Rational> > lp_solution;

   /// Solve lp
   /// @returns first: objective value, second: solution
   lp_solution
   solve_lp(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,
            const Vector<Rational>& Objective, bool maximize);

   void set_basis(const Set<int>& basis);

#if POLYMAKE_DEBUG
   bool debug_print;
#endif
};


} } }

#endif // POLYMAKE_POLYTOPE_SOPLEX_INTERFACE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
