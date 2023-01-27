/* Copyright (c) 1997-2023
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

#include <cstring>
#include <cstddef> // needed for gcc 4.9, see http://gcc.gnu.org/gcc-4.9/porting_to.html

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include <bliss/graph.hh>
#if BLISS_VERSION_MAJOR > 0 || BLISS_VERSION_MINOR >= 76
#include <bliss/digraph.hh>
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "polymake/graph/GraphIso.h"
#include "polymake/permutations.h"

namespace polymake { namespace graph {

struct GraphIso::impl {
   bliss::AbstractGraph* src_graph;
   bliss::AbstractGraph* canon_graph;
   unsigned int* canon_labels;
   int n_colors;
   bool digraph;

   impl(int n_arg, bool dir)
      : n_colors(0)
      , digraph(dir)
   {
      if (n_arg > std::numeric_limits<int>::max())
         throw std::runtime_error("input graph is too big for bliss");
      if (dir)
         src_graph = new bliss::Digraph(n_arg);
      else
         src_graph = new bliss::Graph(n_arg);
      canon_labels = new unsigned int[n_arg];
      canon_graph = nullptr;
   }

   ~impl()
   {
      delete canon_graph;
      delete[] canon_labels;
      delete src_graph;
   }

   void set_color(int from, int to, int color)
   {
      if (digraph) {
         bliss::Digraph* g = static_cast<bliss::Digraph*>(src_graph);
         for (int i = from; i < to; ++i)
            g->change_color(i, color);
      } else {
         bliss::Graph* g = static_cast<bliss::Graph *>(src_graph);
         for (int i = from; i < to; ++i)
            g->change_color(i, color);
      }
   }

   static void store_autom(void *graph, unsigned int n, const unsigned int* aut)
   {
      GraphIso* g = reinterpret_cast<GraphIso*>(graph);
      g->n_autom++;
      g->autom.push_back( Array<Int>(n, aut) );
   }
};

GraphIso::impl* GraphIso::alloc_impl(Int n, bool dir, bool is_colored)
{
   if (n > std::numeric_limits<int>::max())
      throw std::runtime_error("Graph with more than 2^31 nodes is too big for bliss");
   return new impl(int(n), dir);
}

GraphIso::~GraphIso() { delete p_impl; }

void GraphIso::add_edge(Int from, Int to)
{
   // node indexes can't exceed the total node count checked during initial allocation,
   // therefore no further overflow checks
   if (p_impl->digraph) {
      bliss::Digraph* g = static_cast<bliss::Digraph*>(p_impl->src_graph);
      g->add_edge(int(from), int(to));
   } else {
      bliss::Graph* g = static_cast<bliss::Graph*>(p_impl->src_graph);
      g->add_edge(int(from), int(to));
   }
}

void GraphIso::next_color(std::pair<Int, Int>& c)
{
   c.second = p_impl->n_colors++;
}

void GraphIso::copy_colors(const GraphIso& g1) {}

void GraphIso::set_node_color(Int i, std::pair<Int, Int>& c)
{
   // node indexes can't exceed the total node count checked during initial allocation,
   // therefore no further overflow checks
   if (p_impl->digraph) {
      bliss::Digraph* g = static_cast<bliss::Digraph*>(p_impl->src_graph);
      g->change_color(int(i), int(c.second));
   } else {
      bliss::Graph *g = static_cast<bliss::Graph *>(p_impl->src_graph);
      g->change_color(int(i), int(c.second));
   }
}

void GraphIso::partition(Int at)
{
   // node index can't exceed the total node count checked during initial allocation,
   // therefore no further overflow checks
   p_impl->set_color(0U, int(at), 0U);
   p_impl->set_color(int(at), p_impl->src_graph->get_nof_vertices(), 1U);
}

void GraphIso::finalize(bool gather_automorphisms)
{
   bliss::Stats stats;
   size_t n = p_impl->src_graph->get_nof_vertices();
   const unsigned int *perm;
   if (gather_automorphisms) {
      n_autom = 0;
#if BLISS_VERSION_MAJOR > 0 || BLISS_VERSION_MINOR >= 76
      auto aut_fun = [this](unsigned int nn, const unsigned int* aut) {
         impl::store_autom(this, nn, aut);
      };
      perm = p_impl->src_graph->canonical_form(stats, aut_fun);
#else
      perm = p_impl->src_graph->canonical_form(stats, &impl::store_autom, this);
#endif
   } else {
      perm = p_impl->src_graph->canonical_form(stats, nullptr, nullptr);
   }
   p_impl->canon_graph = p_impl->src_graph->permute(perm);
   std::copy(perm, perm+n, p_impl->canon_labels);
}

bool GraphIso::operator== (const GraphIso& g2) const
{
   if (p_impl->digraph != g2.p_impl->digraph)
      return false;
   
   if (!p_impl->canon_graph)
      throw no_match("no canon_graph in p_impl");
   if (!g2.p_impl->canon_graph)
      throw no_match("no canon_graph in g2.p_impl");
   if (p_impl->digraph) {
      bliss::Digraph* d1 = static_cast<bliss::Digraph *>(p_impl->canon_graph);
      bliss::Digraph* d2 = static_cast<bliss::Digraph *>(g2.p_impl->canon_graph);
      return d1->cmp(*d2) == 0;
   } else {
      bliss::Graph* gg1 = static_cast<bliss::Graph *>(p_impl->canon_graph);
      bliss::Graph* gg2 = static_cast<bliss::Graph *>(g2.p_impl->canon_graph);
      return gg1->cmp(*gg2) == 0;
   }
}

optional<Array<Int>>
GraphIso::find_permutation(const GraphIso& g2) const
{
   if (*this != g2)
      return nullopt;
  
   const int n = p_impl->src_graph->get_nof_vertices();
   unsigned int* invCanonLabels = new unsigned int[n];
   for (int i = 0; i < n; ++i)
      invCanonLabels[p_impl->canon_labels[i]] = i;

   Array<Int> perm(n);
   for (int i = 0; i < n; ++i)
      perm[i] = invCanonLabels[ g2.p_impl->canon_labels[i] ];
   
   delete[] invCanonLabels;
   return make_optional(std::move(perm));
}

optional<std::pair<Array<Int>, Array<Int>>>
GraphIso::find_permutations(const GraphIso& g2, const Int n_cols_) const
{
   if (*this != g2)
      return nullopt;
   if (n_cols_ > std::numeric_limits<int>::max())
      throw std::runtime_error("Graph with more than 2^31 nodes is too big for bliss");
   const int n_cols = int(n_cols_);

   const int n = p_impl->src_graph->get_nof_vertices();
   unsigned int* invCanonLabels = new unsigned int[n];
   for (int i = 0; i < n; ++i)
      invCanonLabels[p_impl->canon_labels[i]] = i;

   Array<Int> row_perm(n-n_cols), col_perm(n_cols);
   for (int i = 0; i < n_cols; ++i) 
      col_perm[i] = invCanonLabels[ g2.p_impl->canon_labels[i] ];
   
   for (int i = n_cols; i < n; ++i) 
      row_perm[i - n_cols] = invCanonLabels[ g2.p_impl->canon_labels[i] ] - n_cols;
   
   delete[] invCanonLabels;

   return make_optional(std::make_pair(std::move(row_perm), std::move(col_perm)));
}

Array<Int> GraphIso::canonical_perm() const
{
   const Array<Int> perm(p_impl->src_graph->get_nof_vertices(), p_impl->canon_labels);
   Array<Int> iperm(perm.size());
   // the canonical labels from bliss are an inverse permutation for the nodes
   inverse_permutation(perm, iperm);
   return iperm;
}

long GraphIso::hash(long key) const
{
   // the key is ignored here but needed for compatibility with nauty
   return p_impl->canon_graph->get_hash();
}

} }

// Local Variables:
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
