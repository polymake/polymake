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

#include "polymake/client.h"
#include "polymake/graph/hungarian_method.h"

namespace polymake { namespace graph {

    template <typename E>
    Array<int> hungarian_perfect_matching(const Matrix<E>& weights) {
      return HungarianMethod<E>(weights).stage();
    }
    

UserFunctionTemplate4perl("#@category Other"
                  "# Vector representation of the permutation corresponding to a perfect matching in a weighted bipartite graph."
                  "# @param Matrix weights"
                  "# @return Array"
                  "# @example The following computes a matching in a small bipartite weighted graph:"
                  "# > $M = new Matrix(['inf',2,'inf',1],[2,'inf',1,'inf'],['inf',1,'inf',8],[1,'inf',8,'inf']);"
                  "# > print hungarian_perfect_matching($M);"
                  "# | 3 2 1 0",
                  "hungarian_perfect_matching(Matrix)");
}  }



// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:










