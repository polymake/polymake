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

#ifndef POLYMAKE_POLYTOPE_GENERIC_LP_CLIENT_H
#define POLYMAKE_POLYTOPE_GENERIC_LP_CLIENT_H

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/polytope/solve_LP.h"

namespace polymake { namespace polytope {

template <typename Scalar, typename Ineq>
Set<int> initial_basis_from_known_vertex(const GenericMatrix<Ineq, Scalar>& H, const Vector<Scalar>& V)
{
   const Set<int> zero_rows = orthogonal_rows(H, V);
   const Set<int> basis_zero_rows = basis_rows(H.minor(zero_rows, All));
   if (basis_zero_rows.size() == H.cols() - 1)
      return select(zero_rows, basis_zero_rows);
   return Set<int>();
}

template <typename Scalar>
void store_LP_Solution(perl::Object& p, perl::Object& lp, bool maximize, const LP_Solution<Scalar>& S)
{
   if (S.status == LP_status::valid) {
      lp.take(maximize ? Str("MAXIMAL_VALUE") : Str("MINIMAL_VALUE")) << S.objective_value;
      lp.take(maximize ? Str("MAXIMAL_VERTEX") : Str("MINIMAL_VERTEX")) << S.solution;
      p.take("FEASIBLE") << true;

   } else if (S.status == LP_status::unbounded) {
      if (maximize)
         lp.take("MAXIMAL_VALUE") << std::numeric_limits<Scalar>::infinity();
      else
         lp.take("MINIMAL_VALUE") << -std::numeric_limits<Scalar>::infinity();
      p.take("FEASIBLE") << true;
   } else {
      p.take("FEASIBLE") << false;
   }

   if (S.lineality_dim >= 0)
      p.take("LINEALITY_DIM") << S.lineality_dim;
}

template <typename Scalar, typename Solver, typename=void>
struct allow_initial_basis : std::false_type {

   bool select_arg(bool feasibility_known, const Set<int>&) const
   {
      return feasibility_known;
   }
};

template <typename Scalar, typename Solver>
struct allow_initial_basis<Scalar, Solver,
   accept_valid_type<decltype(std::declval<Solver>().solve(std::declval<Matrix<Scalar>>(), std::declval<Matrix<Scalar>>(),
                                                           std::declval<Vector<Scalar>>(), true, std::declval<Set<int>>()))>> : std::true_type {

   const Set<int>& select_arg(bool, const Set<int>& initial_basis) const
   {
      return initial_basis;
   }
};

template <typename Scalar, typename Solver>
void generic_lp_client(perl::Object& p, perl::Object& lp, bool maximize, const Solver& LP_solver)
{
   std::string H_name;
   const Matrix<Scalar> H = LP_solver.needs_feasibility_known()
                            ? p.give_with_property_name("FACETS | INEQUALITIES", H_name)
                            : p.give("FACETS | INEQUALITIES"),
                        E = p.lookup("AFFINE_HULL | EQUATIONS");
   const Vector<Scalar> Obj = lp.give("LINEAR_OBJECTIVE");

   if (H.cols() != E.cols() &&
       H.cols() && E.cols())
      throw std::runtime_error("lp_client - dimension mismatch between Inequalities and Equations");

   Set<int> initial_basis;
   const allow_initial_basis<Scalar, Solver> allow_basis{};

   if (allow_basis) {
      const Vector<Scalar> V = p.lookup("ONE_VERTEX");
      if (V.dim())
         initial_basis = E.rows() ? initial_basis_from_known_vertex(H/E, V) : initial_basis_from_known_vertex(H, V);
   }

   const bool feasibility_known = LP_solver.needs_feasibility_known() && H_name == "FACETS";

   const LP_Solution<Scalar> S = LP_solver.solve(H, E, Obj, maximize, allow_basis.select_arg(feasibility_known, initial_basis));
   store_LP_Solution(p, lp, maximize, S);
}

} }

#endif // POLYMAKE_POLYTOPE_GENERIC_LP_CLIENT_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
