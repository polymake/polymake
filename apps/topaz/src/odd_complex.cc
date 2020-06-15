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
#include "polymake/graph/bipartite.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/topaz/hasse_diagram.h"
#include "polymake/hash_map"

namespace polymake { namespace topaz {

void odd_complex(BigObject p)
{
   const Array<Set<Int>> C = p.give("FACETS");
   const bool is_pure = p.give("PURE");
   if (!is_pure)
      throw std::runtime_error("odd_complex: Complex is not PURE.");

   Lattice<BasicDecoration> HD;
   BigObject hd;
   if (p.lookup("HASSE_DIAGRAM") >> hd)
      HD = Lattice<BasicDecoration>(hd);
   else
      HD = hasse_diagram_from_facets(C);

   if (C[0].size()-1 < 2)
      throw std::runtime_error("odd_complex: DIM of complex must be greater 2.");

   bool output = false;
   std::list<Set<Int>> odd_complex;
   for (const auto f : HD.nodes_of_rank(HD.rank()-3)) {
      Set<Int> star_facets;
      const Graph<Directed> HDgraph = HD.graph();
      for (const auto n : HD.out_adjacent_nodes(f)) {
         for (const auto nn : HD.out_adjacent_nodes(n))
            star_facets += nn;
      }

      std::list<Set<Int>> Link;
      Set<Int> V_of_Link;
      for (const auto sf : star_facets) {
         const Set<Int> l = HD.face(sf)-HD.face(f);
         Link.push_back(l);
         V_of_Link += l;
      }

      // create hash map for the vertices of Link
      hash_map<Int, Int> vertex_map(V_of_Link.size());
      Int count = 0;
      for (auto s_it = entire(V_of_Link); !s_it.at_end(); ++s_it, ++count)
         vertex_map[*s_it] = count;

      Graph<> G(V_of_Link.size());
      for (const auto& link : Link)
         G.edge( vertex_map[ link.front() ], vertex_map[ link.back() ] );

      if (graph::bipartite_sign(G)<0) {
         output = true;
         odd_complex.push_back(HD.face(f));
      }
   }

   if (output)
      p.take("ODD_SUBCOMPLEX.FACETS") << as_array(odd_complex);
   else
      p.take("ODD_SUBCOMPLEX.FACETS") << Undefined();
}

Function4perl(&odd_complex,"odd_complex");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
