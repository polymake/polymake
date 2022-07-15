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

#ifndef POLYMAKE_APP_WRAPPERS_graph_Decoration_H
#define POLYMAKE_APP_WRAPPERS_graph_Decoration_H

#include_next "polymake/graph/Decoration.h"
#include "polymake/client.h"

namespace polymake { namespace perl_bindings {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T>
   RecognizeType4perl("Polymake::graph::BasicDecoration", (), graph::lattice::BasicDecoration)

   template <typename T, typename T0>
   RecognizeType4perl("Polymake::graph::InverseRankMap", (T0), graph::lattice::InverseRankMap<T0>)
///==== Automatically generated contents end here.  Please do not delete this line. ====
} }

#endif // POLYMAKE_APP_WRAPPERS_graph_Decoration_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
