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

#ifndef POLYMAKE_TOPAZ_BARYCENTRIC_SUBDIVISION_H
#define POLYMAKE_TOPAZ_BARYCENTRIC_SUBDIVISION_H

#include "polymake/graph/HasseDiagram.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"

#include <string>

namespace polymake { namespace topaz {

// Computes the barycentric subdivision
Array< Set<int> > bs(const graph::HasseDiagram& HD);

// Computes the VERTEX_LABELS
Array<std::string> bs_labels(const graph::HasseDiagram& HD);

// Computes the GEOMETRIC_REALIZATION
Matrix<Rational> bs_geom_real(const Matrix<Rational>& old_coord,
                              const graph::HasseDiagram& HD);

} }

#endif // POLYMAKE_TOPAZ_BARYCENTRIC_SUBDIVISION_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
