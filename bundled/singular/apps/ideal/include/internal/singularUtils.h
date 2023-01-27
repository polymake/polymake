/* Copyright (c) 1997-2023
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

inline int safe_cast(Int x)
{
  if (x < std::numeric_limits<int>::min() || x > std::numeric_limits<int>::max())
    throw std::runtime_error("input too big for Singular");
  return static_cast<int>(x);
}

} // end namespace singular
} // end namespace ideal
} // end namespace polymake


