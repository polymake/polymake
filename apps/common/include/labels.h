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
#include "polymake/vector"

namespace polymake { namespace common {

//! Read a `labels' property
//! Generate labels from integrals 0..n_labels-1 if the property does not exist
std::vector<std::string> read_labels(const BigObject& p, AnyString label_prop, Int n_labels);

//! Read a `labels' property into a properly sized container
template <typename Container>
std::enable_if_t<pm::isomorphic_to_container_of<pure_type_t<Container>, std::string>::value>
read_labels(const BigObject& p, AnyString label_prop, Container&& labels)
{
   if (!(p.lookup(label_prop) >> labels)) {
      Int i = 0;
      for (auto l = entire(labels); !l.at_end(); ++l, ++i)
         *l = std::to_string(i);
   }
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
