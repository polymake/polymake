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

#ifndef POLYMAKE_GRAPH_GRAPHISO_H
#define POLYMAKE_GRAPH_GRAPHISO_H

#include "polymake/GenericGraph.h"
#include "polymake/GenericIncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/vector"
#include "polymake/list"
#include "polymake/optional"

namespace polymake { namespace graph {

//! An opaque graph representation for libraries checking for isomorphism (nauty, bliss)
class GraphIso {
   struct impl;
   impl *p_impl;

   Int n_autom;
   std::list<Array<Int>> autom;

   static impl *alloc_impl(Int n_nodes, bool is_directed, bool is_colored = false);
   void add_edge(Int from, Int to);
   void partition(Int at);
   void next_color(std::pair<Int, Int>& c);
   void copy_colors(const GraphIso& g1);
   void set_node_color(Int i, std::pair<Int, Int>& c);
   void finalize(bool gather_automorphisms);

   template <typename TMatrix>
   void fill(const GenericIncidenceMatrix<TMatrix>& M)
   {
      for (auto r=entire(rows(M));  !r.at_end();  ++r)
         for (auto c=entire(*r);  !c.at_end();  ++c)
            add_edge(r.index(), *c);
   }

   template <typename TMatrix, typename Iterator>
   void fill_renumbered(const GenericIncidenceMatrix<TMatrix>& M, Int d, Iterator iit)
   {
      std::vector<Int> renumber(d);
      for (Int i = 0; !iit.at_end(); ++iit, ++i)
         renumber[iit.index()]=i;
      for (auto r=entire(rows(M));  !r.at_end();  ++r)
         for (auto c=entire(*r);  !c.at_end();  ++c)
            add_edge(renumber[r.index()], renumber[*c]);
   }

   template <typename TGraph>
   void fill(const GenericGraph<TGraph>& G)
   {
      if (G.top().has_gaps())
         fill_renumbered(adjacency_matrix(G), G.top().dim(), entire(nodes(G)));
      else
         fill(adjacency_matrix(G));
   }

public:
   GraphIso() 
      : p_impl(nullptr)
      , n_autom(0) {}

   template <typename TGraph>
   explicit GraphIso(const GenericGraph<TGraph>& G, bool gather_automorphisms=false)
      : p_impl(alloc_impl(G.nodes(), G.is_directed))
      , n_autom(0)
   {
      fill(G);
      finalize(gather_automorphisms);
   }

   // non-symmetrical incidence matrix
   template <typename TMatrix>
   explicit GraphIso(const GenericIncidenceMatrix<TMatrix>& M,
                     typename std::enable_if<!TMatrix::is_symmetric, bool>::type gather_automorphisms=false)
      : p_impl(alloc_impl(M.rows()+M.cols(), false))
      , n_autom(0)
   {
      if (Int rnode = M.cols()) {
         partition(rnode);
         for (auto r = entire(rows(M));  !r.at_end();  ++r, ++rnode)
            for (auto c = entire(*r);  !c.at_end();  ++c) {
               add_edge(rnode, *c);
               add_edge(*c, rnode);
            }
      }
      finalize(gather_automorphisms);
   }

   // symmetrical incidence matrix
   template <typename TMatrix>
   explicit GraphIso(const GenericIncidenceMatrix<TMatrix>& M,
                     typename std::enable_if<TMatrix::is_symmetric, bool>::type gather_automorphisms=false)
      : p_impl(alloc_impl(M.rows(), true))
      , n_autom(0)
   {
      // there can be ones on the diagonal, which correspond to the loops in the graph;
      // nauty and bliss require the graph to be declared as directed in this case
      fill(M);
      finalize(gather_automorphisms);
   }

   ~GraphIso();

   bool operator== (const GraphIso& g2) const;
   bool operator!= (const GraphIso& g2) const { return !operator==(g2); }

private:
   static void incr_color_count(std::pair<Int, Int>& p)
   {
      ++p.first; ++p.second;
   }

public:
   template <typename TGraph1, typename Colors1, typename TGraph2, typename Colors2>
   static bool prepare_colored(GraphIso& GI1, const GenericGraph<TGraph1>& G1, const Colors1& colors1,
                               GraphIso& GI2, const GenericGraph<TGraph2>& G2, const Colors2& colors2);
   template <typename TGraph, typename Colors>
   static bool prepare_colored(GraphIso& GI, const GenericGraph<TGraph>& G, const Colors& colors);

   optional<Array<Int>> find_permutation(const GraphIso& g2) const;
   optional<std::pair<Array<Int>, Array<Int>>> find_permutations(const GraphIso& g2, Int n_cols) const;

   Int n_automorphisms() const { return n_autom; }
   const std::list<Array<Int>>& automorphisms() const { return autom; }

