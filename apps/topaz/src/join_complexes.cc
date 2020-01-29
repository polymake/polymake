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
#include "polymake/topaz/merge_vertices.h"

namespace polymake { namespace topaz {
  
BigObject join_complexes(BigObject p_in1,BigObject p_in2,OptionSet options)
{
   const bool relabel=!options["no_labels"];
   const Array<Set<Int>> C1 = p_in1.give("FACETS");
   const Int n1 = p_in1.give("N_VERTICES");

   const Array<Set<Int>> C2 = p_in2.give("FACETS");
       
   // join facets of C1 with the facets of C2
   Array<Set<Int>> Join( C1.size()*C2.size() );
   auto f = Join.begin();
   for (auto c_it1 = entire(C1); !c_it1.at_end(); ++c_it1)
      for (auto c_it2 = entire(C2); !c_it2.at_end(); ++c_it2, ++f) {
         *f = *c_it1;
         for (auto v=entire(*c_it2); !v.at_end(); ++v)
            *f += *v+n1;
      }
   
   BigObject p_out("topaz::SimplicialComplex");
   p_out.set_description()<<"Join of " << p_in1.name() << " and " << p_in2.name() << "."<<endl;
   p_out.take("FACETS") << Join;
    
   if (relabel) {
      Array<std::string> L1 = p_in1.give("VERTEX_LABELS");
      const Array<std::string> L2 = p_in2.give("VERTEX_LABELS");
      merge_disjoint_vertices(L1,L2);
      p_out.take("VERTEX_LABELS") << L1;
   }
   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Creates the join of //complex1// and //complex2//.\n"
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "#  The vertex labels are built from the original labels with a suffix ''_1'' or ''_2'' appended.\n"
                  "# @param SimplicialComplex complex1"
                  "# @param SimplicialComplex complex2"
                  "# @return SimplicialComplex",
                  &join_complexes, "join_complexes(SimplicialComplex SimplicialComplex { no_labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
