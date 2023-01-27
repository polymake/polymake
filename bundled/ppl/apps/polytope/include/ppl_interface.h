/* Copyright (c) 1997-2023
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

#include "polymake/polytope/solve_LP.h"
#include "polymake/polytope/convex_hull.h"

namespace polymake { namespace polytope { namespace ppl_interface {

template <typename Scalar>
class ConvexHullSolver : public polytope::ConvexHullSolver<Scalar> {
public:
   convex_hull_result<Scalar>
   enumerate_facets(const Matrix<Scalar>& Points, const Matrix<Scalar>& Lineality, const bool isCone) const override;

   convex_hull_result<Scalar>
   enumerate_vertices(const Matrix<Scalar>& Inequalities, const Matrix<Scalar>& Equations, const bool isCone) const override; 
};

template <typename Scalar>
class LP_Solver : public polytope::LP_Solver<Scalar> {
public:
   LP_Solution<Scalar>
   solve(const Matrix<Scalar>& Inequalities, const Matrix<Scalar>& Equations, const Vector<Scalar>& Objective, bool maximize, bool=false) const override;
};

} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
