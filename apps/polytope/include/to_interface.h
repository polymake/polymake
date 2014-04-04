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

#ifndef POLYMAKE_POLYTOPE_TO_INTERFACE_H
#define POLYMAKE_POLYTOPE_TO_INTERFACE_H

#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/polytope/linsolver.h"


namespace polymake { namespace polytope { namespace to_interface {

template <typename Coord>
class solver {
public:
   typedef Coord coord_type;

   solver();

   typedef std::pair<coord_type, Vector<coord_type> > lp_solution;

   /// @retval first: objective value, second: solution
   // LP only defined for polytopes
   // CAUTION: to interpret the return value notice that this is a *dual* simplex
   lp_solution
   solve_lp(const Matrix<coord_type>& Inequalities, const Matrix<coord_type>& Equations,
            const Vector<coord_type>& Objective, bool maximize);

#if POLYMAKE_DEBUG
   bool debug_print;
#endif
};

} } }

#endif // POLYMAKE_POLYTOPE_TO_INTERFACE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
