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

#ifndef POLYMAKE_CORE_WRAPPERS_Graph_H
#define POLYMAKE_CORE_WRAPPERS_Graph_H

#include_next "polymake/Graph.h"
#include "polymake/client.h"

namespace polymake { namespace perl_bindings {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T, typename T0>
   RecognizeType4perl("Polymake::common::Graph", (T0), Graph<T0>)

   template <typename T, typename T0, typename T1>
   RecognizeType4perl("Polymake::common::NodeMap", (T0,T1), NodeMap<T0,T1>)

   template <typename T, typename T0, typename T1>
   RecognizeType4perl("Polymake::common::EdgeMap", (T0,T1), EdgeMap<T0,T1>)

   template <typename T, typename T0, typename T1>
   RecognizeType4perl("Polymake::common::NodeHashMap", (T0,T1), NodeHashMap<T0,T1>)

   template <typename T, typename T0, typename T1>
   RecognizeType4perl("Polymake::common::EdgeHashMap", (T0,T1), EdgeHashMap<T0,T1>)

///==== Automatically generated contents end here.  Please do not delete this line. ====
} }

#endif // POLYMAKE_CORE_WRAPPERS_Graph_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
