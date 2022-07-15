/* Copyright (c) 1997-2022
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
#include "polymake/topaz/complex_tools.h"
#include "polymake/topaz/hasse_diagram.h"
#include "polymake/hash_set"

namespace polymake { namespace topaz {

namespace{
Set<Int> apply_vertex_map(const Set<Int>& in, const Array<Int>& map)
{
   Set<Int> out;
   for (const Int i : in)
      out += map[i];
   return out;
}
}

void odd_complex_of_manifold(BigObject p)
{
   const Array<Set<Int>> C = p.give("FACETS");
   const bool is_mf = p.give("MANIFOLD");
   if (!is_mf)
      throw std::runtime_error("odd_complex_of_manifold: Complex is not a MANIFOLD");

   Lattice<BasicDecoration> HD;
   BigObject hd;
   if (p.lookup("HASSE_DIAGRAM") >> hd)
      HD = Lattice<BasicDecoration>(hd);
   else
      HD = hasse_diagram_from_facets(C);

   if (C[0].size()-1 < 2)
      throw std::runtime_error("odd_complex_of_manifold: DIM of complex must be greater 2.");

   // create hash set for the facets of the Boundary
   const Array<Set<Int>> Bound = p.give("BOUNDARY.FACETS");
   const PowerSet<Int> Bound_sk = Bound[0].empty() ? PowerSet<Int>() :
      k_skeleton(Bound, Bound[0].size()-2);

   const Array<Int> map = p.give("BOUNDARY.VERTEX_MAP");
   hash_set<Set<Int>> Boundary(Bound_sk.size());
   for (auto c_it=entire(Bound_sk); !c_it.at_end(); ++c_it)
      Boundary.insert(apply_vertex_map(*c_it,map));

   bool output = false;
   std::list<Set<Int>> odd_complex;
   for (const auto f : HD.nodes_of_rank(HD.rank()-3))
      if (HD.out_edges(f).size()%2 != 0 && Boundary.find(HD.face(f)) == Boundary.end()) {
         output = true;
         odd_complex.push_back(HD.face(f));
      }

   if (output)
      p.take("ODD_SUBCOMPLEX.INPUT_FACES") << as_array(odd_complex);
   else
      p.take("ODD_SUBCOMPLEX.INPUT_FACES") << Undefined();
}

Function4perl(&odd_complex_of_manifold,"odd_complex_of_manifold");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
