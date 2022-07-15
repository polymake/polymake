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

#pragma once

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope {

enum class LP_status {
   valid, infeasible, unbounded, infeasibleOrUnbounded
};

template <typename Scalar>
struct LP_Solution {
   LP_status status;
   Scalar objective_value;
   Vector<Scalar> solution;
   Int lineality_dim = -1;
};

template <typename Scalar>
class LP_Solver {
public:
   virtual ~LP_Solver() {}

   virtual LP_Solution<Scalar> solve(const Matrix<Scalar>& inequalities, const Matrix<Scalar>& equations,
                                     const Vector<Scalar>& objective, bool maximize, bool feasibility_known=false) const = 0;

   virtual bool needs_feasibility_known() const { return false; }
};

template <typename Scalar>
using cached_LP_solver = CachedObjectPointer<LP_Solver<Scalar>, Scalar>;

template <typename Scalar>
const LP_Solver<Scalar>& get_LP_solver()
{
   cached_LP_solver<Scalar> solver_ptr("polytope::create_LP_solver");
   return solver_ptr.get();
}

// convenience wrappers
template <typename Scalar, typename Matrix1, typename Matrix2, typename Vector3>
LP_Solution<Scalar> solve_LP(const GenericMatrix<Matrix1, Scalar>& inequalities, const GenericMatrix<Matrix2, Scalar>& equations,
                             const GenericVector<Vector3, Scalar>& objective, bool maximize)
{
   const LP_Solver<Scalar>& solver = get_LP_solver<Scalar>();
   return solver.solve(convert_to_persistent_dense(inequalities.top()),
                       convert_to_persistent_dense(equations.top()),
                       convert_to_persistent_dense(objective.top()), maximize);
}

template <typename Scalar, typename Matrix1, typename Vector3>
LP_Solution<Scalar> solve_LP(const GenericMatrix<Matrix1, Scalar>& inequalities,
                             const GenericVector<Vector3, Scalar>& objective, bool maximize)
{
   return solve_LP(inequalities, Matrix<Scalar>(), objective, maximize);
}

template <typename Scalar, typename Matrix1, typename Matrix2>
bool H_input_feasible(const GenericMatrix<Matrix1, Scalar>& inequalities, const GenericMatrix<Matrix2, Scalar>& equations)
{
   if (inequalities.cols() != equations.cols() &&
       inequalities.cols() && equations.cols())
      throw std::runtime_error("H_input_feasible - dimension mismatch between Inequalities and Equations");

   const Int d = std::max(inequalities.cols(), equations.cols());
   if (d == 0)
      return true;

   const LP_Solution<Scalar> S = solve_LP(inequalities, equations, unit_vector<Scalar>(d, 0), true);
   return S.status != LP_status::infeasible;
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
