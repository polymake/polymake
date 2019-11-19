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

#ifndef POLYMAKE_POLYTOPE_CONVEX_HULL_H
#define POLYMAKE_POLYTOPE_CONVEX_HULL_H

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/Bitset.h"
#include "polymake/internal/linalg_exceptions.h"
#include "polymake/polytope/generic_convex_hull_client.h"

namespace polymake { namespace polytope {

// first = facets or vertices (cone: rays)
// second = affine hull (cone: linear span) or lineality space
template <typename Scalar>
using convex_hull_result = std::pair<Matrix<Scalar>, Matrix<Scalar>>;

//! check column dimensions of given matrices, extend empty one if needed
//! @param A first matrix
//! @param B second matrix
//! @param prepend_0_column prepend a column of zero elements to non-empty matrices
//! @return success indicator
template <typename E>
bool align_matrix_column_dim(Matrix<E>& A, Matrix<E>& B, bool prepend_0_column)
{
   const int d = std::max(A.cols(), B.cols());
   for (auto m : { &A, &B }) {
      if (m->cols() != d) {
         if (m->rows() != 0 || m->cols() != 0)
            return false;
         m->resize(0, d);
      }
      if (prepend_0_column && d)
         *m = zero_vector<E>() | *m;
   }
   return true;
}

//! Dehomogenize the result of a convex hull computation. If isCone is true,
//! the leading zero is eliminated, and the trivial equation/lineality is
//! filtered out.
//! @param solution convex_hull_result<Scalar>
//! @param bool isCone
template <typename Scalar>
convex_hull_result<Scalar> dehomogenize_cone_solution(const convex_hull_result<Scalar>& solution){
   auto L = solution.second.minor(All, range_from(1));
   Set<int> ind = indices(attach_selector(rows(L), operations::non_zero()));
   return convex_hull_result<Scalar>(solution.first.minor(All, range_from(1)), L.minor(ind, All));
}

//! Check whether the given points give a feasible polytope, i.e. check whether
//! there is any point with positive height.
template <typename TMatrix, typename E>
void check_points_feasibility(const GenericMatrix<TMatrix, E>& points){
   if(points.rows() == 0) {
      throw std::runtime_error("Points matrix is empty.");
   }
   for (const auto& row : rows(points)) {
      if (row[0] > 0) {
         return;
      }
   }
   throw std::runtime_error("Points matrix does not contain an entry with leading positive coordinate.");
}


// not every solver offers direct support of redundant point/inequality elimination
enum class CanEliminateRedundancies { no, yes };

template <typename Scalar, CanEliminateRedundancies can_eliminate = CanEliminateRedundancies::no>
class ConvexHullSolver {
public:
   virtual ~ConvexHullSolver() {}

   virtual convex_hull_result<Scalar> enumerate_facets(const Matrix<Scalar>& points, const Matrix<Scalar>& linealities, bool isCone) const = 0;

