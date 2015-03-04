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

#ifndef POLYMAKE_TOPAZ_IS_SPHERE_H_H
#define POLYMAKE_TOPAZ_IS_SPHERE_H_H

#include "polymake/IndexedSubset.h"
#include "polymake/Integer.h"
#include "polymake/topaz/BistellarComplex.h"
#include "polymake/topaz/SimplicialComplex_as_FaceMap.h"
#include "polymake/topaz/ChainComplex.h"

namespace polymake { namespace topaz {

// return values: 1=true, 0=false, -1=undef
  
int is_sphere_h(const HasseDiagram& HD, const pm::SharedRandomState& random_source, const int strategy, const int n_stable_rounds);

template <typename Complex>
int is_sphere_h(const Complex& C, const pm::SharedRandomState& random_source, const int strategy, const int n_stable_rounds)
{
   return is_sphere_h(pure_hasse_diagram(C), random_source, strategy, n_stable_rounds);
}

int is_ball_or_sphere_h(const HasseDiagram& HD, const pm::SharedRandomState& random_source, const int strategy, const int n_stable_rounds);

template <typename Complex>
int is_ball_or_sphere_h(const Complex& C, const pm::SharedRandomState& random_source, const int strategy, const int n_stable_rounds)
{
   return is_ball_or_sphere_h(pure_hasse_diagram(C), random_source, strategy, n_stable_rounds);
}

} }

#endif // POLYMAKE_TOPAZ_IS_SPHERE_H_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
