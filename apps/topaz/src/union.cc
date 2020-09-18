/* Copyright (c) 1997-2020
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
#include "polymake/Array.h"
#include "polymake/FacetList.h"
#include "polymake/topaz/merge_vertices.h"
#include "polymake/hash_map"

namespace polymake { namespace topaz {
  
BigObject t_union(BigObject p1, BigObject p2,OptionSet options)
{
   const bool relabel=!options["no_labels"];
   const Array<Set<Int>> C1 = p1.give("FACETS");
   Array<std::string> Labels = p1.give("VERTEX_LABELS");
   
   const Array<Set<Int>> C2 = p2.give("FACETS");
   const Array<std::string> L2 = p2.give("VERTEX_LABELS");
   
   // compute vertex map
   hash_map<Int, Int> map = merge_vertices(Labels,L2);
   
   // add facets of C1
   FacetList Union;      
   for (auto c_it = entire(C1); !c_it.at_end(); ++c_it)
      Union.insert(*c_it);
   
   // add facets of C2
   for (auto c_it = entire(C2); !c_it.at_end(); ++c_it) {
      Set<Int> f;
      for (auto v=entire(*c_it); !v.at_end(); ++v)
         f += map[*v];
      
      Union.insertMax(f);
   }
   BigObject p_out("SimplicialComplex");
   p_out.set_description() << "Union of " << p1.name() << " and " << p2.name() << "."<<endl;
   p_out.take("FACETS") << Union;
   
   if (relabel)
      p_out.take("VERTEX_LABELS") << Labels;

   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Produce the union of the two given complexes, identifying\n"
                  "# vertices with equal labels.\n"
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @param SimplicialComplex complex1"
                  "# @param SimplicialComplex complex2"
                  "# @return SimplicialComplex"
                  "# @example The union of two 3-simplices with the same labels on vertices produces the 3-simplex again."
                  "# > print union(simplex(3), simplex(3)) -> F_VECTOR;"
                  "# | 4 6 4 1",



                  &t_union, "union(SimplicialComplex SimplicialComplex { no_labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
