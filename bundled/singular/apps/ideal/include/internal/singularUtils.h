/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_IDEAL_INTERNAL_SINGULAR_UTILS_H
#define POLYMAKE_IDEAL_INTERNAL_SINGULAR_UTILS_H

// Singular includes
#include "polymake/ideal/internal/singularInclude.h"

// polymake includes
#include "polymake/client.h"
#include "polymake/Map.h"

namespace polymake { 
namespace ideal {
namespace singular {
   

   // Storing the handles for the Singular functions globally.
   extern Map<std::string, idhdl> singular_function_map;

   idhdl get_singular_function(std::string s);



} // end namespace singular
} // end namespace ideal
} // end namespace polymake


#endif
