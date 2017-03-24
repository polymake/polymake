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

#include "polymake/polytope/hasse_diagram.h"

namespace polymake { namespace polytope {

	FunctionTemplate4perl("hasse_diagram(IncidenceMatrix, $)");
	FunctionTemplate4perl("bounded_hasse_diagram(IncidenceMatrix, Set<Int>; $=-1)");
	FunctionTemplate4perl("lower_hasse_diagram(IncidenceMatrix, $)");
	FunctionTemplate4perl("upper_hasse_diagram(IncidenceMatrix, $,$)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
