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

#ifndef POLYMAKE_GRAPH_HASSE_DIAGRAM_H
#define POLYMAKE_GRAPH_HASSE_DIAGRAM_H

#include "polymake/client.h"
#include "polymake/Graph.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/vector"
#include <algorithm>

namespace polymake { namespace graph {

class HasseDiagram {
public:
   typedef Graph<Directed> graph_type;
   typedef NodeMap< Directed, Set<int> > faces_map_type;
protected:
   graph_type G;
   faces_map_type F;
   std::vector<int> dim_map, count_map;
   bool built_min_first;
public:
   bool built_dually() const { return !built_min_first; }

   HasseDiagram() : F(G) {}

   graph_type& graph() { return G; }
   const graph_type& graph() const { return G; }
   const faces_map_type& faces() const { return F; }
   const std::vector<int>& dims() const { return dim_map; }
   const std::vector<int>& dim_counts() const { return count_map; }

   const Set<int>& face(int n) const { return F[n]; }

   Array< Set<int> > dual_faces() const;

   int nodes() const { return G.nodes(); }
   int edges() const { return G.edges(); }

   friend const Nodes<graph_type>& nodes(const HasseDiagram& me) { return pm::nodes(me.G); }
   friend const Edges<graph_type>& edges(const HasseDiagram& me) { return pm::edges(me.G); }
   friend const AdjacencyMatrix<graph_type>& adjacency_matrix(const HasseDiagram& me) { return pm::adjacency_matrix(me.G); }

   bool node_exists(int n) const { return G.node_exists(n); }
   bool edge_exists(int n1, int n2) const { return G.edge_exists(n1,n2); }

   graph_type::const_out_edge_list_ref out_edges(int n) const { return G.out_edges(n); }
   graph_type::const_in_edge_list_ref in_edges(int n) const { return G.in_edges(n); }

   graph_type::const_out_adjacent_node_list_ref out_adjacent_nodes(int n) const { return G.out_adjacent_nodes(n); }
   graph_type::const_in_adjacent_node_list_ref in_adjacent_nodes(int n) const { return G.in_adjacent_nodes(n); }

   int out_degree(int n) const { return G.out_degree(n); }
   int in_degree(int n) const { return G.in_degree(n); }
   int degree(int n) const { return G.degree(n); }

   void clear()
   {
      G.clear();
      dim_map.clear();
   }

   int dim() const { return dim_map.size()-(built_dually() || !proper_top_node() ? 1 : 2); }

   struct node_exists_pred {
      const graph_type *G;

      node_exists_pred() : G(0) {}
      node_exists_pred(const graph_type& G_arg) : G(&G_arg) {}

      typedef int argument_type;
      typedef bool result_type;
      result_type operator() (int n) const { return G->node_exists(n); }
   };

   typedef SelectedSubset<sequence, node_exists_pred> range_with_gaps;
   typedef ContainerUnion< pm::cons<sequence, range_with_gaps> > nodes_of_dim_set;

   const sequence node_range_of_dim(int d) const
   {
      const int D=dim();
      if (d>=std::numeric_limits<int>::max()-D)
         throw std::runtime_error("HasseDiagram::nodes_of_dim - dimension out of range");
      if (d<0) d+=D;
      if (d<0 || d>D)
         throw std::runtime_error("HasseDiagram::nodes_of_dim - dimension out of range");
      if (d==D)
         return sequence(top_node(), 1);
      if (built_dually()) d=D-1-d;
      return range(dim_map[d], dim_map[d+1]-1);
   }

   const nodes_of_dim_set nodes_of_dim(int d) const
   {
      if (G.has_gaps()) return range_with_gaps(node_range_of_dim(d),G);
      return node_range_of_dim(d);
   }

   const sequence node_range_of_dim(int d1, int d2) const
   {
      const int D=dim();
      if (d1<0) d1+=D;
      if (d2<0) d2+=D;
      if (d1<0 || d2>D || d1>d2)
         throw std::runtime_error("HasseDiagram::nodes_of_dim - dimension out of range");
      if (d2==D) {
         if (built_dually())
            return range(0, dim_map[D-d1]-1);
         else
            return range(dim_map[d1], G.nodes()-1);
      }
      if (built_dually()) {
         const int _d1=d1; d1=D-1-d2; d2=D-1-_d1;
      }
      return range(dim_map[d1], dim_map[d2+1]-1);
   }

   const nodes_of_dim_set nodes_of_dim(int d1, int d2) const
   {
      if (G.has_gaps()) return range_with_gaps(node_range_of_dim(d1,d2),G);
      return node_range_of_dim(d1,d2);
   }