   Array<Int> canonical_perm() const;
   long hash(long key) const;
};

template <typename TGraph1, typename TGraph2>
bool isomorphic(const GenericGraph<TGraph1>& G1, const GenericGraph<TGraph2>& G2)
{
   if (G1.is_directed != G2.is_directed || G1.nodes() != G2.nodes())
      return false;
   if (G1.nodes() <= 1)
      return true;
   GraphIso GI1(G1), GI2(G2);
   return GI1==GI2;
}

template <typename TGraph1, typename Colors1, typename TGraph2, typename Colors2>
std::enable_if_t<std::is_same<typename Colors1::value_type, typename Colors2::value_type>::value, bool>
isomorphic(const GenericGraph<TGraph1>& G1, const Colors1& colors1,
           const GenericGraph<TGraph2>& G2, const Colors2& colors2)
{
   if (G1.is_directed != G2.is_directed || G1.nodes() != G2.nodes())
      return false;
   if (G1.nodes() <= 1)
      return G1.nodes()==0 || colors1.front()==colors2.front();
   GraphIso GI1, GI2;
   return GraphIso::prepare_colored(GI1, G1, colors1, GI2, G2, colors2) && GI1==GI2;
}

template <typename TGraph1, typename TGraph2, typename = std::enable_if_t<TGraph1::is_directed == TGraph2::is_directed>>
optional<Array<Int>>
find_node_permutation(const GenericGraph<TGraph1>& G1, const GenericGraph<TGraph2>& G2)
{
   if (G1.nodes() != G2.nodes())
      return nullopt;
   if (G1.nodes() <= 1)
      return polymake::make_optional(Array<Int>(G1.nodes(), 0));
   GraphIso GI1(G1), GI2(G2);
   return GI1.find_permutation(GI2);
}

template <typename TGraph1, typename Colors1, typename TGraph2, typename Colors2,
          typename = std::enable_if_t<(TGraph1::is_directed == TGraph2::is_directed &&
                                       std::is_same<typename Colors1::value_type, typename Colors2::value_type>::value)>>
optional<Array<Int>>
find_node_permutation(const GenericGraph<TGraph1>& G1, const Colors1& colors1,
                      const GenericGraph<TGraph2>& G2, const Colors2& colors2)
{
   if (G1.nodes() != G2.nodes())
      return nullopt;
   if (G1.nodes() <= 1) {
      if (G1.nodes() == 1 && colors1.front() != colors2.front())
         return nullopt;
      return polymake::make_optional(Array<Int>(G1.nodes(), 0));
   }
   GraphIso GI1, GI2;
   if (GraphIso::prepare_colored(GI1, G1, colors1, GI2, G2, colors2))
      return GI1.find_permutation(GI2);
   else
      return nullopt;
}

template <typename TGraph>
Array<Array<Int>> automorphisms(const GenericGraph<TGraph>& G)
{
   GraphIso GI(G, true);
   return Array<Array<Int>>(GI.n_automorphisms(), GI.automorphisms().begin());
}

template <typename TGraph, typename Colors>
Array<Array<Int>> automorphisms(const GenericGraph<TGraph>& G, const Colors& colors)
{
   GraphIso GI;
   GraphIso::prepare_colored(GI, G, colors);
   return Array<Array<Int>>(GI.n_automorphisms(), GI.automorphisms().begin());
}

template <typename TGraph>
Int n_automorphisms(const GenericGraph<TGraph>& G)
{
   GraphIso GI(G, true);
   return GI.n_automorphisms();
}

template <typename TGraph, typename Colors>
Int n_automorphisms(const GenericGraph<TGraph>& G, const Colors& colors)
{
   GraphIso GI;
   GraphIso::prepare_colored(GI,G,colors);
   return GI.n_automorphisms();
}

template <typename TMatrix1, typename TMatrix2>
std::enable_if_t<TMatrix1::is_symmetric == TMatrix2::is_symmetric, bool>
isomorphic(const GenericIncidenceMatrix<TMatrix1>& M1, const GenericIncidenceMatrix<TMatrix2>& M2)
{
   if (M1.rows() != M2.rows() || (!TMatrix1::is_symmetric && M1.cols() != M2.cols()))
      return false;
   if (M1.rows() == 0 || (!TMatrix1::is_symmetric && M1.cols() == 0))
      return true;
   GraphIso GI1(M1), GI2(M2);
   return GI1==GI2;
}

template <typename TMatrix1, typename TMatrix2>
std::enable_if_t<TMatrix1::is_symmetric && TMatrix2::is_symmetric,
                 optional<Array<Int>>>
find_row_permutation(const GenericIncidenceMatrix<TMatrix1>& M1, const GenericIncidenceMatrix<TMatrix2>& M2)
{
   if (M1.rows() != M2.rows())
      return nullopt;
   if (M1.rows() == 0)
      return polymake::make_optional(Array<Int>());
   GraphIso GI1(M1), GI2(M2);
   return GI1.find_permutation(GI2);
}

template <typename TMatrix1, typename TMatrix2>
std::enable_if_t<!TMatrix1::is_symmetric && !TMatrix2::is_symmetric,
                 optional<std::pair<Array<Int>, Array<Int>>>>
find_row_col_permutation(const GenericIncidenceMatrix<TMatrix1>& M1, const GenericIncidenceMatrix<TMatrix2>& M2)
{
   if (M1.rows() != M2.rows() || M1.cols() != M2.cols())
      return nullopt;
   if (M1.rows() == 0 && M1.cols() == 0)
      return polymake::make_optional(std::pair<Array<Int>, Array<Int>>());
   GraphIso GI1(M1), GI2(M2);
   return GI1.find_permutations(GI2, M1.cols());
}

template <typename TMatrix>
std::enable_if_t<TMatrix::is_symmetric, Array<Array<Int>>>
automorphisms(const GenericIncidenceMatrix<TMatrix>& M)
{
   GraphIso GI(M, true);
   return Array<Array<Int>>(GI.n_automorphisms(), GI.automorphisms().begin());
}

template <typename Matrix>
typename std::enable_if<!Matrix::is_symmetric, Array<std::pair<Array<Int>, Array<Int>>>>::type
automorphisms(const GenericIncidenceMatrix<Matrix>& M)
{
   GraphIso GI(M, true);
   Array<std::pair<Array<Int>, Array<Int>>> result(GI.n_automorphisms());
   auto p = GI.automorphisms().begin();
   const Int n_rows = M.rows();
   const Int n_cols = M.cols();
   sequence rows(n_cols, n_rows), cols(0, n_cols);

   for (auto r=entire(result); !r.at_end(); ++r, ++p) {
      r->first.append(n_rows, translate(select(*p,rows), -n_cols).begin());
      r->second.append(n_cols, select(*p,cols).begin());
   }
   return result;
}

template <typename TGraph1, typename Colors1, typename TGraph2, typename Colors2>
bool GraphIso::prepare_colored(GraphIso& GI1, const GenericGraph<TGraph1>& G1, const Colors1& colors1,
                               GraphIso& GI2, const GenericGraph<TGraph2>& G2, const Colors2& colors2)
{
   const Int n = G1.nodes();
   GI1.p_impl = alloc_impl(n, G1.is_directed, true);
   GI2.p_impl = alloc_impl(n, G2.is_directed, true);

   using color_map_type = Map<typename Colors1::value_type, std::pair<Int, Int>>;
   color_map_type color_map;

   // count the nodes per color in the first graph
   for (auto c = entire(colors1); !c.at_end(); ++c)
      incr_color_count(color_map[*c]);

   // compare the counts with the second graph; all must gown to zero, otherwise the graphs are not isomoprhic
   for (auto c = entire(colors2); !c.at_end(); ++c)
      if (--color_map[*c].second < 0)
         return false;

   for (auto cm = color_map.begin(); !cm.at_end(); ++cm)
      GI1.next_color(cm->second);

   GI2.copy_colors(GI1);

   for (auto c = entire<indexed>(colors1); !c.at_end(); ++c)
      GI1.set_node_color(c.index(), color_map[*c]);

   for (auto c = entire<indexed>(colors2); !c.at_end(); ++c)
      GI2.set_node_color(c.index(), color_map[*c]);

   GI1.fill(G1);  GI1.finalize(false);
   GI2.fill(G2);  GI2.finalize(false);
   return true;
}

template <typename TGraph, typename Colors>
bool GraphIso::prepare_colored(GraphIso& GI, const GenericGraph<TGraph>& G, const Colors& colors)
{
   const Int n = G.nodes();
   GI.p_impl = alloc_impl(n, G.is_directed, true);

   using color_map_type = Map<typename Colors::value_type, std::pair<Int, Int>>;
   color_map_type color_map;

   for (auto c = entire(colors); !c.at_end(); ++c)
      ++(color_map[*c].first);

   for (auto cm = color_map.begin(); !cm.at_end(); ++cm)
      GI.next_color(cm->second);

   for (auto c = entire<indexed>(colors); !c.at_end(); ++c)
      GI.set_node_color(c.index(), color_map[*c]);

   GI.fill(G);  GI.finalize(true);
   return true;
}

template <typename TGraph>
typename TGraph::persistent_type canonical_form(const GenericGraph<TGraph>& G)
{
   if (G.nodes() <= 1)
      return G;
   GraphIso GI(G);
   if (G.top().has_gaps())
      return permuted_nodes(renumber_nodes(G),GI.canonical_perm());
   else
      return permuted_nodes(G, GI.canonical_perm());
}

template <typename TGraph>
long canonical_hash(const GenericGraph<TGraph>& G, long k)
{
   GraphIso GI(G);
   return GI.hash(k);
}

template <typename TMatrix>
long canonical_hash(const GenericIncidenceMatrix<TMatrix>& M, long k)
{
   GraphIso GI(M);
   return GI.hash(k);
}

} }

#endif // POLYMAKE_GRAPH_GRAPHISO_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
