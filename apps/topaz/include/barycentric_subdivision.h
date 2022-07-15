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

#pragma once
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/graph/maximal_chains.h"
#include "polymake/topaz/hasse_diagram.h"

namespace polymake { namespace topaz {


using graph::lattice::BasicDecoration;
using graph::lattice::Nonsequential;

namespace {

inline
Int
adjusted_index(const Int i,
               const Int bottom_node_index,
               const bool ignore_bottom_node,
               const Int top_node_index,
               const bool ignore_top_node)
{
   const Int adj =
      i
      - (ignore_bottom_node && i > bottom_node_index)
      - (ignore_top_node    && i > top_node_index);
   assert(adj>=0);
   return adj;
}

inline
Int
reverse_adjusted_index(const Int i,
                       const Int bottom_node_index,
                       const bool ignore_bottom_node,
                       const Int top_node_index,
                       const bool ignore_top_node)
{
   return
      i
      + (ignore_bottom_node && i >= bottom_node_index)
      + (ignore_top_node    && i >= top_node_index);
}
      
// Renumbers node indices as if bottom node (and potentially top node)
// are removed
template <typename Decoration, typename SeqType>
void
bs_renumber_nodes(Array<Set<Int>>& facets,
                  const graph::Lattice<Decoration, SeqType>& HD,
                  const bool ignore_top_node)
{
   const Int b_index = HD.bottom_node();
   const Int t_index = HD.top_node();
   for (auto f_it = entire(facets); !f_it.at_end(); ++f_it) {
      Set<Int> nset;
      for (auto n : (*f_it)) {
         if (n<0)
            cerr << "bs_renumber_nodes: weird *f_it: " << *f_it << endl;
         nset += adjusted_index(n, b_index, true, t_index, ignore_top_node);
      }
      *f_it = nset;
   }
}

} // end anonymous namespace


struct FacetsAndLabels {
   Array<Set<Int>> facets;
   Array<Set<Set<Int>>> labels;
};
      

/*
    Template type of input Hasse diagram:
    - for Polytope :         SeqType = Sequential,
    - for SimplicialComplex: SeqType = Nonsequential

    Output: two arrays
    - The first array contains the facets of the second barycentric subdivision 
      as a simplicial complex. The node indices refer to the nodes of the 
      Hasse diagram of the first barycentric subdivision, which is not returned

    - The second array contains the label for each vertex in the first array.
      This is a tuple of faces of the original complex such that the vertex 
      in question is the barycenter of those face barycenters.
*/
template<typename SeqType>   
FacetsAndLabels
first_barycentric_subdivision(const Lattice<BasicDecoration, SeqType>& HD0,
                              const bool ignore_top_node = false)
{
   // the Hasse diagram can come ordered top-to-bottom or bottom-to-top, or be unordered in the case of Nonsequential, and always has a top and a bottom node
   // depending on this, we have to do some index gymnastics
   const Int
      HD0_top_index   (HD0.top_node()),
      HD0_bottom_index(HD0.bottom_node());
   const bool
      HD0_top_invalid(ignore_top_node || scalar2set(-1) == HD0.decoration()[HD0_top_index].face),
      HD0_bottom_empty(0 == HD0.decoration()[HD0_bottom_index].face.size());

#if POLYMAKE_DEBUG   
   if (get_debug_level() > 1)
      cerr << "ignore_top_node: " << ignore_top_node << "\n"
           << "HD0_top_index: " << HD0_top_index << "\n"
           << "HD0_bottom_index: " << HD0_bottom_index << "\n"
           << "HD0_top_invalid: " << HD0_top_invalid << "\n"
           << "HD0_bottom_empty: " << HD0_bottom_empty << endl;
#endif

   FacetsAndLabels facets_and_labels;
   // make the first barycentric subdivision
   //   Array<Set<Int>> HD1_facets = maximal_chains(HD0, HD0_bottom_empty, HD0_top_invalid);
   facets_and_labels.facets = maximal_chains(HD0, HD0_bottom_empty, HD0_top_invalid);
   topaz::bs_renumber_nodes(facets_and_labels.facets, HD0, HD0_top_invalid);

   // make the vertex labels
   facets_and_labels.labels = Array<Set<Set<Int>>> (HD0.nodes() - HD0_top_invalid - HD0_bottom_empty);

   for (auto hd1_it = entire<indexed>(HD0.decoration()); !hd1_it.at_end(); ++hd1_it) {
      if ((HD0_top_index    == hd1_it.index() && HD0_top_invalid) ||
          (HD0_bottom_index == hd1_it.index() && HD0_bottom_empty))
          continue;

      facets_and_labels.labels[topaz::adjusted_index(hd1_it.index(), HD0_bottom_index, true, HD0_top_index, HD0_top_invalid)] += hd1_it->face;
   }
   
   return facets_and_labels;
}   

/*
  Same input and output as for first_barycentric_subdivision.
  There are some slight differences in the code that make refactoring not 
  really worthwhile.
 */
template<typename SeqType>   
std::pair<Array<Set<Int>>, Array<Set<Set<Int>>>>
second_barycentric_subdivision(const Lattice<BasicDecoration, SeqType>& HD0,
                               const bool ignore_top_node = false)
{
   // the Hasse diagram can come ordered top-to-bottom or bottom-to-top, or be unordered in the case of Nonsequential, and always has a top and a bottom node
   // depending on this, we have to do some index gymnastics
   const Int
      HD0_top_index   (HD0.top_node()),
      HD0_bottom_index(HD0.bottom_node());
   const bool
      HD0_top_invalid(ignore_top_node || scalar2set(-1) == HD0.decoration()[HD0_top_index].face),
      HD0_bottom_empty(0 == HD0.decoration()[HD0_bottom_index].face.size());
   
   // make the first barycentric subdivision
   Array<Set<Int>> HD1_facets = maximal_chains(HD0, HD0_bottom_empty, HD0_top_invalid);
   bs_renumber_nodes(HD1_facets, HD0, HD0_top_invalid);
   
   const Lattice<BasicDecoration, Nonsequential> HD1 = hasse_diagram_from_facets(HD1_facets);

   // extract metadata from first barycentric subdivision
   const Int
      HD1_bottom_index(HD1.bottom_node()),
      HD1_top_index(HD1.top_node());
   
   const bool
      HD1_bottom_empty(0 == HD1.decoration()[HD1_bottom_index].face.size()),
      HD1_top_invalid(scalar2set(-1) == HD1.decoration()[HD1_top_index].face);

   // make the facets of the second barycentric subdivision
   Array<Set<Int>> HD2_facets = maximal_chains(HD1, HD1_bottom_empty, HD1_top_invalid);
   bs_renumber_nodes(HD2_facets, HD1, HD1_top_invalid);
   
   // make the vertex labels
   Array<Set<Set<Int>>> HD2_labels_as_set(HD1.nodes() - HD1_top_invalid - HD1_bottom_empty);
   auto HD2_labels_as_set_it = entire(HD2_labels_as_set);

   for (auto hd1_it = entire<indexed>(HD1.decoration()); !hd1_it.at_end(); ++hd1_it) {
      if ((HD1_top_index    == hd1_it.index() && HD1_top_invalid) ||
          (HD1_bottom_index == hd1_it.index() && HD1_bottom_empty))
          continue;

      for (const auto node_index: hd1_it->face) 
         *HD2_labels_as_set_it += HD0.decoration()[reverse_adjusted_index(node_index, HD0_bottom_index, true, HD0_top_index, HD0_top_invalid)].face;

      ++HD2_labels_as_set_it;
   }
   
   return std::make_pair(HD2_facets, HD2_labels_as_set);
}


} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
