/* Copyright (c) 1997-2014
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
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"


namespace polymake { namespace polytope {

template <typename Matrix2, typename E>
void orthogonalize_facets(Matrix<E>& F, const GenericMatrix<Matrix2,E>& AH)
{
   for (typename Entire< Rows<Matrix2> >::const_iterator a=entire(rows(AH)); !a.at_end(); ++a) {
      const E s=sqr(a->slice(1));
      for (typename Entire< Rows< Matrix<E> > >::iterator f=entire(rows(F)); !f.at_end(); ++f) {
         const E x= f->slice(1) * a->slice(1);
         if (!is_zero(x)) *f -= (x/s) * (*a);
      }
   }
}

/*
 *  computes the pointed part of a polyhedron
 */
template <typename Scalar>
perl::Object pointed_part(perl::Object p_in)
{
  Matrix<Scalar> Points, Inequalities, LinealitySpace;

  perl::ObjectType t=p_in.type();
  perl::Object p_out(t);

  // Check if VERTICES are given, because it forms the pointed part
  if (p_in.lookup("VERTICES") >> Points)
  {
    p_in.give("LINEALITY_SPACE") >> LinealitySpace;

    Set<int> b=scalar2set(0);

    Vector<Scalar> a = (0|Points.row(0).slice(1));
    Matrix<Scalar> m = repeat_row(a, Points.rows());

    Points -= m;

    orthogonalize_facets(Points, LinealitySpace);

    Points += m;

    p_out.take("VERTICES") << Points;
    const Matrix<Scalar> empty;
    p_out.take("LINEALITY_SPACE") << empty;

    // Copying the labels  
    Array<std::string> labels;
    if (p_in.lookup("VERTEX_LABELS") >> labels)
      p_out.take("VERTEX_LABELS") << labels;
  }
  // Check if POINTS are given, because the POINTS without those,
  // which lie in LINEALITY_SPACE are the pointed part.
  else if(p_in.lookup("POINTS") >> Points)
  {    
    Matrix<Scalar> M = Points * T(null_space(LinealitySpace));
    Set<int> PointsInLinSpace;
    Vector<Scalar> zeros = zero_vector<Scalar>(M.cols());

    p_in.give("LINEALITY_SPACE") >> LinealitySpace;

    for (int i=0; i < M.rows(); ++i)
    {
      if (M.row(i) == zeros) PointsInLinSpace += i;
    }

    Vector<Scalar> a = (0|Points.row(0).slice(1));
    Matrix<Scalar> m = repeat_row(a, Points.rows());

    Points -= m;

    orthogonalize_facets(Points, LinealitySpace);

    Points += m;

    p_out.take("POINTS") << Points.minor(~PointsInLinSpace,All);
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
  }

  return p_out;	
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Produces the pointed part of a polyhedron"
                          "# @param Polytope P"
                          "# @return Polytope", 
                          "pointed_part<Scalar>(Polytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
