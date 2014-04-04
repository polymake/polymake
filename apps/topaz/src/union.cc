/* Copyright (c) 1997-2014
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
#include "polymake/Array.h"
#include "polymake/FacetList.h"
#include "polymake/topaz/merge_vertices.h"
#include "polymake/hash_map"

namespace polymake { namespace topaz {
  
perl::Object t_union(perl::Object p1, perl::Object p2,perl::OptionSet options)
{
   const bool relabel=options["labels"];
   const Array< Set<int> > C1 = p1.give("FACETS");
   Array<std::string> Labels = p1.give("VERTEX_LABELS");
   
   const Array< Set<int> > C2 = p2.give("FACETS");
   const Array<std::string> L2 = p2.give("VERTEX_LABELS");
   
   // compute vertex map
   hash_map<int,int> map = merge_vertices(Labels,L2);
   
   // add facets of C1
   FacetList Union;      
   for (Entire< Array< Set<int> > >::const_iterator c_it=entire(C1);
        !c_it.at_end(); ++c_it)
      Union.insert(*c_it);
   
   // add facets of C2
   for (Entire< Array< Set<int> > >::const_iterator c_it=entire(C2);
        !c_it.at_end(); ++c_it) {
      Set<int> f;
      for (Entire< Set<int> >::const_iterator v=entire(*c_it);
           !v.at_end(); ++v)
         f += map[*v];
      
      Union.insertMax(f);
   }
   perl::Object p_out("SimplicialComplex");
   p_out.set_description() << "Union of " << p1.name() << " and " << p2.name() << "."<<endl;
   p_out.take("FACETS") << Union;
   
   if (relabel)
      p_out.take("VERTEX_LABELS") << Labels;

   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Produce the union of the two given complexes, identifying\n"
                  "# vertices with equal labels.\n"
                  "# @option Bool labels creates [[VERTEX_LABELS]].\n"
                  "# @param SimplicialComplex complex1"
                  "# @param SimplicialComplex complex2"
                  "# @return SimplicialComplex",
                  &t_union, "union(SimplicialComplex SimplicialComplex { labels => 1 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
