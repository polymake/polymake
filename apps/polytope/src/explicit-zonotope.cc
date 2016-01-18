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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

template<typename E>
perl::Object explicit_zonotope(const Matrix<E>& zones, perl::OptionSet options)
{
   const bool rows_are_points = options["rows_are_points"];
   const int dim = rows_are_points ? zones.cols()-1 : zones.cols();
   Matrix<E> points(1, dim+1);    // one zero-filled row

   for (typename Entire< Rows< Matrix<E> > >::const_iterator z=entire(rows(zones)); !z.at_end(); ++z) {
      Vector<E> hom_row (*z);
      if (!rows_are_points) hom_row = (E(1) | hom_row);
      points =
         ( points + repeat_row(hom_row, points.rows()) ) /
         ( points - repeat_row(hom_row, points.rows()) );
   }
   points.col(0).fill(1); // fix the homogenizing column that has been messed up by the sums

   perl::Object Z("Polytope");
   Z.take("ZONOTOPE_INPUT_POINTS") << zones;
   Z.take("POINTS") << points;

   return Z;
}

UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                  "# Produce the POINTS of a zonotope as the iterated Minkowski sum of all intervals [-x,x],"
                  "# where x ranges over the rows of the input matrix //zones//."
                  "# "
                  "# @param Matrix zones the input vectors"
                  "# @option Bool rows_are_points the rows of the input matrix represent affine points(true, default) or linear vectors(false)"
                  "# @return Polytope"
                  "# @example > $M = new Matrix([1,1],[1,-1]);"
                  "# > $p = explicit_zonotope($M,rows_are_points=>0);"
                  "# > print $p->VERTICES;"
                  "# | 1 2 0"
                  "# | 1 0 -2"
                  "# | 1 0 2"
                  "# | 1 -2 0",
                  "explicit_zonotope<E>(Matrix<E> { rows_are_points => 1 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
