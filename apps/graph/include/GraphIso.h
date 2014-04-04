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

#ifndef POLYMAKE_GRAPH_GRAPHISO_H
#define POLYMAKE_GRAPH_GRAPHISO_H

#include "polymake/GenericGraph.h"
#include "polymake/GenericIncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/vector"
#include "polymake/list"

namespace polymake { namespace graph {

//! An opaque graph representation for libraries chacking for isomorphism (nauty, bliss)
class GraphIso {
   struct impl;
   impl *p_impl;

   int n_autom;
   std::list< Array<int> > autom;

   static impl *alloc_impl(int n_nodes, bool is_directed, bool is_colored=false);
   void add_edge(int from, int to);
   void partition(int at);
   void next_color(std::pair<int, int>& c);
   void copy_colors(const GraphIso& g1);
   void set_node_color(int i, std::pair<int, int>& c);
   void finalize(bool gather_automorphisms);

   template <typename Matrix>
   void fill(const GenericIncidenceMatrix<Matrix>& M)
   {
      for (typename Entire< Rows< Matrix > >::const_iterator r=entire(rows(M));  !r.at_end();  ++r)
         for (typename Entire< typename Matrix::row_type >::const_iterator c=entire(*r);  !c.at_end();  ++c)
            add_edge(r.index(), *c);
   }

   template <typename Matrix, typename Iterator>
   void fill_renumbered(const GenericIncidenceMatrix<Matrix>& M, int d, Iterator iit)
   {
      std::vector<int> renumber(d);
      for (int i=0; !iit.at_end(); ++iit, ++i)
         renumber[iit.index()]=i;
      for (typename Entire< Rows< Matrix > >::const_iterator r=entire(rows(M));  !r.at_end();  ++r)
         for (typename Entire< typename Matrix::row_type >::const_iterator c=entire(*r);  !c.at_end();  ++c)
            add_edge(renumber[r.index()], renumber[*c]);
   }

   template <typename Graph>
   void fill(const GenericGraph<Graph>& G)
   {
      if (G.top().has_gaps())
         fill_renumbered(adjacency_matrix(G), G.top().dim(), entire(nodes(G)));
      else
         fill(adjacency_matrix(G));
   }

public:
   GraphIso() 
      : p_impl(0)
      , n_autom(0) {}

   template <typename Graph>
   explicit GraphIso(const GenericGraph<Graph>& G, bool gather_automorphisms=false)
      : p_impl(alloc_impl(G.nodes(), G.is_directed))
      , n_autom(0)
   {
      fill(G);
      finalize(gather_automorphisms);
   }

   // non-symmetrical incidence matrix
   template <typename Matrix>
   explicit GraphIso(const GenericIncidenceMatrix<Matrix>& M,
                     typename pm::disable_if<bool, Matrix::is_symmetric>::type gather_automorphisms=false)
      : p_impl(alloc_impl(M.rows()+M.cols(), false))
      , n_autom(0)
   {
      int rnode=M.cols();
      partition(rnode);
      for (typename Entire< Rows< Matrix > >::const_iterator r=entire(rows(M));  !r.at_end();  ++r, ++rnode)
         for (typename Entire< typename Matrix::row_type >::const_iterator c=entire(*r);  !c.at_end();  ++c) {
            add_edge(rnode, *c);
            add_edge(*c, rnode);
         }
      finalize(gather_automorphisms);
   }

   // symmetrical incidence matrix
   template <typename Matrix>
   explicit GraphIso(const GenericIncidenceMatrix<Matrix>& M,
                     typename pm::enable_if<bool, Matrix::is_symmetric>::type gather_automorphisms=false)
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
   static void incr_color_count(std::pair<int,int>& p)
   {
      ++p.first; ++p.second;
   }

public:
   template <typename Graph1, typename Colors1, typename Graph2, typename Colors2>
   static bool prepare_colored(GraphIso& GI1, const GenericGraph<Graph1>& G1, const Colors1& colors1,
                               GraphIso& GI2, const GenericGraph<Graph2>& G2, const Colors2& colors2);
   template <typename Graph, typename Colors>
   static bool prepare_colored(GraphIso& GI, const GenericGraph<Graph>& G, const Colors& colors);

   Array<int> find_permutation(const GraphIso& g2) const;
   std::pair< Array<int>, Array<int> > find_permutations(const GraphIso& g2, int n_cols) const;

