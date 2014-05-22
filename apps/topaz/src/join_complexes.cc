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
#include "polymake/topaz/merge_vertices.h"

namespace polymake { namespace topaz {
  
perl::Object join_complexes(perl::Object p_in1,perl::Object p_in2,perl::OptionSet options)
{
   const bool relabel=options["labels"];
   const Array< Set<int> > C1 = p_in1.give("FACETS");
   const int n1 = p_in1.give("N_VERTICES");

   const Array< Set<int> > C2 = p_in2.give("FACETS");
       
   // join facets of C1 with the facets of C2
   Array< Set<int> > Join( C1.size()*C2.size() );
   Array< Set<int> >::iterator f=Join.begin();
   for (Entire< Array< Set<int> > >::const_iterator c_it1=entire(C1);
        !c_it1.at_end(); ++c_it1)
      for (Entire< Array< Set<int> > >::const_iterator c_it2=entire(C2);
           !c_it2.at_end(); ++c_it2, ++f) {
         *f = *c_it1;
         for (Entire< Set<int> >::const_iterator v=entire(*c_it2);
              !v.at_end(); ++v)
            *f += *v+n1;
      }
   
   perl::Object p_out("topaz::SimplicialComplex");
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
                  "# @option Bool labels creates [[VERTEX_LABELS]].\n"
                  "#  The vertex labels are built from the original labels with a suffix ''_1'' or ''_2'' appended.\n"
                  "# @param SimplicialComplex complex1"
                  "# @param SimplicialComplex complex2"
                  "# @return SimplicialComplex",
                  &join_complexes, "join_complexes(SimplicialComplex SimplicialComplex { labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
