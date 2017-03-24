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
#include "polymake/SparseMatrix.h"
#include "polymake/polytope/transform.h"

namespace polymake { namespace polytope {

template <typename Scalar>
perl::Object bound(perl::Object p_in)
{
   const bool is_positive=p_in.give("POSITIVE");
   if (!is_positive)
      throw std::runtime_error("polyhedron must be positive");

   const int d=p_in.call_method("AMBIENT_DIM");

   SparseMatrix<Scalar> tau=unit_matrix<Scalar>(d+1);
   tau.col(0).fill(1);

   perl::Object p_out=transform<Scalar>(p_in, tau);
   p_out.set_description() << "Bounded polytope transformed from " << p_in.name() << endl;

   p_out.take("BOUNDED") << true;
   return p_out;
}

UserFunctionTemplate4perl("# @category Transformations"
                          "# Make a positive polyhedron bounded."
                          "# Apply a projective linear transformation to a polyhedron mapping the far hyperplane"
                          "# to the hyperplane spanned by the unit vectors."
                          "# The origin (1,0,...,0) is fixed."
                          "# "
                          "# The input polyhedron should be [[POSITIVE]]; i.e. no negative coordinates."
                          "# @param Polytope P a positive polyhedron"
                          "# @return Polytope"
                          "# @example Observe the transformation of a simple unbounded 2-polyhedron:"
                          "# > $P = new Polytope(VERTICES=>[[1,0,0],[0,1,1],[0,0,1]]);"
                          "# > print bound($P)->VERTICES;"
                          "# | 1 0 0"
                          "# | 1 1/2 1/2"
                          "# | 1 0 1"
                          "# As you can see, the far points are mapped to the hyperplane spanned by (1,1,0) and (1,0,1).",
                          "bound<Scalar> (Polytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
