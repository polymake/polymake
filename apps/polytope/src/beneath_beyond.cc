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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/polytope/beneath_beyond.h"
#include "polymake/polytope/beneath_beyond_impl.h"

namespace polymake { namespace polytope {

template <typename Scalar>
BeneathBeyondConvexHullSolver<Scalar>::~BeneathBeyondConvexHullSolver() {}

template <typename Scalar>
convex_hull_result<Scalar>
BeneathBeyondConvexHullSolver<Scalar>::enumerate_facets(const Matrix<Scalar>& Points, const Matrix<Scalar>& Linealities, const bool isCone) const
{
   beneath_beyond_algo<Scalar> algo;
   algo.expecting_redundant(true).for_cone(isCone).making_triangulation(false);
   algo.compute(Points, Linealities);
   return { algo.getFacets(), algo.getAffineHull() };
}

template <typename Scalar>
convex_hull_result<Scalar>
BeneathBeyondConvexHullSolver<Scalar>::enumerate_vertices(const Matrix<Scalar>& Inequalities, const Matrix<Scalar>& Equations, const bool isCone) const
{
   beneath_beyond_algo<Scalar> algo;
   algo.expecting_redundant(true).for_cone(isCone).making_triangulation(false).computing_vertices(true);
   algo.compute(Inequalities, Equations);
   convex_hull_result<Scalar> result{ algo.getFacets(), algo.getAffineHull() };
   if (!isCone && result.first.rows() == 0 && result.second.rows() == 0 && (Inequalities.rows() != 0 || Equations.rows() != 0))
      throw infeasible();
   return result;
}

template <typename Scalar>
std::pair<Bitset, Set<int>>
BeneathBeyondConvexHullSolver<Scalar>::get_non_redundant_points(const Matrix<Scalar>& Points, const Matrix<Scalar>& Linealities, bool isCone) const
{
   beneath_beyond_algo<Scalar> algo;
   algo.expecting_redundant(true).for_cone(isCone).making_triangulation(false);
   algo.compute(Points, Linealities);
   return { algo.getNonRedundantPoints(), algo.getNonRedundantLinealities() };
}

template <typename Scalar>
std::pair<Bitset, Set<int>>
BeneathBeyondConvexHullSolver<Scalar>::get_non_redundant_inequalities(const Matrix<Scalar>& Inequalities, const Matrix<Scalar>& Equations, bool isCone) const
{
   beneath_beyond_algo<Scalar> algo;
   algo.expecting_redundant(true).for_cone(isCone).making_triangulation(false).computing_vertices(true);
   algo.compute(Inequalities, Equations);
   return { algo.getNonRedundantPoints(), algo.getNonRedundantLinealities() };
}

template <typename Scalar>
Array<Set<int>>
BeneathBeyondConvexHullSolver<Scalar>::placing_triangulation(const Matrix<Scalar>& Points, const Matrix<Scalar>& Lineality) const
{
   beneath_beyond_algo<Scalar> algo;
   algo.expecting_redundant(true).for_cone(true).making_triangulation(true);
   algo.compute(Points, Lineality);
   return algo.getTriangulation();
}

// this instance is explicitly used in sympol and elsewhere; other types are instantiated on demand
template class BeneathBeyondConvexHullSolver<Rational>;

template <typename Scalar>
auto create_beneath_beyond_solver(CanEliminateRedundancies needsEliminate = CanEliminateRedundancies::yes)
{
   perl::ListReturn ret;
   if (needsEliminate == CanEliminateRedundancies::yes)
      ret << cached_convex_hull_solver<Scalar, CanEliminateRedundancies::yes>(new BeneathBeyondConvexHullSolver<Scalar>(), true);
   else
      ret << cached_convex_hull_solver<Scalar, CanEliminateRedundancies::no>(new BeneathBeyondConvexHullSolver<Scalar>(), true);
   return ret;
}

template <typename Scalar>
void beneath_beyond_find_facets(perl::Object p, const bool isCone, perl::OptionSet options)
{
   const bool non_redundant = options["non_redundant"];
   const Matrix<Scalar> Points = p.give(non_redundant ? Str("RAYS") : Str("INPUT_RAYS"));
   const Matrix<Scalar> Lins = p.lookup(non_redundant ? Str("LINEALITY_SPACE") : Str("INPUT_LINEALITY"));

   beneath_beyond_algo<Scalar> algo;
   algo.expecting_redundant(!non_redundant).for_cone(isCone);
   algo.compute(Points, Lins);

   p.take("FACETS") << algo.getFacets();
   p.take("LINEAR_SPAN") << algo.getAffineHull();
   p.take("RAYS_IN_FACETS") << algo.getVertexFacetIncidence();
   p.take("DUAL_GRAPH.ADJACENCY") << algo.getDualGraph();

   if (non_redundant) {
      p.take("ESSENTIALLY_GENERIC") << algo.getGenericPosition();
      perl::Object t("topaz::GeometricSimplicialComplex", mlist<Scalar>());
      t.take("FACETS") << algo.getTriangulation();
      p.take("TRIANGULATION") << t;
   } else {
      p.take("RAYS") << algo.getVertices();
      p.take("LINEALITY_SPACE") << algo.getLinealities();
      p.take("TRIANGULATION_INT") << algo.getTriangulation();
   }
}

template <typename Scalar>
void beneath_beyond_find_vertices(perl::Object p, const bool isCone, perl::OptionSet options)
{
   const bool non_redundant = options["non_redundant"];
   const Matrix<Scalar> Points = p.give(non_redundant ? Str("FACETS") : Str("INEQUALITIES"));
   const Matrix<Scalar> Lins = p.lookup(non_redundant ? Str("LINEAR_SPAN") : Str("EQUATIONS"));

   beneath_beyond_algo<Scalar> algo;
   algo.expecting_redundant(!non_redundant).making_triangulation(false).for_cone(isCone).computing_vertices(true);
   algo.compute(Points, Lins);

   p.take("RAYS") << algo.getFacets();
   p.take("LINEALITY_SPACE") << algo.getAffineHull();
   p.take("RAYS_IN_FACETS") << T(algo.getVertexFacetIncidence());
   p.take("GRAPH.ADJACENCY") << algo.getDualGraph();

   if (!non_redundant) {
      p.take("FACETS") << algo.getVertices();
      p.take("LINEAR_SPAN") << algo.getLinealities();
   }
}

template <typename Scalar>
Array< Set<int> >
placing_triangulation(const Matrix<Scalar>& Points, perl::OptionSet options)
{
   const bool non_redundant = options["non_redundant"];
   beneath_beyond_algo<Scalar> algo;
   algo.expecting_redundant(!non_redundant).for_cone(true).making_triangulation(true);
   Array<int> permutation;
   if (options["permutation"] >> permutation) {
      if (permutation.size() != Points.rows())
         throw std::runtime_error("placing_triangulation: wrong permutation");
      algo.compute(Points, Matrix<Scalar>(), entire(permutation));
   } else {
      algo.compute(Points, Matrix<Scalar>());
   }
   return algo.getTriangulation();
}

template <typename Scalar>
Array< Set<int> >
placing_triangulation(const SparseMatrix<Scalar>& Points, perl::OptionSet options)
{
   const Matrix<Scalar> full_points(Points);
   return placing_triangulation(full_points, options);
}

FunctionTemplate4perl("beneath_beyond_find_facets<Scalar> (Cone<Scalar>; $=true, { non_redundant => false })");

FunctionTemplate4perl("beneath_beyond_find_facets<Scalar> (Polytope<Scalar>; $=false, { non_redundant => false })");

FunctionTemplate4perl("beneath_beyond_find_vertices<Scalar> (Cone<Scalar>; $=true, { non_redundant => false })");

FunctionTemplate4perl("beneath_beyond_find_vertices<Scalar> (Polytope<Scalar>; $=false, { non_redundant => false })");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Compute the placing triangulation of the given point set using the beneath-beyond algorithm."
                          "# @param Matrix Points the given point set"
                          "# @option Bool non_redundant whether it's already known that //Points// are non-redundant"
                          "# @option Array<Int> permutation placing order of //Points//, must be a valid permutation of (0..Points.rows()-1)"
                          "# @return Array<Set<Int>>"
                          "# @example To compute the placing triangulation of the square (of whose vertices we know that"
                          "# they're non-redundant), do this:"
                          "# > $t = placing_triangulation(cube(2)->VERTICES, non_redundant=>1);"
                          "# > print $t;"
                          "# | {0 1 2}"
                          "# | {1 2 3}",
                          "placing_triangulation(Matrix; { non_redundant => false, permutation => undef })");

InsertEmbeddedRule("function beneath_beyond.convex_hull: create_convex_hull_solver<Scalar> [is_ordered_field_with_unlimited_precision(Scalar)] (;$=0)"
                   " : c++ (name => 'create_beneath_beyond_solver') : returns(cached);\n");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
