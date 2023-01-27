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

#include "polymake/pair.h"
#include "polymake/internal/comparators_ops.h"
#include "polymake/internal/type_union.h"
#include "polymake/internal/shared_object.h"
#include "polymake/FacetList.h"
#include <stdexcept>

namespace pm {
namespace unions {

void invalid_null_op()
{
   throw std::runtime_error("invalid operation on an empty union object");
}

}
namespace fl_internal {

const facet superset_iterator::empty_facet(0);

}

shared_object_secrets::rep<> shared_object_secrets::empty_rep;

double spec_object_traits<double>::global_epsilon = 1e-7;

}

namespace std {
   pm::nothing pair<pm::nothing, pm::nothing>::first, pair<pm::nothing, pm::nothing>::second;
}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
