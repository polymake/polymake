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

#include "polymake/client.h"
#include "polymake/polytope/cdd_interface.h"
#include "polymake/polytope/generic_convex_hull_client.h"

namespace polymake { namespace polytope { namespace cdd_interface {

template <typename Scalar>
auto create_convex_hull_solver(CanEliminateRedundancies needsEliminate = CanEliminateRedundancies::yes)
{
   ListReturn ret;
   if (needsEliminate == CanEliminateRedundancies::yes)
      ret << cached_convex_hull_solver<Scalar, CanEliminateRedundancies::yes>(new ConvexHullSolver<Scalar>(), true);
   else
      ret << cached_convex_hull_solver<Scalar, CanEliminateRedundancies::no>(new ConvexHullSolver<Scalar>(), true);
   return ret;
}

}

template <typename Scalar>
void cdd_ch_primal(BigObject p, const bool verbose, const bool isCone)
{
   generic_convex_hull_primal<Scalar>(p, isCone, cdd_interface::ConvexHullSolver<Scalar>(verbose));
}

template <typename Scalar>
void cdd_ch_dual(BigObject p, const bool verbose, const bool isCone)
{
   generic_convex_hull_dual<Scalar>(p, isCone, cdd_interface::ConvexHullSolver<Scalar>(verbose));
}

FunctionTemplate4perl("cdd_ch_primal<Scalar> (Cone<Scalar>; $=false, $=true)");
FunctionTemplate4perl("cdd_ch_dual<Scalar> (Cone<Scalar>; $=false, $=true)");

FunctionTemplate4perl("cdd_ch_primal<Scalar> (Polytope<Scalar>; $=false, $=false)");
FunctionTemplate4perl("cdd_ch_dual<Scalar> (Polytope<Scalar>; $=false, $=false)");

InsertEmbeddedRule("function cdd.convex_hull: create_convex_hull_solver<Scalar> [Scalar==Rational || Scalar==Float] (;$=0)"
                   " : c++ (name => 'cdd_interface::create_convex_hull_solver') : returns(cached);\n");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
