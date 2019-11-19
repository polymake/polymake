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

#ifndef POLYMAKE_POLYTOPE_SOLVE_MILP_H
#define POLYMAKE_POLYTOPE_SOLVE_MILP_H

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/polytope/solve_LP.h"

namespace polymake { namespace polytope {

template <typename Scalar>
struct MILP_Solution {
   LP_status status;
   Scalar objective_value;
   Vector<Scalar> solution;
   int lineality_dim = -1;
};

template <typename Scalar>
class MILP_Solver {
public:
   virtual ~MILP_Solver() {}

   virtual MILP_Solution<Scalar> solve(const Matrix<Scalar>& inequalities, const Matrix<Scalar>& equations,
                                     const Vector<Scalar>& objective, const Set<int>& integerVariables, bool maximize) const = 0;

};

template <typename Scalar>
using cached_MILP_solver = CachedObjectPointer<MILP_Solver<Scalar>, Scalar>;

template <typename Scalar>
const MILP_Solver<Scalar>& get_MILP_solver()
{
   cached_MILP_solver<Scalar> solver_ptr("polytope::create_MILP_solver");
   return solver_ptr.get();
}

// convenience wrappers
template <typename Scalar, typename Matrix1, typename Matrix2, typename Vector3>
MILP_Solution<Scalar> solve_MILP(const GenericMatrix<Matrix1, Scalar>& inequalities, const GenericMatrix<Matrix2, Scalar>& equations,
                             const GenericVector<Vector3, Scalar>& objective, const Set<int>& integerVariables, bool maximize)
{
   const MILP_Solver<Scalar>& solver = get_MILP_solver<Scalar>();
   return solver.solve(convert_to_persistent_dense(inequalities.top()),
                       convert_to_persistent_dense(equations.top()),
                       convert_to_persistent_dense(objective.top()), 
                       integerVariables,
                       maximize);
}

template <typename Scalar, typename Matrix1, typename Vector3>
MILP_Solution<Scalar> solve_MILP(const GenericMatrix<Matrix1, Scalar>& inequalities,
                             const GenericVector<Vector3, Scalar>& objective, const Set<int>& integerVariables, bool maximize)
{
   return solve_MILP(inequalities, Matrix<Scalar>(), objective, maximize);
}


} }

#endif // POLYMAKE_POLYTOPE_SOLVE_MILP_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