   int dim_of_node(int n) const
   {
      if (POLYMAKE_DEBUG) {
         if (G.invalid_node(n))
            throw std::runtime_error("HasseDiagram::dim_of_node - node id out of range or deleted");
      }
      const int d = std::upper_bound(dim_map.begin(), dim_map.end(), n) - dim_map.begin();
      return built_dually() ? dim()-d : d-1;
   }

   int bottom_node() const
   {
      return built_dually() ? G.nodes()-1 : 0;
   }

   int top_node() const
   {
      return built_dually() ? 0 : G.nodes()-1;
   }

   bool proper_top_node() const
   {
      const size_t d=dim_map.size()-1;
      return d==0 || dim_map[d]-dim_map[d-1]==1 && dim_map[d-1]==top_node();
   }

   typedef ContainerUnion< pm::cons< IndexedSubset<const faces_map_type&, const graph_type::in_adjacent_node_list&>,
                                     pm::single_value_container< const Set<int>& > > >
      max_faces_list;
   max_faces_list max_faces() const
   {
      if (proper_top_node())
         return item2container(F[top_node()]);
      else
         return select(F, G.in_adjacent_nodes(top_node()));
   }

protected:
   void update_dim_after_squeeze();
public:
   void delete_node(int n);

   void delete_node_and_squeeze(int n)
   {
      G.delete_node(n);
      for (std::vector<int>::iterator map_end=dim_map.end(), d=std::upper_bound(dim_map.begin(), map_end, n);
           d != map_end;  ++d)
         *d -= 1;
      G.squeeze();
      update_dim_after_squeeze();
   }

   template <typename SetTop>
   void delete_nodes(const GenericSet<SetTop>& nodes_to_delete)
   {
      for (typename Entire<SetTop>::const_iterator n=entire(nodes_to_delete.top()); !n.at_end(); ++n)
         delete_node(*n);
   }

   template <typename SetTop>
   void delete_nodes_and_squeeze(const GenericSet<SetTop>& nodes_to_delete)
   {
      if (nodes_to_delete.top().empty()) return;
      if (POLYMAKE_DEBUG || !pm::Unwary<SetTop>::value) {
         if (!set_within_range(nodes_to_delete, this->nodes()))
            throw std::runtime_error("HasseDiagram::delete_nodes - node numbers out of range");
      }

      typename Entire<SetTop>::const_iterator n=entire(nodes_to_delete.top());
      int cnt=0;
      typename std::vector<int>::iterator map_end=dim_map.end();
      for (typename std::vector<int>::iterator d=std::upper_bound(dim_map.begin(), map_end, *n);
           d != map_end;  ++d) {
         while (!n.at_end() && *n<*d) ++n, ++cnt;
         *d -= cnt;
      }
      for (n=entire(nodes_to_delete.top()); !n.at_end(); ++n) G.delete_node(*n);
      G.squeeze();
      update_dim_after_squeeze();
   }

   class _filler {
   protected:
      mutable HasseDiagram* HD;
   public:
      _filler(HasseDiagram& HD_arg, bool min_first) : HD(&HD_arg) { if (HD->nodes()) HD->clear(); HD->built_min_first=min_first;}

      _filler(const _filler& f) : HD(f.HD) { f.HD=0; }

      template <typename SetTop>
      int add_node(const GenericSet<SetTop>& vertex_set) const
      {
         int n=HD->G.nodes();
         HD->G.resize(n+1);
         HD->F[n]=vertex_set;
         return n;
      }

      template <typename Iterator>
      int add_nodes(int n, Iterator vertex_sets) const
      {
         int n_old=HD->G.nodes();
         HD->G.resize(n_old+n);
         for (Set<int> *s=&(HD->F[n_old]), *e=s+n; s<e; ++s, ++vertex_sets)
            *s=*vertex_sets;
         return n_old;
      }

      /// face n_from must be included in face n_to
      void add_edge(int n_from, int n_to) const { HD->G.edge(n_from, n_to); }

      void increase_dim() const { HD->next_layer(); }

      const graph_type& graph() const { return HD->G; }
      const faces_map_type& faces() const { return HD->F; }

      ~_filler() { if (HD) HD->G.resize(HD->G.nodes()); } // FIXME: w/o effect now!
   };

   friend _filler filler(HasseDiagram& me, bool min_first) {return _filler(me, min_first); }

   perl::Object makeObject() const;
   void fromObject(const perl::Object&);
   explicit HasseDiagram(const perl::Object& o) : F(G) { fromObject(o); }
   friend void operator<< (const perl::Value& v, const HasseDiagram& me);
   friend bool operator>> (const perl::Value& v, HasseDiagram& me);

protected:
   void next_layer()
   {
      dim_map.push_back(G.nodes());
   }
};
} }
namespace pm { namespace perl {
template <>
struct check_for_magic_storage<polymake::graph::HasseDiagram> : False {};
} }

#endif // POLYMAKE_GRAPH_HASSE_DIAGRAM_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
