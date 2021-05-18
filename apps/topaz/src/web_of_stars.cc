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
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/topaz/sum_triangulation_tools.h"

namespace polymake { namespace topaz {

IncidenceMatrix<> web_of_stars(const Array<Int>& poset_hom,
                               const Array<Set<Set<Int>>>& star_shaped_balls,
                               const Array<Set<Int>>& simplices)
{
   Map<Set<Int>, Int> index_of;
   Int index = -1;
   for (auto ait = entire(simplices); !ait.at_end(); ++ait)
      index_of[*ait] = ++index;

   IncidenceMatrix<> wos(poset_hom.size(), simplices.size());
   for (Int i = 0; i < poset_hom.size(); ++i) {
      Set<Int> image_indices;
      for (auto sit = entire(star_shaped_balls[poset_hom[i]]); !sit.at_end(); ++sit)
         image_indices += index_of[*sit];
      wos[i] = image_indices;
   }
   return wos;
}

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Produce a web of stars from two given triangulations\n"
                  "# and a map between them.\n"
                  "# @param Array<Int> poset_hom the poset homomorphism from stabbing order to star-shaped balls"
                  "# @param Array<Set<Set<Int>>> star_shaped_balls the collection of star-shaped balls of T"
                  "# @param Array<Set<Int>> triang the facets of the underlying triangulation of Q"
                  "# @return IncidenceMatrix WebOfStars Every row corresponds to a full dimensional simplex in P and every column to a full dimensional simplex in Q.",
                  &web_of_stars,
                  "web_of_stars(Array<Int>, Array<Set<Set<Int>>>, Array<Set<Int>>)");
                  

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
