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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/hash_set"


namespace polymake { namespace polytope {

template <typename Matrix>
bool detect_multiple(const GenericMatrix<Matrix> &mat)
{
   hash_set<typename Matrix::const_row_type> all;
   for (auto v=entire(rows(mat)); !v.at_end(); ++v)
      if (!all.insert(*v).second) return true;
   return false;
}

FunctionTemplate4perl("detect_multiple(Matrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
