/* Copyright (c) 1997-2018
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
#include "polymake/graph/bipartite.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/topaz/hasse_diagram.h"
#include "polymake/hash_map"

namespace polymake { namespace topaz {

void odd_complex(perl::Object p)
{
   const Array< Set<int> > C = p.give("FACETS");
   const bool is_pure = p.give("PURE");
   if (!is_pure)
      throw std::runtime_error("odd_complex: Complex is not PURE.");
   Lattice<BasicDecoration> HD;
   perl::Object hd("Lattice<BasicDecoration>");
   if ((p.lookup("HASSE_DIAGRAM") >> hd)) HD=Lattice<BasicDecoration>(hd);
   else  HD = hasse_diagram_from_facets(C);

   if (C[0].size()-1 < 2)
      throw std::runtime_error("odd_complex: DIM of complex must be greater 2.");

   bool output = false;
   std::list< Set<int> > odd_complex;
   for (auto f=entire(HD.nodes_of_rank(HD.rank()-3));
        !f.at_end(); ++f) {

      Set<int> star_facets;
      const Graph<Directed> HDgraph=HD.graph();
      for (Entire< Graph<Directed>::const_out_adjacent_node_list_ref >::const_iterator n=entire(HD.out_adjacent_nodes(*f));
           !n.at_end(); ++n) {
         for (Entire<Graph<Directed>::const_out_adjacent_node_list_ref >::const_iterator nn=entire(HD.out_adjacent_nodes(*n));
              !nn.at_end(); ++nn)
            star_facets += *nn;
      }

      std::list< Set<int> > Link;
      Set<int> V_of_Link;
      for (Entire< Set<int> >::iterator s_it=entire(star_facets);
           !s_it.at_end(); ++s_it) {
         const Set<int> l = HD.face(*s_it)-HD.face(*f);
         Link.push_back(l);
         V_of_Link += l;
      }

      // create hash map for the vertices of Link
      hash_map<int, int> vertex_map(V_of_Link.size());
      int count=0;
      for (Entire< Set<int> >::iterator s_it=entire(V_of_Link);
           !s_it.at_end(); ++s_it, ++count)
         vertex_map[*s_it] = count;

      Graph<> G(V_of_Link.size());
      for (Entire< std::list< Set<int> > >::iterator l_it=entire(Link);
           !l_it.at_end(); ++l_it)
         G.edge( vertex_map[ l_it->front() ], vertex_map[ l_it->back() ] );

      if (graph::bipartite_sign(G)<0) {
         output = true;
         odd_complex.push_back(HD.face(*f));
      }
   }

   if (output)
      p.take("ODD_SUBCOMPLEX.FACETS") << as_array(odd_complex);
   else
      p.take("ODD_SUBCOMPLEX.FACETS") << perl::undefined();
}

Function4perl(&odd_complex,"odd_complex");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
