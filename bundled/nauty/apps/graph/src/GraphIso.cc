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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#define graph nauty_graph
#define set nauty_set
#define permutation nauty_permutation

#include <nauty/nauty.h>
#include <nauty/naututil.h>
#include <memory>

namespace {

nauty_set* graph_row(nauty_graph* g, int v, int m)
{
   return GRAPHROW(g, v, m);
}

}

#undef GRAPHROW
#undef graph
#undef set
#undef permutation

namespace polymake { namespace graph { namespace {

DEFAULTOPTIONS_GRAPH(default_options);

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

} } }

#include "polymake/graph/GraphIso.h"

namespace polymake { namespace graph {
namespace {

thread_local GraphIso* in_processing = nullptr;

}

struct GraphIso::impl {
   int n, m, n_colored, is_second;
   std::unique_ptr<nauty_graph[]> src_graph, canon_graph;
   std::unique_ptr<int[]> orbits, canon_labels, partitions;
   optionblk options;

   explicit impl(int n_arg)
      : n(n_arg)
      , m((n+WORDSIZE-1) / WORDSIZE)
      , n_colored(0)
      , is_second(0)
      , src_graph(std::make_unique<nauty_graph[]>(n*m))
      , canon_graph(std::make_unique<nauty_graph[]>(n*m))
      , orbits(std::make_unique<int[]>(n))
      , canon_labels(std::make_unique<int[]>(n))
      , partitions(std::make_unique<int[]>(n))
   {
      EMPTYSET(src_graph.get(), n*m);
      options=default_options;
   }

   static void store_autom(int n_autom, nauty_permutation *perm, int*, int, int, int n)
   {
      in_processing->n_autom = n_autom;
      in_processing->autom.push_back(Array<Int>(n, perm));
   }
};

GraphIso::impl* GraphIso::alloc_impl(Int n, bool dir, bool is_colored)
{
   if (n > std::numeric_limits<int>::max())
      throw std::runtime_error("Graph with more than 2^31 nodes is too big for nauty");
   impl* i = new impl(int(n));
   i->options.digraph = dir;
   i->options.getcanon = true;
   i->options.defaultptn = !is_colored;
   return i;
}

GraphIso::~GraphIso() { delete p_impl; }

void GraphIso::add_edge(Int from, Int to)
{
   // node indexes can't exceed the total node count checked during initial allocation,
   // therefore no further overflow checks
   nauty_set* gv = graph_row(p_impl->src_graph.get(), int(from), p_impl->m);
   ADDELEMENT(gv, int(to));
}

void GraphIso::partition(Int at)
{
   // node index can't exceed the total node count checked during initial allocation,
   // therefore no further overflow checks
   p_impl->options.defaultptn = false;
   std::fill(p_impl->partitions.get(), p_impl->partitions.get() + p_impl->n-1, 1);
   std::copy(pm::sequence_iterator<int, true>(0), pm::sequence_iterator<int, true>(p_impl->n), p_impl->canon_labels.get());
   p_impl->partitions[int(at)-1] = 0;
   p_impl->partitions[p_impl->n-1] = 0;
}

void GraphIso::finalize(bool gather_automorphisms)
{
   statsblk stats;
   const size_t worksize = 100*1024*1024;    // 100MB
   nauty_set* workspace = new nauty_set[worksize];
   if (gather_automorphisms) {
      p_impl->options.userautomproc = &impl::store_autom;
      in_processing = this;
   }
   nauty(p_impl->src_graph.get(), p_impl->canon_labels.get(), p_impl->partitions.get(), nullptr, p_impl->orbits.get(),
         &p_impl->options, &stats, workspace, worksize, p_impl->m, p_impl->n, p_impl->canon_graph.get());
   delete[] workspace;
}

bool GraphIso::operator== (const GraphIso& g2) const
{
   if (p_impl->n != g2.p_impl->n) return false;
   nauty_set *cg1=p_impl->canon_graph.get(), *cg1_end=cg1+p_impl->n*p_impl->m, *cg2=g2.p_impl->canon_graph.get();
   return std::equal(cg1, cg1_end, cg2);
}

optional<Array<Int>>
GraphIso::find_permutation(const GraphIso& g2) const
{
   if (*this != g2)
      return nullopt;

   Array<Int> perm(p_impl->n);
   auto dst = perm.begin();
   for (int *lab1 = p_impl->canon_labels.get(), *const lab1_end = lab1 + p_impl->n, *lab2 = g2.p_impl->canon_labels.get();
        lab1 != lab1_end; ++lab1, ++lab2)
      dst[*lab2] = *lab1;
   return make_optional(std::move(perm));
}

optional<std::pair<Array<Int>, Array<Int>>>
GraphIso::find_permutations(const GraphIso& g2, const Int n_cols_) const
{
   if (*this != g2)
      return nullopt;
   if (n_cols_ > std::numeric_limits<int>::max())
      throw std::runtime_error("Graph with more than 2^31 nodes is too big for nauty");
   const int n_cols = int(n_cols_);
   Array<Int> row_perm(p_impl->n - n_cols), col_perm(n_cols);

   auto dst = col_perm.begin();
   int *lab1 = p_impl->canon_labels.get(), *lab1_end = lab1 + n_cols, *lab2 = g2.p_impl->canon_labels.get();
   for (; lab1 != lab1_end; ++lab1, ++lab2)
      dst[*lab2] = *lab1;

   dst = row_perm.begin();
   lab1_end = p_impl->canon_labels.get() + p_impl->n;
   for (; lab1 < lab1_end; ++lab1, ++lab2)
      dst[*lab2 - n_cols] = *lab1 - n_cols;

   return make_optional(std::make_pair(std::move(row_perm), std::move(col_perm)));
}

void GraphIso::next_color(std::pair<Int, Int>& c)
{
   // colored node counts can't exceed the total node count checked during initial allocation,
   // therefore no further overflow checks
   c.second = p_impl->n_colored;
   std::fill(p_impl->partitions.get()+p_impl->n_colored, p_impl->partitions.get()+p_impl->n_colored+c.first-1, 1);
   p_impl->partitions[p_impl->n_colored+c.first-1] = 0;
   p_impl->n_colored += int(c.first);
}

void GraphIso::copy_colors(const GraphIso& g1)
{
   p_impl->options.defaultptn = g1.p_impl->options.defaultptn;
   std::copy(g1.p_impl->partitions.get(), g1.p_impl->partitions.get() + g1.p_impl->n, p_impl->partitions.get());
   p_impl->is_second = -1;
}

void GraphIso::set_node_color(Int i, std::pair<Int, Int>& c)
{
   p_impl->canon_labels[(c.second++) - (c.first & p_impl->is_second)] = int(i);
}

Array<Int> GraphIso::canonical_perm() const
{
   Array<Int> perm(p_impl->n, p_impl->canon_labels.get());
   return perm;
}

long GraphIso::hash(long key) const
{
   return hashgraph(p_impl->canon_graph.get(), p_impl->m, p_impl->n, key);
}

} }

// Local Variables:
// c-basic-offset:3
// c-basic-offset:3
// End:
