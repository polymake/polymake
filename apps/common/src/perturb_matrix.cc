/* Copyright (c) 1997-2023
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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/RandomPoints.h"

namespace polymake { namespace common {

Matrix<Rational>
perturb_matrix(Matrix<Rational> M, const Rational& eps, bool not_hom, OptionSet options)
{
   const Int n_cols = M.cols() - !not_hom;
   RandomSpherePoints<> random_source(n_cols, RandomSeed(options["seed"]));
   const Matrix<Rational> randomMatrix(M.rows(), n_cols, random_source.begin());

   if (not_hom) {
      M += randomMatrix * eps;
   } else {
      M.minor(All, sequence(1,n_cols)) += randomMatrix * eps;
   }
   return M;
}

UserFunction4perl("# @category Utilities"
                  "# Perturb a given matrix //M// by adding a random matrix."
                  "# The random matrix consists of vectors that are uniformly distributed"
                  "# on the unit sphere. Optionally, the random matrix can be scaled by"
                  "# a factor //eps//."
                  "# @param Matrix M"
                  "# @param Float eps the factor by which the random matrix is multiplied"
                  "#   default value: 1"
                  "# @param Bool not_hom if set to 1, the first column will also be perturbed;"
                  "#   otherwise the first columns of the input matrix //M// and the perturbed one"
                  "#   coincide (useful for working with homogenized coordinates);"
                  "#   default value: 0 (homogen. coords)"
                  "# @option Int seed controls the outcome of the random number generator;"
                  "#   fixing a seed number guarantees the same outcome."
                  "# @return Matrix",
                  &perturb_matrix, "perturb_matrix(Matrix; $=1, $=0, { seed => undef })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
