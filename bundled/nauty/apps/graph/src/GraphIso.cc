/* Copyright (c) 1997-2015
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

#define graph nauty_graph
#define set nauty_set
#define permutation nauty_permutation

#include <nauty.h>

namespace {
inline nauty_set* graph_row(nauty_graph *g, int v, int m) { return GRAPHROW(g,v,m); }
}

#undef GRAPHROW
#undef graph
#undef set
#undef permutation

#include "polymake/graph/GraphIso.h"

namespace polymake { namespace graph {
namespace {

DEFAULTOPTIONS_GRAPH(default_options);
GraphIso *in_processing=0;

}

struct GraphIso::impl {
   int n, m, n_colored, is_second;
   nauty_graph *src_graph, *canon_graph;
   int *orbits, *canon_labels, *partitions;
   optionblk options;

   explicit impl(int n_arg)
      : n(n_arg)
      , m((n + WORDSIZE - 1) / WORDSIZE)
      , n_colored(0)
      , is_second(0)
      , src_graph(0)
      , canon_graph(0)
      , orbits(0)
      , canon_labels(0)
      , partitions(0)
   {
      src_graph=new nauty_graph[n*m];
      EMPTYSET(src_graph,n*m);
      canon_graph=new nauty_graph[n*m];
      orbits=new int[n];
      canon_labels=new int[n];
      partitions=new int[n];
      options=default_options;
   }

   ~impl()
   {
      delete[] partitions;
      delete[] canon_labels;
      delete[] orbits;
      delete[] canon_graph;
      delete[] src_graph;
   }

   static void store_autom(int n_autom, nauty_permutation *perm, int*, int, int, int n)
   {
      in_processing->n_autom=n_autom;
      in_processing->autom.push_back( Array<int>(n, perm) );
   }
};

GraphIso::impl* GraphIso::alloc_impl(int n, bool dir, bool is_colored)
{
   impl* i=new impl(n);
   i->options.digraph=dir;
   i->options.getcanon=true;
   i->options.defaultptn=!is_colored;
   return i;
}

GraphIso::~GraphIso() { delete p_impl; }

void GraphIso::add_edge(int from, int to)
{
   nauty_set *gv=graph_row(p_impl->src_graph, from, p_impl->m);
   ADDELEMENT(gv, to);
}

void GraphIso::partition(int at)
{
   p_impl->options.defaultptn=false;
   std::fill(p_impl->partitions, p_impl->partitions+p_impl->n-1, 1);
   copy(entire(sequence(0,p_impl->n)), p_impl->canon_labels);
   p_impl->partitions[at-1]=0;
   p_impl->partitions[p_impl->n-1]=0;
}

void GraphIso::finalize(bool gather_automorphisms)
{
   statsblk stats;
   const int worksize=100*1024*1024;    // 100MB
   nauty_set *workspace=new nauty_set[worksize];
   if (gather_automorphisms) {
      p_impl->options.userautomproc=&impl::store_autom;
      in_processing=this;
   }
   nauty(p_impl->src_graph, p_impl->canon_labels, p_impl->partitions, 0, p_impl->orbits,
         &p_impl->options, &stats, workspace, worksize, p_impl->m, p_impl->n, p_impl->canon_graph);
   delete[] workspace;
}

bool GraphIso::operator== (const GraphIso& g2) const
{
   if (p_impl->n != g2.p_impl->n) return false;
   nauty_set *cg1=p_impl->canon_graph, *cg1_end=cg1+p_impl->n*p_impl->m, *cg2=g2.p_impl->canon_graph;
   return std::equal(cg1, cg1_end, cg2);
}

Array<int> GraphIso::find_permutation(const GraphIso& g2) const
{
   if (*this != g2)
      throw no_match("not isomorphic");

   Array<int> perm(p_impl->n);
   for (int *dst=perm.begin(), *lab1=p_impl->canon_labels, *lab1_end=lab1+p_impl->n, *lab2=g2.p_impl->canon_labels;
        lab1<lab1_end; ++lab1, ++lab2)
      dst[*lab2]=*lab1;
   return perm;
}

std::pair< Array<int>, Array<int> >
GraphIso::find_permutations(const GraphIso& g2, int n_cols) const
{
   if (*this != g2)
      throw no_match("not isomorphic");

   Array<int> row_perm(p_impl->n-n_cols), col_perm(n_cols);

   int *lab1=p_impl->canon_labels, *lab1_end=lab1+n_cols, *lab2=g2.p_impl->canon_labels;
   for (int *dst=col_perm.begin(); lab1<lab1_end; ++lab1, ++lab2)
      dst[*lab2]=*lab1;

   lab1_end=p_impl->canon_labels+p_impl->n;
   for (int *dst=row_perm.begin(); lab1<lab1_end; ++lab1, ++lab2)
      dst[*lab2-n_cols]=*lab1-n_cols;

   return std::make_pair(row_perm, col_perm);
}

void GraphIso::next_color(std::pair<int, int>& c)
{
   c.second=p_impl->n_colored;
   std::fill(p_impl->partitions+p_impl->n_colored, p_impl->partitions+p_impl->n_colored+c.first-1, 1);
   p_impl->partitions[p_impl->n_colored+c.first-1]=0;
   p_impl->n_colored += c.first;
}

void GraphIso::copy_colors(const GraphIso& g1)
{
   p_impl->options.defaultptn=g1.p_impl->options.defaultptn;
   std::copy(g1.p_impl->partitions, g1.p_impl->partitions+g1.p_impl->n, p_impl->partitions);
   p_impl->is_second=-1;
}

void GraphIso::set_node_color(int i, std::pair<int, int>& c)
{
   p_impl->canon_labels[(c.second++) - (c.first & p_impl->is_second)]=i;
}

} }

// Local Variables:
// c-basic-offset:3
// c-basic-offset:3
// End:
