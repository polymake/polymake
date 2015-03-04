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

#define USE_DDF_
#include "polymake/polytope/cdd_interface_impl.h"
#include <sstream>

namespace polymake { namespace polytope { namespace cdd_interface {

template <> inline
double cdd_lp_sol<double>::optimal_value() const
{
   return double(ptr->optvalue[0]);
}

template class cdd_matrix<double>;
template class cdd_polyhedron<double>;
template class solver<double>;

} } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
