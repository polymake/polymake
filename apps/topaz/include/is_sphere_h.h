/* Copyright (c) 1997-2022
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

#pragma once

#include "polymake/IndexedSubset.h"
#include "polymake/Integer.h"
#include "polymake/topaz/BistellarComplex.h"
#include "polymake/topaz/SimplicialComplex_as_FaceMap.h"
#include "polymake/topaz/HomologyComplex.h"
#include "polymake/topaz/hasse_diagram.h"

namespace polymake { namespace topaz {

// return values: 1=true, 0=false, -1=undef

Int is_sphere_h(const Lattice<BasicDecoration>& HD, const pm::SharedRandomState& random_source, const Int strategy, const Int n_stable_rounds);

template <typename Complex>
Int is_sphere_h(const Complex& C, const pm::SharedRandomState& random_source, const Int strategy, const Int n_stable_rounds)
{
   return is_sphere_h(hasse_diagram_from_facets(Array<Set<Int>>(C)), random_source, strategy, n_stable_rounds);
}

Int is_ball_or_sphere_h(const Lattice<BasicDecoration>& HD, const pm::SharedRandomState& random_source, const Int strategy, const Int n_stable_rounds);

template <typename Complex>
Int is_ball_or_sphere_h(const Complex& C, const pm::SharedRandomState& random_source, const Int strategy, const Int n_stable_rounds)
{
   return is_ball_or_sphere_h(hasse_diagram_from_facets(Array<Set<Int>>(C)), random_source, strategy, n_stable_rounds);
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
