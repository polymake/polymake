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
#include "polymake/polytope/soplex_interface.h"


namespace polymake { namespace polytope {


void soplex_solve_lp(perl::Object p, perl::Object lp, bool maximize, perl::OptionSet options)
{
   typedef soplex_interface::solver Solver;
   const Matrix<Rational> H = p.give("FACETS | INEQUALITIES");
   const Matrix<Rational> E = p.lookup("AFFINE_HULL | EQUATIONS");
   const Vector<Rational> Obj = lp.give("LINEAR_OBJECTIVE");

   try
   {
      Solver solver;
      if ( options.exists("initial_basis") )
      {
         const Set<int> basis = options["initial_basis"];
         solver.set_basis(basis);
      }

      typename Solver::lp_solution S = solver.solve_lp(H, E, Obj, maximize);
      lp.take(maximize ? "MAXIMAL_VALUE" : "MINIMAL_VALUE") << S.first;
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << S.second;
      p.take("FEASIBLE") << true;
   }
   catch (infeasible)
   {
      lp.take(maximize ? "MAXIMAL_VALUE" : "MINIMAL_VALUE") << perl::undefined();
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << perl::undefined();
      p.take("FEASIBLE") << false;
   }
   catch (unbounded)
   {
      if (maximize)
         lp.take("MAXIMAL_VALUE") << std::numeric_limits<Rational>::infinity();
      else
         lp.take("MINIMAL_VALUE") << -std::numeric_limits<Rational>::infinity();
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << perl::undefined();
      p.take("FEASIBLE") << true;
   }
}

FunctionTemplate4perl("soplex_solve_lp(Polytope<Rational>, LinearProgram<Rational>, $; {initial_basis => undef}) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
