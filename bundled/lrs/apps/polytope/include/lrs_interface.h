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

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Bitset.h"
#include "polymake/polytope/solve_LP.h"
#include "polymake/polytope/convex_hull.h"

namespace polymake { namespace polytope { namespace lrs_interface {

class LrsInstance {
   class Initializer {
      friend class LrsInstance;
      Initializer();
      ~Initializer();

      static void (* const global_construct)();
      static void (* const global_destroy)();
   };
protected:
   LrsInstance()
   {
      static Initializer init{};
   }
};

class ConvexHullSolver : public LrsInstance, public polytope::ConvexHullSolver<Rational> {
public:
   explicit ConvexHullSolver(bool verbose_ = false)
      : verbose(verbose_) {}

   convex_hull_result<Rational>
   enumerate_facets(const Matrix<Rational>& Points, const Matrix<Rational>& Lineality, bool isCone) const override; 

   long
   count_facets(const Matrix<Rational>& Points,const Matrix<Rational>& Lineality, bool isCone) const;

   convex_hull_result<Rational>
   enumerate_vertices(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, bool isCone) const override;

   struct vertex_count {
      long n_vertices;
      long n_bounded_vertices;
      long lineality_dim;
   };

   vertex_count
   count_vertices(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, bool only_bounded) const;

   using non_redundant = std::pair<Bitset, Matrix<Rational>>;

   /// @retval first: indices of vertices, second: affine hull (primal)
   /// @retval first: indices of facets, second: lineality space (dual)
   // function only works for pointed objects: lrs does not remove generators in the lineality space from input
   non_redundant find_irredundant_representation(const Matrix<Rational>& Points, const Matrix<Rational>& Lineality, const bool dual) const;

private:
   const bool verbose;
};

class LP_Solver : public LrsInstance, public polytope::LP_Solver<Rational> {
public:
   LP_Solution<Rational>
   solve(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations,
         const Vector<Rational>& Objective, bool maximize, bool=false) const override;

   bool check_feasibility(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations) const;
   bool check_feasibility(const Matrix<Rational>& Inequalities, const Matrix<Rational>& Equations, Vector<Rational>& ValidPoint) const;
};

struct dictionary;

} } }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
