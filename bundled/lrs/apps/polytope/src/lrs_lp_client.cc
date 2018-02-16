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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/polytope/lrs_interface.h"

namespace polymake { namespace polytope {

void lrs_solve_lp(perl::Object p, perl::Object lp, bool maximize)
{
   typedef lrs_interface::solver Solver;
   const Matrix<Rational> H=p.give("FACETS | INEQUALITIES"),
                          E=p.lookup("AFFINE_HULL | EQUATIONS");
   const Vector<Rational> Obj=lp.give("LINEAR_OBJECTIVE");

   if (H.cols() != E.cols() &&
       H.cols() && E.cols())
      throw std::runtime_error("lrs_solve_lp - dimension mismatch between Inequalities and Equations");

   int lineality_dim;

   try {
      Solver solver;
      Solver::lp_solution S=solver.solve_lp(H, E, Obj, maximize, &lineality_dim);
      lp.take(maximize ? "MAXIMAL_VALUE" : "MINIMAL_VALUE") << S.first;
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << S.second;
      p.take("FEASIBLE") << true;
      p.take("LINEALITY_DIM") << lineality_dim;
   }
   catch (const unbounded&) {
      if (maximize)
         lp.take("MAXIMAL_VALUE") << std::numeric_limits<Rational>::infinity();
      else
         lp.take("MINIMAL_VALUE") << -std::numeric_limits<Rational>::infinity();
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << perl::undefined();
      p.take("FEASIBLE") << true;
      p.take("LINEALITY_DIM") << lineality_dim;
   }
   catch (const infeasible&) {
      lp.take(maximize ? "MAXIMAL_VALUE" : "MINIMAL_VALUE") << perl::undefined();
      lp.take(maximize ? "MAXIMAL_VERTEX" : "MINIMAL_VERTEX") << perl::undefined();
      p.take("FEASIBLE") << false;
      p.take("LINEALITY_DIM") << 0;
   }
}

void lrs_valid_point(perl::Object p)
{
   lrs_interface::solver solver;
   const Matrix<Rational> H=p.give("FACETS | INEQUALITIES"),
      E=p.lookup("LINEAR_SPAN | EQUATIONS");
   Vector<Rational> P;
   if (H.rows() && solver.check_feasibility(H,E,P)) {  // if H has no rows then the polytope is empty by definition
      p.take("VALID_POINT", perl::temporary) << P;
   } else {
      p.take("VALID_POINT") << perl::undefined();
   }
}

Function4perl(&lrs_solve_lp, "lrs_solve_lp(Polytope<Rational>, LinearProgram<Rational>, $)");
Function4perl(&lrs_valid_point, "lrs_valid_point(Cone<Rational>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
