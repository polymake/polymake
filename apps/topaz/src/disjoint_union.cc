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
#include "polymake/topaz/merge_vertices.h"

namespace polymake { namespace topaz {
   
BigObject disjoint_union(BigObject p1, BigObject p2,OptionSet options)
{
   const bool relabel=!options["no_labels"];
   Array<Set<Int>> UNION = p1.give("FACETS");
   Array<std::string> Labels = p1.give("VERTEX_LABELS");
   const Int n1 = Labels.size();

   const Array<Set<Int>> C2 = p2.give("FACETS");
   const Array<std::string> L2 = p2.give("VERTEX_LABELS");

   Int count = UNION.size();
   UNION.resize(count + C2.size());
   
   // add facets of C2
   for (auto f=entire(C2); !f.at_end(); ++f, ++count) {
      Set<Int> facet;
      for (auto v=entire(*f); !v.at_end(); ++v)
         facet += n1 + *v;
      
      UNION[count]=facet;
   }
   
   BigObject p_out("SimplicialComplex");
   p_out.set_description() << "Disjoint union of " << p1.name() << " and " << p2.name() << "."<<endl;
   p_out.take("FACETS") << UNION;

   if (relabel) {
      merge_disjoint_vertices(Labels, L2);
      p_out.take("VERTEX_LABELS") << Labels;
   }
   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Produce the __disjoint union__ of the two given complexes.\n"
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# The vertex labels are built from the original labels with a suffix ''_1'' or ''_2'' appended.\n"
                  "# @param SimplicialComplex complex1"
                  "# @param SimplicialComplex complex2"
                  "# @return SimplicialComplex"
                  "# @example The following creates the disjoint union of a triangle and an edge."
                  "# > $s = disjoint_union(simplex(2), simplex(1));"
                  "# > print $s -> F_VECTOR;"
                  "# | 5 4 1",
                  &disjoint_union, "disjoint_union(SimplicialComplex SimplicialComplex { no_labels => 0 })");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
