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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"


namespace polymake { namespace polytope {

/*
 * affine projection of the points to the orthogonal
 * complement of the lineality space
 */
template <typename E>
void orthogonalize(Matrix<E>& Points, Matrix<E>& LS)
{
    // needed since project_to_orthogonal_complement wants a orthogonal basis.
    orthogonalize(entire(rows(LS)));
    Set<Int> noRays;
    for (Int i = 0; i < Points.rows(); ++i) {
       if (Points(i,0) != 0)
          noRays += i;
    }

    // moving one point to the origin (watch out for rays)
    Vector<E> a = (0|Points.row(*noRays.begin()).slice(range_from(1)));
    Matrix<E> m = repeat_row(a, noRays.size());
    Points.minor(noRays,All) -= m;
    
    project_to_orthogonal_complement(Points,LS);

    Points.minor(noRays,All) += m;
}

/*
 *  computes the pointed part of a polyhedron
 */
template <typename Scalar>
BigObject pointed_part(BigObject p_in)
{
  Matrix<Scalar> Points, Inequalities, LinealitySpace;

  BigObjectType t = p_in.type();
  BigObject p_out(t);

  // Check if VERTICES are given, because it forms the pointed part
  if (p_in.lookup("VERTICES") >> Points)
  {
    p_in.give("LINEALITY_SPACE") >> LinealitySpace;

    orthogonalize(Points, LinealitySpace);

    p_out.take("VERTICES") << Points;
    const Matrix<Scalar> empty(0, Points.cols());
    p_out.take("LINEALITY_SPACE") << empty;

    // Copying the labels  
    Array<std::string> labels;
    if (p_in.lookup("VERTEX_LABELS") >> labels)
      p_out.take("VERTEX_LABELS") << labels;
  }
  // Check if POINTS are given, because the POINTS without those,
  // which lie in LINEALITY_SPACE are the pointed part.
  else if (p_in.lookup("POINTS") >> Points)
  {    
    p_in.give("LINEALITY_SPACE") >> LinealitySpace;
     
    if(LinealitySpace.rows() != 0){
       Matrix<Scalar> M = Points * T(null_space(LinealitySpace));
       Set<Int> PointsInLinSpace;
       Vector<Scalar> zeros = zero_vector<Scalar>(M.cols());
       
       for (Int i=0; i < M.rows(); ++i) {
          if (M.row(i) == zeros) PointsInLinSpace += i;
       }
       
       Points = Points.minor(~PointsInLinSpace,All);
    }
    
    orthogonalize(Points, LinealitySpace);

    p_out.take("POINTS") << Points;
  }
  // If neither VERTICES nor POINTS are defined, we check for FACETS and INEQUALITIES.
  // the pointed part consists of all the inequalities and equations fromt the old polyhedron together
  // with the null space from the lineality space. So the lineality space are the normal vectors,
  // which define the transversal space
  else if(p_in.lookup("FACETS | INEQUALITIES") >> Inequalities)
  {
    Matrix<Scalar> Equations = p_in.give("AFFINE_HULL");

    p_in.give("LINEALITY_SPACE") >> LinealitySpace;

    p_out.take("INEQUALITIES") << Inequalities;
    p_out.take("EQUATIONS") << (Equations / LinealitySpace);

    p_out.take("FEASIBLE") << p_in.give("FEASIBLE");
  }

  return p_out;	
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Produces the pointed part of a polyhedron"
                          "# @param Polytope P"
                          "# @return Polytope"
                          "# @example"
                          "# > $p = new Polytope(POINTS=>[[1,0,0],[1,0,1],[1,1,0],[1,1,1],[0,1,0],[0,0,1]]);"
                          "# > $pp = pointed_part($p);"
                          "# > print $pp->VERTICES;"
                          "# | 1 0 0"
                          "# | 0 1 0"
                          "# | 0 0 1",
                          "pointed_part<Scalar>(Polytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
