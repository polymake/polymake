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

#pragma once


#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/permutations.h"

namespace polymake { namespace polytope {

template <typename Iterator> inline
void canonicalize_oriented(Iterator&& it)
{
  typedef typename pm::iterator_traits<Iterator>::value_type Scalar;
  while (!it.at_end() && is_zero(*it))
    ++it;
  if (!it.at_end() && !abs_equal(*it, one_value<Scalar>())) {
      const Scalar leading=abs(*it);
      auto&& tail=ensure_private_mutable(std::forward<Iterator>(it));
      do *tail /= leading; while (!(++tail).at_end());
   }
}

} }

