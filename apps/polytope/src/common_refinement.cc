/* Copyright (c) 1997-2020
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
#include "polymake/list"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {

template <typename Scalar>
IncidenceMatrix<> common_refinement(const Matrix<Scalar>& vertices, const IncidenceMatrix<>& sub1, const IncidenceMatrix<>& sub2, const Int dim)
{
   BigObjectType polytope("Polytope", mlist<Scalar>());

   RestrictedIncidenceMatrix<> refinement;
   for (auto i=entire(rows(sub1)); !i.at_end(); ++i)
      for (auto j=entire(rows(sub2)); !j.at_end(); ++j) {
         const Set<Int> intersection = (*i)*(*j);
         if (intersection.size() > dim) {
            BigObject p(polytope, "VERTICES", vertices.minor(intersection, All));
            const Int int_dim = p.call_method("DIM");
            if (int_dim == dim) {
               refinement /= intersection;
            }
         }
      }

   return IncidenceMatrix<>(std::move(refinement));
}

template <typename Scalar>
BigObject common_refinement(BigObject p1, BigObject p2)
{
   const Int dim = p1.call_method("DIM");
   const Matrix<Scalar> vert=p1.give("VERTICES");
   const IncidenceMatrix<> sub1=p1.give("POLYTOPAL_SUBDIVISION.MAXIMAL_CELLS");
   const IncidenceMatrix<> sub2=p2.give("POLYTOPAL_SUBDIVISION.MAXIMAL_CELLS");

   BigObject p_out(p1.type()); //FIXME: p_out should become a copy of p1 if there exists a copy method
   //  p_out.remove("WEIGHTS"); FIXME (If there is a remove method).
   if (p1.exists("POLYTOPAL_SUBDIVISION.WEIGHTS") && p2.exists("POLYTOPAL_SUBDIVISION.WEIGHTS")) {
      const Vector<Scalar> w1=p1.give("POLYTOPAL_SUBDIVISION.WEIGHTS");
      const Vector<Scalar> w2=p2.give("POLYTOPAL_SUBDIVISION.WEIGHTS");
      p_out.take("POLYTOPAL_SUBDIVISION.WEIGHTS") << (w1+w2);
   }
   p_out.take("FEASIBLE") << true;
   p_out.take("VERTICES") << vert; //FIXME
   p_out.take("POLYTOPAL_SUBDIVISION.MAXIMAL_CELLS") << common_refinement(vert, sub1, sub2, dim);
   return p_out;
}

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Computes the common refinement of two subdivisions of //points//."
                          "# It is assumed that there exists a common refinement of the two subdivisions."
                          "# @param Matrix points"
                          "# @param IncidenceMatrix sub1 first subdivision"
                          "# @param IncidenceMatrix sub2 second subdivision"
                          "# @param Int dim dimension of the point configuration"
                          "# @return IncidenceMatrix the common refinement"
                          "# @example A simple 2-dimensional set of points:"
                          "# > $points = new Matrix<Rational>([[1,0,0],[1,1,0],[1,0,1],[1,1,1],[1,2,1]]);"
                          "# Two different subdivisions..."
                          "# > $sub1 = new IncidenceMatrix([[0,1,2],[1,2,3,4]]);"
                          "# > $sub2 = new IncidenceMatrix([[1,3,4],[0,1,2,3]]);"
                          "# ...and their common refinement:"
                          "# > print common_refinement($points,$sub1,$sub2,2);"
                          "# | {0 1 2}"
                          "# | {1 3 4}"
                          "# | {1 2 3}"
                          "# @author Sven Herrmann",
                          "common_refinement(Matrix IncidenceMatrix IncidenceMatrix $)");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Computes the common refinement of two subdivisions of the same polytope //p1//, //p2//."
                          "# It is assumed that there exists a common refinement of the two subdivisions."
                          "# It is not checked if //p1// and //p2// are indeed the same!"
                          "# @param Polytope p1"
                          "# @param Polytope p2"
                          "# @return Polytope"
                          "# @author Sven Herrmann",
                          "common_refinement<Scalar>(Polytope<Scalar> Polytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
