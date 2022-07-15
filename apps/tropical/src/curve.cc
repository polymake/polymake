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

#include <map>
#include "polymake/hash_set"
#include "polymake/group/orbit.h"
#include "polymake/graph/GraphIso.h"
#include "polymake/tropical/curve.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace tropical {

bool
isomorphic_curves_impl(const Curve& ct_v,
                       const Curve& ct_w,
                       const Array<Int>& coloring_v,
                       const Map<Int,Int>& color_of_edge_for_w,
                       const Int verbosity)
{
   if (ct_v.get_marked_edges() != ct_w.get_marked_edges()) {
      if (verbosity > 3)
         cerr << "isomorphic_curves_impl: different marked edges, not isomorphic." << endl;
      return false;
   }

   const Array<Int> coloring_w(ct_w.induced_node_coloring(color_of_edge_for_w));
   
   if (verbosity > 3)
      cerr << "isomorphic_curves_impl: coloring_v = " << coloring_v
           << ", coloring_w = " << coloring_w << endl;

   if (!ct_v.get_marked_edges().size()) {
      // no marked edges, easy case
      const bool result(graph::isomorphic(ct_v.subdivided_graph(), coloring_v,
                                          ct_w.subdivided_graph(), coloring_w));
      if (verbosity > 3)
         cerr << "result of isomorphism test: " << result << endl;
      return result;
   }

   // there are marked edges
   if (verbosity > 3)
      cerr << "ct_v graph:\n" << ct_v.subdivided_graph()
           << "with coloring " << coloring_v << endl
           << "ct_w graph:\n" << ct_w.subdivided_graph()
           << "with coloring " << coloring_w << endl;

   const optional<Array<Int>> node_perm = graph::find_node_permutation(ct_v.subdivided_graph(), coloring_v,
                                                                       ct_w.subdivided_graph(), coloring_w);
   if (!node_perm) {
      if (verbosity > 3)
         cerr << "isomorphic_curves_impl: no node permutation" << endl;
      return false;
   }
   if (verbosity > 3)
      cerr << "node perm found: " << node_perm.value() << endl;
   
   const Array<Array<Int>> w_autos(graph::automorphisms(ct_w.subdivided_graph(), coloring_w));
   if (verbosity > 3) {
      if (w_autos.size()) 
         cerr << "w_autos:\n" << w_autos << endl;
      else
         cerr << "no autos." << endl;
   }

   const Int n(coloring_v.size());
   hash_set<Array<Int>> entire_group;
   if (w_autos.size())
      entire_group = group::unordered_orbit<pm::operations::group::on_container, Array<Int>, Array<Int>>(w_autos, Array<Int>(sequence(0,n)));
   else
      entire_group += Array<Int>(sequence(0,n));

   for (const auto& g: entire_group) {
      Array<Int> h(n);
      for (Int i=0; i<n; ++i)
         h[i] = g[node_perm.value()[i]];
      bool coloring_preserved(true);
      for (Int i=0; i<n && coloring_preserved; ++i)
         if (coloring_v[i] != coloring_w[h[i]]) 
            coloring_preserved = false;
      if (coloring_preserved) {
         if (verbosity > 4)
            cerr << "isomorphic_curves_impl: color-preserving iso is " << h << endl;
         return true;
      }
   }
   if (verbosity > 4)
      cerr << "isomorphic_curves_impl: no color-preserving iso found" << endl;
   return false;
}

void
fill_contracted_graph_collection(ContractedGraphCollection& cgc,
				 const Curve& tg)
{
   if (tg.get_verbosity() > 2)
      cerr << "making contracted graphs" << endl;
   
   for (Int codim=0; codim < tg.get_unmarked_edges().size(); ++codim)
      for (auto contracted_coos_it = entire(all_subsets_of_k(sequence(0, tg.get_unmarked_edges().size()), codim)); !contracted_coos_it.at_end(); ++contracted_coos_it) {
         const Set<Int> contracted_coos(*contracted_coos_it);
         if (!contracted_coos.size()) {
            cgc.emplace(std::make_pair(contracted_coos, tg));
            continue;
         }

         const Set<Int> less_contracted_coos(contracted_coos - contracted_coos.front());
         cgc.emplace(std::make_pair(contracted_coos,
                                    Curve(cgc.find(less_contracted_coos)->second,
                                          tg.get_unmarked_edges()[contracted_coos.front()])));
      }

   if (tg.get_verbosity() > 2)
      cerr << "done making " << cgc.size() << " contracted graphs." << endl;
}

  } }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
