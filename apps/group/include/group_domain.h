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

#ifndef POLYMAKE_GROUP_DOMAIN_H
#define POLYMAKE_GROUP_DOMAIN_H
#undef DOMAIN

namespace polymake { namespace group {

// cf. enum in 'apps/polytope/rules/permlib.rules'
enum Domain {
   OnRays = 1, 
   OnFacets = 2, 
   OnCoords = 3
};

} // end namespace group
} // end namespace polymake

#endif // POLYMAKE_GROUP_DOMAIN_H

// Local Variables:
// mode:C++
// c-basis-offset:3
// indent-tabs-mode:nil
// End:
