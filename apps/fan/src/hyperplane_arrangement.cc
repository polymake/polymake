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
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"
#include "polymake/Rational.h"

namespace polymake { namespace fan {

typedef Rational Coord;

perl::Object hyperplane_arrangement(const Matrix<Coord>& hyperplanes)
{
  perl::Object f("PolyhedralFan", mlist<Coord>());
  const int n=hyperplanes.rows();
  const int d=hyperplanes.cols();
  ListMatrix<Vector<Coord>> rays(0,d);
  const Integer n_subsets=Integer::binom(n,d-1);

  // test for each d-1 hyperplanes if their intersection is 1-dimensional
  for (auto i=entire(all_subsets_of_k(sequence(0,n),d-1)); !i.at_end(); ++i) {
    const Matrix<Coord> h=hyperplanes.minor(*i,All);

    // they must have full rank
    if (rank(h)==d-1) {
      bool is_new=true;

      // exclude rays we already had
      for (auto j=entire(rows(rays)); is_new && !j.at_end(); ++j)
         if (is_zero(h*(*j))) is_new=false;

      if (is_new) {
          Vector<Rational> a=null_space(h).row(0);
          // normalizes the ray, i.e. divides by the first non-zero entry
          Coord fac;
          for (int k=0; is_zero(fac=a[k]); ++k);
          a/=fac;
          rays/=a;
      }
    }
  }

  f.take("RAYS") << rays/-rays;
  f.take("FACET_NORMALS") << hyperplanes;
  f.take("LINEALITY_SPACE") << Matrix<Coord>(0,d);
  f.set_description() << "Arrangement of the hyperplanes in FACET_NORMALS" << endl;

  return f;

}

UserFunction4perl("# @category Producing a fan"
                  "# Compute the fan given by a bunch of hyperplanes //H//."
                  "# @param Matrix H"
                  "# @return PolyhedralFan"
                  "# @author Sven Herrmann",
                  &hyperplane_arrangement, "hyperplane_arrangement");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
