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

#include "polymake/common/labels.h"

namespace polymake { namespace common {

std::vector<std::string> read_labels(const BigObject& p, AnyString label_prop, Int n_labels)
{
   std::vector<std::string> labels;
   if (p.lookup(label_prop) >> labels) {
      if (Int(labels.size()) != n_labels)
         throw std::runtime_error("read_labels(): unexpected number of labels: " + std::to_string(labels.size()) +
                                  " instead of " + std::to_string(n_labels));
   } else {
      labels.reserve(n_labels);
      for (Int i = 0; i < n_labels; ++i)
         labels.emplace_back(std::to_string(i));
   }
   return labels;
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