   int n_automorphisms() const { return n_autom; }
   const std::list< Array<int> >& automorphisms() const { return autom; }
};

template <typename Graph1, typename Graph2> inline
bool isomorphic(const GenericGraph<Graph1>& G1, const GenericGraph<Graph2>& G2)
{
   if (G1.is_directed != G2.is_directed || G1.nodes() != G2.nodes())
      return false;
   if (G1.nodes()<=1) return true;
   GraphIso GI1(G1), GI2(G2);
   return GI1==GI2;
}

template <typename Graph1, typename Colors1, typename Graph2, typename Colors2> inline
typename pm::enable_if<bool, pm::identical<typename Colors1::value_type, typename Colors2::value_type>::value>::type
isomorphic(const GenericGraph<Graph1>& G1, const Colors1& colors1,
           const GenericGraph<Graph2>& G2, const Colors2& colors2)
{
   if (G1.is_directed != G2.is_directed || G1.nodes() != G2.nodes())
      return false;
   if (G1.nodes()==0 || (G1.nodes()==1 && colors1.front()==colors2.front())) return true;
   GraphIso GI1, GI2;
   return GraphIso::prepare_colored(GI1, G1, colors1, GI2, G2, colors2) && GI1==GI2;
}

template <typename Graph1, typename Graph2> inline
Array<int> find_node_permutation(const GenericGraph<Graph1>& G1, const GenericGraph<Graph2>& G2)
{
   if (G1.is_directed != G2.is_directed)
      throw no_match("graphs of different kind");
   if (G1.nodes() != G2.nodes())
      throw no_match("graphs of different size");
   GraphIso GI1(G1), GI2(G2);
   return GI1.find_permutation(GI2);
}

template <typename Graph1, typename Colors1, typename Graph2, typename Colors2> inline
typename pm::enable_if<Array<int>, pm::identical<typename Colors1::value_type, typename Colors2::value_type>::value>::type
find_node_permutation(const GenericGraph<Graph1>& G1, const Colors1& colors1,
                      const GenericGraph<Graph2>& G2, const Colors2& colors2)
{
   if (G1.is_directed != G2.is_directed)
      throw no_match("graphs of different kind");
   if (G1.nodes() != G2.nodes())
      throw no_match("graphs of different size");
   GraphIso GI1, GI2;
   if (GraphIso::prepare_colored(GI1, G1, colors1, GI2, G2, colors2))
      return GI1.find_permutation(GI2);
   else
      throw no_match("different colors");
}

template <typename Graph> inline
Array< Array<int> > automorphisms(const GenericGraph<Graph>& G)
{
   GraphIso GI(G, true);
   return Array< Array<int> >(GI.n_automorphisms(), GI.automorphisms().begin());
}

template <typename Graph, typename Colors> inline
Array< Array<int> > automorphisms(const GenericGraph<Graph>& G, const Colors& colors)
{
   GraphIso GI;
   GraphIso::prepare_colored(GI,G,colors);
   return Array< Array<int> >(GI.n_automorphisms(), GI.automorphisms().begin());
}

template <typename Graph> inline
int n_automorphisms(const GenericGraph<Graph>& G)
{
   GraphIso GI(G, true);
   return GI.n_automorphisms();
}

template <typename Graph, typename Colors> inline
int n_automorphisms(const GenericGraph<Graph>& G, const Colors& colors)
{
   GraphIso GI;
   GraphIso::prepare_colored(GI,G,colors);
   return GI.n_automorphisms();
}

template <typename Matrix1, typename Matrix2> inline
typename pm::enable_if<bool, Matrix1::is_symmetric==Matrix2::is_symmetric>::type
isomorphic(const GenericIncidenceMatrix<Matrix1>& M1, const GenericIncidenceMatrix<Matrix2>& M2)
{
   if (M1.rows() != M2.rows() || M1.cols() != M2.cols())
      return false;
   if (M1.rows()<=1) return true;
   GraphIso GI1(M1), GI2(M2);
   return GI1==GI2;
}

template <typename Matrix1, typename Matrix2> inline
typename pm::enable_if< Array<int>, Matrix1::is_symmetric && Matrix2::is_symmetric>::type
find_row_permutation(const GenericIncidenceMatrix<Matrix1>& M1, const GenericIncidenceMatrix<Matrix2>& M2)
{
   if (M1.rows() != M2.rows())
      throw no_match("matrices of different dimensions");
   GraphIso GI1(M1), GI2(M2);
   return GI1.find_permutation(GI2);
}

template <typename Matrix1, typename Matrix2> inline
typename pm::enable_if< std::pair< Array<int>, Array<int> >, !Matrix1::is_symmetric && !Matrix2::is_symmetric>::type
find_row_col_permutation(const GenericIncidenceMatrix<Matrix1>& M1, const GenericIncidenceMatrix<Matrix2>& M2)
{
   if (M1.rows() != M2.rows() || M1.cols() != M2.cols())
      throw no_match("matrices of different dimensions");
   GraphIso GI1(M1), GI2(M2);
   return GI1.find_permutations(GI2, M1.cols());
}

template <typename Matrix> inline
typename pm::enable_if< Array< Array<int> >, Matrix::is_symmetric>::type
automorphisms(const GenericIncidenceMatrix<Matrix>& M)
{
   GraphIso GI(M, true);
   return Array< Array<int> >(GI.n_automorphisms(), GI.automorphisms().begin());
}

template <typename Matrix>
typename pm::disable_if< Array< std::pair< Array<int>, Array<int> > >, Matrix::is_symmetric>::type
automorphisms(const GenericIncidenceMatrix<Matrix>& M)
{
   GraphIso GI(M, true);
   Array< std::pair< Array<int>, Array<int> > > result(GI.n_automorphisms());
   std::list< Array<int> >::const_iterator p=GI.automorphisms().begin();
   const int n_rows=M.rows(), n_cols=M.cols();
   sequence rows(n_cols, n_rows), cols(0, n_cols);

   for (Entire< Array< std::pair< Array<int>, Array<int> > > >::iterator r=entire(result); !r.at_end(); ++r, ++p) {
      r->first.append(n_rows, translate(select(*p,rows), -n_cols).begin());
      r->second.append(n_cols, select(*p,cols).begin());
   }
   return result;
}

template <typename Graph1, typename Colors1, typename Graph2, typename Colors2>
bool GraphIso::prepare_colored(GraphIso& GI1, const GenericGraph<Graph1>& G1, const Colors1& colors1,
                               GraphIso& GI2, const GenericGraph<Graph2>& G2, const Colors2& colors2)
{
   const int n=G1.nodes();
   GI1.p_impl=alloc_impl(n, G1.is_directed, true);
   GI2.p_impl=alloc_impl(n, G2.is_directed, true);

   typedef Map<typename Colors1::value_type, std::pair<int, int> > color_map_type;
   color_map_type color_map;

   // count the nodes per color in the first graph
   for (typename Entire<Colors1>::const_iterator c=entire(colors1); !c.at_end(); ++c)
      incr_color_count(color_map[*c]);

   // compare the counts with the second graph; all must gown to zero, otherwise the graphs are not isomoprhic
   for (typename Entire<Colors2>::const_iterator c=entire(colors2); !c.at_end(); ++c)
      if (--color_map[*c].second < 0)
         return false;

   for (typename color_map_type::iterator cm=color_map.begin(); !cm.at_end(); ++cm)
      GI1.next_color(cm->second);

   GI2.copy_colors(GI1);

   for (typename pm::ensure_features<Colors1, pm::cons<pm::end_sensitive, pm::indexed> >::const_iterator
           c=ensure(colors1, (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); !c.at_end(); ++c)
      GI1.set_node_color(c.index(), color_map[*c]);

   for (typename pm::ensure_features<Colors2, pm::cons<pm::end_sensitive, pm::indexed> >::const_iterator
           c=ensure(colors2, (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); !c.at_end(); ++c)
      GI2.set_node_color(c.index(), color_map[*c]);

   GI1.fill(G1);  GI1.finalize(false);
   GI2.fill(G2);  GI2.finalize(false);
   return true;
}

template <typename Graph, typename Colors>
bool GraphIso::prepare_colored(GraphIso& GI, const GenericGraph<Graph>& G, const Colors& colors)
{
   const int n=G.nodes();
   GI.p_impl=alloc_impl(n, G.is_directed, true);

   typedef Map<typename Colors::value_type, std::pair<int, int> > color_map_type;
   color_map_type color_map;

   for (typename Entire<Colors>::const_iterator c=entire(colors); !c.at_end(); ++c)
      ++(color_map[*c].first);

   for (typename color_map_type::iterator cm=color_map.begin(); !cm.at_end(); ++cm)
      GI.next_color(cm->second);

   for (typename pm::ensure_features<Colors, pm::cons<pm::end_sensitive, pm::indexed> >::const_iterator
           c=ensure(colors, (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); !c.at_end(); ++c)
      GI.set_node_color(c.index(), color_map[*c]);

   GI.fill(G);  GI.finalize(true);
   return true;
}

} }

#endif // POLYMAKE_GRAPH_GRAPHISO_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
