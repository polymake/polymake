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
#include "polymake/permutations.h"
#include "polymake/Array.h"
#include "polymake/GenericMatrix.h"
#include "polymake/Matrix.h"

namespace polymake { namespace group {

template <typename Scalar>
Array<Array<Int>> col_to_row_action(const Matrix<Scalar>& M, const Array<Array<Int>>& G)
{
   return rows_induced_from_cols(M, G);
}

UserFunctionTemplate4perl("#@category Symmetry"
                  "# If the action of some permutations on the entries of the rows "
                  "# maps each row of a matrix to another row we obtain an induced action"
		              "# on the set of rows of the matrix."
                  "# Considering the rows as points this corresponds to the action on the"
		              "# points induced by the action of some permutations on the coordinates."
                  "# @param Matrix M"
            		  "# @param Array<Array> p the permutations acting of the rows" 
                  "# @return Array<Array> permutations resulting of the actions",
                  "col_to_row_action<Scalar>(Matrix<Scalar>,Array)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
