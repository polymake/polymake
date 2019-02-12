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
#include "polymake/Array.h"
#include "polymake/Rational.h"
#include <fstream>
#include "polymake/common/print_constraints.h"

namespace polymake { namespace common {

template <typename Scalar>
void print_constraints(const Matrix<Scalar>& M, perl::OptionSet options)
{
   print_constraints_sub(M, options["coord_labels"],options["row_labels"],options["equations"],options["homogeneous"]);
}

UserFunctionTemplate4perl("# @category Formatting"
                          "# Write the rows of a matrix //M// as inequalities (//equations=0//)"
                          "# or equations (//equations=1//) in a readable way."
                          "# It is possible to specify labels for the coordinates via"
                          "# an optional array //coord_labels//."
                          "# @param Matrix<Scalar> M the matrix whose rows are to be written"
                          "# @option Array<String> coord_labels changes the labels of the coordinates"
                          "# @option Array<String> row_labels changes the labels of the rows"
                          "# @option Bool homogeneous false if the first coordinate should be interpreted as right hand side"
                          "# @option Bool equations true if the rows represent equations instead of inequalities"
                          "# @example"
                          "# > $M = new Matrix([1,2,3],[4,5,23]);"
                          "# > print_constraints($M,equations=>1);"
                          "# | 0: 2 x1 + 3 x2 = -1"
                          "# | 1: 5 x1 + 23 x2 = -4",
                          "print_constraints<Scalar>(Matrix<Scalar>; { equations => 0, homogeneous => 0, coord_labels => undef, row_labels => undef })");
} }
