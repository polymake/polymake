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
#include "polymake/topaz/morse_matching_tools.h"

namespace polymake { namespace topaz {

int morse_matching_size(perl::Object p)
{
   const HasseEdgeMap EM = p.give("HASSE_DIAGRAM.MORSE_MATCHING");
   return EdgeMapSize(EM);
}

UserFunction4perl("# @category Other"
                  "# Compute the number of edges in a Morse matching. "
                  "# @param SimplicialComplex complex a complex with a Morse matching "
                  "# @return Int the number of edges in the matching.",
                  &morse_matching_size, "morse_matching_size($)");
                  

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
