/* Copyright (c) 1997-2015
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
                                (take_VERTICES ? "FACETS" : "INEQUALITIES") :                                      
                                (take_VERTICES ? "RAYS" : "INPUT_RAYS"));
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
placing_triangulation(const Matrix<Scalar>& Points, const Array<int>& permutation)
{
   beneath_beyond_algo<Scalar> algo(Points, false);
   if (permutation.empty()) {
      algo.compute(entire(sequence(0,Points.rows())));
   } else {
      if (permutation.size() != Points.rows())
         throw std::runtime_error("placing_triangulation: wrong permutation");
      algo.compute(entire(permutation));
   }
   return algo.getTriangulation();
}

template <typename Scalar>
Array< Set<int> >
placing_triangulation(const SparseMatrix<Scalar>& Points, const Array<int>& permutation)
{
   const Matrix<Scalar> full_points(Points);
   return placing_triangulation(full_points, permutation);
}

FunctionTemplate4perl("beneath_beyond<Scalar> (Cone<Scalar>; $=1, $=0) : void");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Compute the placing triangulation of the given point set using the beneath-beyond algorithm."
                          "# @param Matrix Points the given point set"
                          "# @param Array<Int> permutation"
                          "# @return Array<Set<Int>>",
                          "placing_triangulation(Matrix; $=[ ])");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
