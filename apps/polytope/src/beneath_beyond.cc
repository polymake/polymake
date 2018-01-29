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
#include "polymake/polytope/beneath_beyond.h"

namespace polymake { namespace polytope {

template <typename Scalar>
void beneath_beyond(perl::Object p, bool take_VERTICES, bool dual)
{
   Matrix<Scalar> Points=p.give(dual ?
                                (take_VERTICES ? Str("FACETS") : Str("INEQUALITIES")) :
                                (take_VERTICES ? Str("RAYS") : Str("INPUT_RAYS")));
   beneath_beyond_algo<Scalar> algo(Points, take_VERTICES);
   algo.compute(entire(sequence(0,Points.rows())));

   if (!dual) {
      p.take("FACETS") << algo.getFacets();
      p.take("LINEAR_SPAN") << algo.getAffineHull();
      p.take("RAYS_IN_FACETS") << algo.getVertexFacetIncidence();
      p.take("DUAL_GRAPH.ADJACENCY") << algo.getDualGraph();

      if (take_VERTICES) {
         perl::Object t(perl::ObjectType::construct<Scalar>("topaz::GeometricSimplicialComplex"));
         t.take("FACETS") << algo.getTriangulation();
         p.take("TRIANGULATION") << t;
         p.take("ESSENTIALLY_GENERIC") << algo.getGenericPosition();
         // do not write TRIANGULATION.COORDINATES here as it is redundant;
         // better use temporary copy of INUT_RAYS/RAYS whenever required
      } else {
         p.take("RAYS") << algo.getVertices();
         // by assumption any polytope treated by beneath_beyond is pointed
         p.take("LINEALITY_SPACE") << Matrix<Scalar>();
         p.take("TRIANGULATION_INT") << algo.getTriangulation();
      }
   } else {
      p.take("RAYS") << algo.getFacets();
      p.take("LINEALITY_SPACE") << algo.getAffineHull();
      p.take("RAYS_IN_FACETS") << T(algo.getVertexFacetIncidence());
      p.take("GRAPH.ADJACENCY") << algo.getDualGraph();

      if (!take_VERTICES) {
         p.take("FACETS") << algo.getVertices();
         // by assumption any dual polytope treated by beneath_beyond is full dimensional
         p.take("LINEAR_SPAN") << Matrix<Scalar>();
      }
   }
}

template <typename Scalar>
Array< Set<int> >
placing_triangulation(const Matrix<Scalar>& Points, perl::OptionSet options)
{
   const bool already_VERTICES = options["non_redundant"];
   beneath_beyond_algo<Scalar> algo(Points, already_VERTICES);
   Array<int> permutation;
   if (options["permutation"] >> permutation) {
      if (permutation.size() != Points.rows())
         throw std::runtime_error("placing_triangulation: wrong permutation");
      algo.compute(entire(permutation));
   } else {
      algo.compute(entire(sequence(0,Points.rows())));
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

FunctionTemplate4perl("beneath_beyond<Scalar> (Cone<Scalar>; $=1, $=0) : void");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Compute the placing triangulation of the given point set using the beneath-beyond algorithm."
                          "# @param Matrix Points the given point set"
                          "# @option Bool non_redundant whether it's already known that //Points// are non-redundant"
                          "# @option Array<Int> permutation placing order of //Points//, must be a valid permutation of (0..Points.rows()-1)"
                          "# @return Array<Set<Int>>"
                          "# @example To compute the placing triangulation of the square (of whose vertices we know that"
                          "# they're non-redundant), do this:"
                          "# > $t = placing_triangulation(cube(2)->VERTICES,non_redundant=>1);"
                          "# > print $t;"
                          "# | {0 1 2}"
                          "# | {1 2 3}",
                          "placing_triangulation(Matrix, { non_redundant => 0, permutation => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
