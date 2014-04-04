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

#ifndef POLYMAKE_POLYTOPE_CANONICALIZE_H
#define POLYMAKE_POLYTOPE_CANONICALIZE_H


#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/permutations.h"

namespace polymake { namespace polytope {

template <typename Iterator> inline
void canonicalize_oriented(Iterator e)
{
   if (!e.at_end() && !abs_equal(*e,1)) {
      const typename Iterator::value_type leading=abs(*e);
      do *e /= leading; while (!(++e).at_end());
   }
}

  }
}

#endif
