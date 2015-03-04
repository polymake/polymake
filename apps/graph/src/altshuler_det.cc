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
#include "polymake/IncidenceMatrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace graph {

Integer altshuler_det(const IncidenceMatrix<>& Inc)
{
   return Inc.rows() <= Inc.cols()
          ? det( same_element_sparse_matrix<Integer>(Inc) * T(same_element_sparse_matrix<Integer>(Inc)) )
          : det( T(same_element_sparse_matrix<Integer>(Inc)) * same_element_sparse_matrix<Integer>(Inc) );
}

Function4perl(&altshuler_det, "altshuler_det");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
