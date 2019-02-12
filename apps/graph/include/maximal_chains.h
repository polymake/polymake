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

#ifndef POLYMAKE_GRAPH_MAXIMAL_CHAINS_H
#define POLYMAKE_GRAPH_MAXIMAL_CHAINS_H

#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"

#include <string>

namespace polymake { namespace graph {

// Computes the barycentric subdivision
// @param HD A Lattice
// @param ignore_bottom_node If true, the bottom node is not included in the facet comptutation
// @param ignore_top_node If true, the top node is not included in the facet computation
// @return Array<Set<int> > Facets of the barycentric subdivision.
// Indices are node indices in the Hasse diagram, in particular they need not start
// with 0.
template <typename Decoration, typename SeqType>
Array<Set<int>> maximal_chains(const Lattice<Decoration, SeqType>& HD, bool ignore_bottom_node, bool ignore_top_node)
{
   const int total_rank = HD.rank();
   const int dim = total_rank - 1 - ignore_top_node;
   const int top_index = HD.top_node();
   const int bottom_index = HD.bottom_node();

   // each old facet is divided into at least (dim+1)! cells, with equality iff the object is simplicial.
   // since we don't know the size beforehand, we use a std::vector instead of an Array.
   // each facet of the barycentric subdivision is a flag in the input face lattice HD,
   // stored as the set of node indices of the constituent faces in HD
   std::vector<Set<int>> facets;
   facets.reserve(HD.nodes_of_rank(total_rank-1).size() * int(Integer::fac(dim+1)));

   using out_edge = Graph<Directed>::out_edge_list::const_iterator;
   using stack_type = std::vector<out_edge>;  // vector is more efficient than list
   stack_type flag;
   flag.reserve(dim+1);

   // If it has only the bottom node, return the empty list
   if (HD.graph().nodes() == 1) {
      Array<Set<int>> trivial_result(ignore_top_node || ignore_bottom_node ? 0 : 1);
      if (!(ignore_top_node || ignore_bottom_node)) {
         trivial_result[0] = scalar2set(bottom_index);
      }
      return trivial_result;
   }

   // start with the "empty set" node - just for convenience
   flag.push_back(HD.out_edges(bottom_index).begin());

   do {
      // complete the facet
      while (true) {
         int n = flag.back().to_node();
         if (n == top_index) break;
         flag.push_back(HD.out_edges(n).begin()); // the index of the next face in the flag
      }

      // copy the facet
      Set<int> facet;
      if (!ignore_bottom_node) facet += bottom_index;
      for (auto s=entire(flag);  !s.at_end();  ++s) {
         if (!ignore_top_node || s->to_node() != top_index)
            facet += s->to_node();
      }
      facets.push_back(facet);

      // depth-first search to the next facet
      do {
         if (!(++flag.back()).at_end()) break;
         flag.pop_back();
      } while (!flag.empty());
   } while (!flag.empty());

   return Array<Set<int>>(facets);
}

// Computes the VERTEX_LABELS
// If ignore_top_node is true, the top node will get the empty face (i.e. the length of the list
// is always the number of nodes.
template <typename Decoration, typename SeqType>
Array<std::string> bs_labels(const Lattice<Decoration, SeqType>& HD, const Array<std::string>& old_labels, bool ignore_top_node)
{
   Array<std::string> L(HD.nodes());
   auto f = entire<indexed>(HD.decoration());
   std::ostringstream label;
   const bool convert_old_labels = !old_labels.empty();
   const int top_index = HD.top_node();
   for (auto l=entire(L); !l.at_end(); ++l, ++f) {
      if (ignore_top_node && f.index() == top_index) {
         *l = label.str();
         continue;
      }
      if (!convert_old_labels) {
         wrap(label) << f->face;
      } else {
         wrap(label) << "{";
         bool first = true;
         for (auto fsit = entire(f->face); !fsit.at_end(); ++fsit) {
            if (first) first = false;
            else wrap(label) << " ";
            wrap(label) << old_labels[*fsit];
         }
         wrap(label) << "}";
      }
      *l=label.str();
      label.str("");
   }
   return L;
}

// Computes the GEOMETRIC_REALIZATION
// If ignore_top_node = true, the top node will get a zero vector (i.e. the length of the list
// is always the number of nodes
template <typename Scalar, typename Decoration, typename SeqType>
Matrix<Scalar> bs_geom_real(const Matrix<Scalar>& old_coord, const Lattice<Decoration, SeqType> HD, bool ignore_top_node)
{
   const int ambient_dim = old_coord.cols();
   const int top_index = HD.top_node();
   Matrix<Scalar> new_coord(HD.nodes(), ambient_dim);

   auto f = entire<indexed>(HD.decoration());
   for (auto r=entire(rows(new_coord));  !r.at_end();  ++r, ++f) {
      if (ignore_top_node && f.index() == top_index) continue;
      accumulate_in(entire(select(rows(old_coord), f->face)), operations::add(), *r);
      if (f->face.size()) {
         *r /= f->face.size();
      } else {
         (*r)[0] = one_value<Scalar>();
      }
   }
   return new_coord;
}

} }

#endif // POLYMAKE_GRAPH_MAXIMAL_CHAINS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
