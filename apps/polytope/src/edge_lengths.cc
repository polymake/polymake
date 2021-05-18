/* Copyright (c) 1997-2021
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
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include <cmath>

namespace polymake { namespace polytope {

template <typename Scalar>
Array<Scalar> edge_lengths(const Array<Vector<Scalar>>& V)
{
  const Int n = V.size();
  Array<Scalar> length(n);
  for (Int i = 0; i < n; ++i)
      length[i]=accumulate(attach_operation(V[i], operations::abs_value()), operations::max());
  return length;
}

FunctionTemplate4perl("edge_lengths<Scalar>(Array<Vector<Scalar>>)");

}}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