   virtual convex_hull_result<Scalar> enumerate_vertices(const Matrix<Scalar>& inequalities, const Matrix<Scalar>& equations, bool isCone) const = 0;
};

template <typename Scalar>
class ConvexHullSolver<Scalar, CanEliminateRedundancies::yes>
   : public ConvexHullSolver<Scalar, CanEliminateRedundancies::no> {
public:
   // first = non-redundant points (vertices) as row indexes into points
   // second = non-redundant linealities as row indexes into points (when i < points.rows()) or linealities (i - points.rows())
   virtual std::pair<Bitset, Set<int>> get_non_redundant_points(const Matrix<Scalar>& points, const Matrix<Scalar>& linealities, bool isCone) const = 0;

   // first = non-redundant inequalities (facets) as row indexes into inequalities
   // second = non-redundant equations as row indexes into inequalities (when i < inequalities.rows()) or equations (i - inequalities.rows())
   virtual std::pair<Bitset, Set<int>> get_non_redundant_inequalities(const Matrix<Scalar>& inequalities, const Matrix<Scalar>& equations, bool isCone) const = 0;
};

template <typename Scalar, CanEliminateRedundancies can_eliminate = CanEliminateRedundancies::no>
using cached_convex_hull_solver = CachedObjectPointer<ConvexHullSolver<Scalar, can_eliminate>, Scalar>;

template <typename Scalar, CanEliminateRedundancies can_eliminate = CanEliminateRedundancies::no>
const ConvexHullSolver<Scalar, can_eliminate>& get_convex_hull_solver()
{
   static cached_convex_hull_solver<Scalar, can_eliminate> solver_ptr("polytope::create_convex_hull_solver");
   return can_eliminate == CanEliminateRedundancies::yes ? solver_ptr.get(can_eliminate) : solver_ptr.get();
}

// convenience wrappers
template <typename Scalar, typename Matrix1, typename Matrix2>
convex_hull_result<Scalar> enumerate_vertices(const GenericMatrix<Matrix1, Scalar>& inequalities, const GenericMatrix<Matrix2, Scalar>& equations,
                                              bool isCone)
{
   const auto& solver = get_convex_hull_solver<Scalar>();
   return enumerate_vertices(inequalities, equations, isCone, solver);
}

template <typename Scalar, typename Matrix1, typename Matrix2, typename Solver>
convex_hull_result<Scalar> enumerate_vertices(const GenericMatrix<Matrix1, Scalar>& inequalities, const GenericMatrix<Matrix2, Scalar>& equations,
                                              bool isCone, const Solver& solver)
{
   Matrix<Scalar> H(inequalities), EQ(equations);
   if (!align_matrix_column_dim(H, EQ, isCone))
      throw std::runtime_error("convex_hull_dual - dimension mismatch between FACETS|INEQUALITIES and LINEAR_SPAN|EQUATIONS");
   return isCone ? dehomogenize_cone_solution(solver.enumerate_vertices(H, EQ, true)) : solver.enumerate_vertices(H, EQ, false);
}

template <typename Scalar, typename Matrix1, typename Matrix2>
convex_hull_result<Scalar> enumerate_facets(const GenericMatrix<Matrix1, Scalar>& points, const GenericMatrix<Matrix2, Scalar>& linealities,
                                            bool isCone)
{
   const auto& solver = get_convex_hull_solver<Scalar>();
   return enumerate_facets(points, linealities, isCone, solver);
}

template <typename Scalar, typename Matrix1, typename Matrix2, typename Solver>
convex_hull_result<Scalar> enumerate_facets(const GenericMatrix<Matrix1, Scalar>& points, const GenericMatrix<Matrix2, Scalar>& linealities,
                                            bool isCone, const Solver& solver){
   Matrix<Scalar> Points(points), Lineality(linealities);
   if(!isCone) check_points_feasibility(Points);
   if (!align_matrix_column_dim(Points, Lineality, isCone)) throw
      std::runtime_error("convex_hull_primal - dimension mismatch between RAYS|INPUT_RAYS and LINEALITY_SPACE|INPUT_LINEALITY");
   return isCone ? dehomogenize_cone_solution(solver.enumerate_facets(Points, Lineality, true)) : solver.enumerate_facets(Points, Lineality, false);
}

// convenience wrappers for empty equations/lineality
template <typename Scalar, typename Matrix1>
convex_hull_result<Scalar> enumerate_vertices(const GenericMatrix<Matrix1, Scalar>& inequalities, bool isCone)
{
   return enumerate_vertices(inequalities, Matrix<Scalar>(0, inequalities.cols()), isCone);
}

template <typename Scalar, typename Matrix1>
convex_hull_result<Scalar> enumerate_facets(const GenericMatrix<Matrix1, Scalar>& points, bool isCone)
{
   return enumerate_facets(points.top(), Matrix<Scalar>(0, points.cols()), isCone);
}

// try to compute vertices, return empty matrices in the infeasible case
template <typename Scalar, typename Matrix1, typename Matrix2>
convex_hull_result<Scalar> try_enumerate_vertices(const GenericMatrix<Matrix1, Scalar>& inequalities, const GenericMatrix<Matrix2, Scalar>& equations,
                                                  bool isCone)
{
   try {
      return enumerate_vertices(inequalities, equations, isCone);
   }
   catch (const infeasible&) {
      const int d = std::max(inequalities.cols(), equations.cols());
      return { Matrix<Scalar>(0, d), Matrix<Scalar>(0, d) };
   }
}

template <typename Scalar, typename Matrix1, typename Matrix2>
std::pair<Bitset, Set<int>> get_non_redundant_points(const GenericMatrix<Matrix1, Scalar>& points, const GenericMatrix<Matrix2, Scalar>& linealities,
                                                     bool isCone)
{
   const auto& solver = get_convex_hull_solver<Scalar, CanEliminateRedundancies::yes>();
   return solver.get_non_redundant_points(convert_to_persistent_dense(points.top()), convert_to_persistent_dense(linealities.top()), isCone);
}

template <typename Scalar, typename Matrix1, typename Matrix2>
std::pair<Bitset, Set<int>> get_non_redundant_inequalities(const GenericMatrix<Matrix1, Scalar>& inequalities, const GenericMatrix<Matrix2, Scalar>& equations,
                                                           bool isCone)
{
   const auto& solver = get_convex_hull_solver<Scalar, CanEliminateRedundancies::yes>();
   return solver.get_non_redundant_inequalities(convert_to_persistent_dense(inequalities.top()), convert_to_persistent_dense(equations.top()), isCone);
}

} }

#endif // POLYMAKE_POLYTOPE_CONVEX_HULL_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
