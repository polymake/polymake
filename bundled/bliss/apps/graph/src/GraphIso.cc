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

#include <cstring>
#include <cstddef> // needed for gcc 4.9, see http://gcc.gnu.org/gcc-4.9/porting_to.html
#include <bliss/graph.hh>
#include "polymake/graph/GraphIso.h"
#include "polymake/permutations.h"

namespace polymake { namespace graph {

struct GraphIso::impl {
   bliss::AbstractGraph *src_graph;
   bliss::AbstractGraph *canon_graph;
   unsigned int *canon_labels;
   int n_colors;
   bool digraph;

   impl(int n_arg, bool dir)
      : n_colors(0)
      , digraph(dir)
   {
      if (dir)
         src_graph = new bliss::Digraph(n_arg);
      else
         src_graph = new bliss::Graph(n_arg);
      canon_labels=new unsigned int[n_arg];
      canon_graph = 0;
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
         bliss::Digraph *g = reinterpret_cast<bliss::Digraph *>(src_graph);
         for (int i = from; i < to; i++)
            g->change_color(i, color);
      } else {
         bliss::Graph *g = reinterpret_cast<bliss::Graph *>(src_graph);
         for (int i = from; i < to; i++)
            g->change_color(i, color);
      }
   }

   static void store_autom(void *graph, unsigned int n, const unsigned int *aut)
   {
      GraphIso *g=reinterpret_cast<GraphIso *>(graph);
      g->n_autom++;
      g->autom.push_back( Array<int>(n, aut) );
   }
};

GraphIso::impl* GraphIso::alloc_impl(int n, bool dir, bool is_colored)
{
   return new impl(n, dir);
}

GraphIso::~GraphIso() { delete p_impl; }

void GraphIso::add_edge(int from, int to)
{
   if (p_impl->digraph) {
      bliss::Digraph *g = reinterpret_cast<bliss::Digraph *>(p_impl->src_graph);
      g->add_edge(from, to);
   } else {
      bliss::Graph *g = reinterpret_cast<bliss::Graph *>(p_impl->src_graph);
      g->add_edge(from, to);
   }
}

void GraphIso::next_color(std::pair<int, int>& c)
{
   c.second=p_impl->n_colors++;
}

void GraphIso::copy_colors(const GraphIso& g1) {}

void GraphIso::set_node_color(int i, std::pair<int, int>& c)
{
   if (p_impl->digraph) {
      bliss::Digraph *g = reinterpret_cast<bliss::Digraph *>(p_impl->src_graph);
      g->change_color(i, c.second);
   } else {
      bliss::Graph *g = reinterpret_cast<bliss::Graph *>(p_impl->src_graph);
      g->change_color(i, c.second);
   }
}

void GraphIso::partition(int at)
{
   p_impl->set_color(0U, at, 0U);
   p_impl->set_color(at, p_impl->src_graph->get_nof_vertices(), 1U);
}

void GraphIso::finalize(bool gather_automorphisms)
{
   bliss::Stats stats;
   size_t n=p_impl->src_graph->get_nof_vertices();
   const unsigned int *perm;
   if (gather_automorphisms) {
      n_autom=0;
      perm=p_impl->src_graph->canonical_form(stats, &impl::store_autom, this);
   } else {
      perm=p_impl->src_graph->canonical_form(stats, NULL, NULL);
   }
   p_impl->canon_graph = p_impl->src_graph->permute(perm);
   std::memcpy(p_impl->canon_labels, perm, n * sizeof(int));
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
      bliss::Digraph *d1 = reinterpret_cast<bliss::Digraph *>(p_impl->canon_graph);
      bliss::Digraph *d2 = reinterpret_cast<bliss::Digraph *>(g2.p_impl->canon_graph);
      return d1->cmp(*d2) == 0;
   } else {
      bliss::Graph *gg1 = reinterpret_cast<bliss::Graph *>(p_impl->canon_graph);
      bliss::Graph *gg2 = reinterpret_cast<bliss::Graph *>(g2.p_impl->canon_graph);
      return gg1->cmp(*gg2) == 0;
   }
}

optional<Array<int>>
GraphIso::find_permutation(const GraphIso& g2) const
{
   if (*this != g2)
      return nullopt;
  
   const int n = p_impl->src_graph->get_nof_vertices();
   unsigned int* invCanonLabels = new unsigned int[n];
   for (int i = 0; i < n; ++i)
      invCanonLabels[p_impl->canon_labels[i]] = i;

   Array<int> perm(n);
   for (int i = 0; i < n; ++i)
      perm[i] = invCanonLabels[ g2.p_impl->canon_labels[i] ];
   
   delete[] invCanonLabels;
   return make_optional(std::move(perm));
}

optional<std::pair<Array<int>, Array<int>>>
GraphIso::find_permutations(const GraphIso& g2, int n_cols) const
{
   if (*this != g2)
      return nullopt;
   
   const int n = p_impl->src_graph->get_nof_vertices();
   unsigned int* invCanonLabels = new unsigned int[n];
   for (int i = 0; i < n; ++i)
      invCanonLabels[p_impl->canon_labels[i]] = i;

   Array<int> row_perm(n-n_cols), col_perm(n_cols);
   for (int i = 0; i < n_cols; ++i) 
      col_perm[i] = invCanonLabels[ g2.p_impl->canon_labels[i] ];
   
   for (int i = n_cols; i < n; ++i) 
      row_perm[i - n_cols] = invCanonLabels[ g2.p_impl->canon_labels[i] ] - n_cols;
   
   delete[] invCanonLabels;

   return make_optional(std::make_pair(std::move(row_perm), std::move(col_perm)));
}

Array<int> GraphIso::canonical_perm() const
{
   const Array<int> perm(p_impl->src_graph->get_nof_vertices(), p_impl->canon_labels);
   Array<int> iperm(perm.size());
   // the canonical labels from bliss are an inverse permutation for the nodes
   inverse_permutation(perm,iperm);
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
