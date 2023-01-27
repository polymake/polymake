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

#pragma once

#include "polymake/client.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Graph.h"

namespace polymake { namespace graph {

namespace {

template <typename TGraph>
SparseMatrix<Int> incidence_matrix_impl(const GenericGraph<TGraph>& G, Int s = 1)
{
   SparseMatrix<Int> E(G.top().nodes(), G.top().edges());
   Int col = 0;
   for (auto eit = entire(edges(G.top())); !eit.at_end(); ++eit, ++col) {
      E(eit.from_node(), col) = s;
      E(eit.to_node(), col) = 1;
   }
   return E;
}

} // end anonymous namespace

template <typename TGraph>
SparseMatrix<Int> incidence_matrix(const GenericGraph<TGraph>& p)
{
   return incidence_matrix_impl(p.top(), 1);
}

template<typename TGraph>
SparseMatrix<Int> signed_incidence_matrix(const GenericGraph<TGraph>& p)
{
   return incidence_matrix_impl(p.top(), -1);
}

template<typename Dir>
SparseMatrix<Int> incidence_matrix(BigObject p)
{
   Graph<Dir> G = p.give("ADJACENCY");
   return incidence_matrix_impl(G, 1);
}

template<typename Dir>
SparseMatrix<Int> signed_incidence_matrix(BigObject p)
{
   Graph<Dir> G=p.give("ADJACENCY");
   return incidence_matrix_impl(G, -1);
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
