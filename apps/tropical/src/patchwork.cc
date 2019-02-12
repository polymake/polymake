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
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/tropical/patchwork.h"

/* #include "polymake/ListMatrix.h" */
/* #include "polymake/Graph.h" */
/* #include "polymake/common/incidence_tools.h" */
/* #include "polymake/polytope/compress_incidence.h" */
/* #include "polymake/tropical/arithmetic.h" */
/* #include "polymake/tropical/thomog.h" */

namespace polymake { namespace tropical {

FunctionTemplate4perl("real_facets<Addition>(Vector<Bool>, Matrix<Int>, Vector<TropicalNumber<Addition>>, Matrix<Rational>, IncidenceMatrix<NonSymmetric>)");

FunctionTemplate4perl("real_part_realize<Addition>(Matrix<Int>, Vector<TropicalNumber<Addition>>, Matrix<Rational>, IncidenceMatrix<NonSymmetric>, Set<Int>, IncidenceMatrix<NonSymmetric>, String)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:2
// indent-tabs-mode:nil
// End:
