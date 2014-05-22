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
#include "polymake/topaz/complex_tools.h"

#include "polymake/hash_set"

namespace polymake { namespace topaz {

void odd_complex_of_manifold(perl::Object p)
{
   const Array< Set<int> > C = p.give("FACETS");
   const bool is_mf = p.give("MANIFOLD");
   if (!is_mf)
      throw std::runtime_error("odd_complex_of_manifold: Complex is not a MANIFOLD");   
   
   HasseDiagram HD;
   perl::Object hd("FaceLattice");
   if ((p.lookup("HASSE_DIAGRAM") >> hd)) HD=HasseDiagram(hd);
   else  HD = hasse_diagram(C,C[0].size()-1,-2);
   
   if (C[0].size()-1 < 2)
      throw std::runtime_error("odd_complex_of_manifold: DIM of complex must be greater 2.");
   
   // create hash set for the facets of the Boundary
   const Array< Set<int> > Bound = p.give("BOUNDARY.INPUT_FACES");
   const PowerSet<int> Bound_sk = Bound[0].empty() ? PowerSet<int>() :
      k_skeleton(Bound, Bound[0].size()-2);
   
   hash_set< Set<int> > Boundary(Bound_sk.size());
   for (Entire< PowerSet<int> >::const_iterator c_it=entire(Bound_sk); !c_it.at_end(); ++c_it)
      Boundary.insert(*c_it);
   
   bool output = false;
   std::list< Set<int> > odd_complex;
   for (Entire<HasseDiagram::nodes_of_dim_set>::iterator f=entire(HD.nodes_of_dim(-3)); !f.at_end(); ++f)
      if ( HD.out_edges(*f).size() % 2 && Boundary.find(HD.face(*f)) ==  Boundary.end()) {
         output = true;
         odd_complex.push_back(HD.face(*f));
      }
   
   if (output)
      p.take("ODD_SUBCOMPLEX.INPUT_FACES") << as_array(odd_complex);
   else
      p.take("ODD_SUBCOMPLEX.INPUT_FACES") << perl::undefined();
}

Function4perl(&odd_complex_of_manifold,"odd_complex_of_manifold");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
