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
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename E, typename Matrix1, typename Matrix2> inline
IncidenceMatrix<>
incidence_matrix(const GenericMatrix<Matrix1,E>& R, const GenericMatrix<Matrix2,E>& C)
{
   return IncidenceMatrix<> (R.rows(), C.rows(),
                             attach_operation(product(rows(R), rows(C), operations::mul()), operations::is_zero()).begin());
}

template <typename Matrix1, typename Matrix2> inline
IncidenceMatrix<>
incidence_matrix(const GenericMatrix<Matrix1,double>& R, const GenericMatrix<Matrix2,double>& C)
{
   return incidence_matrix<double, Matrix1, Matrix2>(normalized(R), normalized(C));
}

FunctionTemplate4perl("incidence_matrix(Matrix,Matrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
