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

#ifndef POLYMAKE_POLYTOPE_CDD_INTERFACE_H
#define POLYMAKE_POLYTOPE_CDD_INTERFACE_H

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Bitset.h"
#include "polymake/permutations.h"
#include "polymake/polytope/solve_LP.h"
#include "polymake/polytope/convex_hull.h"

namespace polymake { namespace polytope { namespace cdd_interface {

enum class representation {
   V,  // rays and lineality
   H   // inequalities and equations
};

class CddInstance {
   class Initializer {
      friend class CddInstance;
      Initializer();
      ~Initializer();

      static void (* const global_construct)();
      static void (* const global_destroy)();
   };
protected:
   CddInstance()
   {
      static Initializer init{};
   }
};

template <typename Scalar>
class ConvexHullSolver : public CddInstance, public polytope::ConvexHullSolver<Scalar, CanEliminateRedundancies::yes> {
public:
   explicit ConvexHullSolver(bool verbose_ = false)
      : verbose(verbose_) {}

   convex_hull_result<Scalar>
   enumerate_facets(const Matrix<Scalar>& Points, const Matrix<Scalar>& Lineality, const bool isCone) const override;

   convex_hull_result<Scalar>
   enumerate_vertices(const Matrix<Scalar>& Inequalities, const Matrix<Scalar>& Equations, const bool isCone) const override;

   std::pair<Bitset, Set<int>> get_non_redundant_points(const Matrix<Scalar>& points, const Matrix<Scalar>& linealities, bool isCone) const override;

   std::pair<Bitset, Set<int>> get_non_redundant_inequalities(const Matrix<Scalar>& inequalities, const Matrix<Scalar>& equations, bool isCone) const override;

   /// first: indices of vertices, second: certificates (co-vertices)
   using non_redundant = std::pair<Bitset, ListMatrix< Vector<Scalar>>>;

   non_redundant
   find_vertices_among_points(const Matrix<Scalar>& Points);

   Bitset
   canonicalize_lineality(const Matrix<Scalar>& Points, const Matrix<Scalar>& InputLineality, const representation source_rep);
private:
   const bool verbose;
};

template <typename Scalar>
class LP_Solver : public CddInstance, public polytope::LP_Solver<Scalar> {
public:
   LP_Solution<Scalar>
   solve(const Matrix<Scalar>& Inequalities, const Matrix<Scalar>& Equations,
         const Vector<Scalar>& Objective, bool maximize, bool feasibility_known = false) const override;

   bool needs_feasibility_known() const override { return true; }
};

} } }

#endif // POLYMAKE_POLYTOPE_CDD_INTERFACE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
