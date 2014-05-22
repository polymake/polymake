/* Copyright (c) 1997-2014
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
#include "polymake/polytope/lrs_interface.h"

namespace polymake { namespace polytope {

void lrs_ch_primal(perl::Object p)
{
   lrs_interface::solver solver;
   ch_primal(p, solver);
}

void lrs_ch_dual(perl::Object p)
{
   lrs_interface::solver solver;
   ch_dual(p, solver);
}

void lrs_count_vertices(perl::Object p, bool only_bounded)
{
   lrs_interface::solver solver;
   count_vertices(p, solver, only_bounded);
}

void lrs_count_facets(perl::Object p)
{
   lrs_interface::solver solver;
   count_facets(p, solver);
}

Function4perl(&lrs_ch_primal, "lrs_ch_primal(Cone<Rational>)");
Function4perl(&lrs_ch_dual, "lrs_ch_dual(Cone<Rational>)");
Function4perl(&lrs_count_vertices, "lrs_count_vertices(Cone<Rational>; $=0)");
Function4perl(&lrs_count_facets, "lrs_count_facets(Cone<Rational>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
