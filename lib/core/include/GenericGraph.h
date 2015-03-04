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

/** @file GenericGraph.h
    @brief Implementation of pm::GenericGraph class
*/

#ifndef POLYMAKE_GENERIC_GRAPH_H
#define POLYMAKE_GENERIC_GRAPH_H

#include "polymake/GenericIncidenceMatrix.h"
#include "polymake/GenericMatrix.h"
#include "polymake/TransformedContainer.h"
#include "polymake/CascadedContainer.h"
#include "polymake/GenericIO.h"

namespace pm {

/** @namespace graph
    @brief traits classes related to graphs
*/

namespace graph {

// the values must be consistent with sparse2d::symmetric
struct Undirected : True {
   typedef Undirected non_multi_type;
   static const bool multigraph=false;
};
struct Directed : False {
   typedef Directed non_multi_type;
   static const bool multigraph=false;
};

struct UndirectedMulti : True {
   typedef Undirected non_multi_type;
   static const bool multigraph=true;
};
struct DirectedMulti : False {
   typedef Directed non_multi_type;
   static const bool multigraph=true;
};

template <typename dir=Undirected> class Graph;

template <typename IteratorRef>
struct uniq_edge_predicate {
   typedef IteratorRef argument_type;
   typedef bool result_type;

   bool operator() (argument_type it) const
   {
      return it.to_node() <= it.from_node();
   }
};

template <typename EdgeContainer>
class uniq_edge_list
   : public modified_container_impl< uniq_edge_list<EdgeContainer>,
                                     list( Hidden< EdgeContainer >,
                                           IteratorConstructor< input_truncator_constructor >,
                                           Operation< BuildUnaryIt<uniq_edge_predicate> > ) > {
public:
   int dim() const { return this->hidden().dim(); }
};

template <typename Graph, bool directed=Graph::is_directed,
          bool _is_masquerade=attrib<typename Graph::out_edge_list_ref>::is_reference>
struct edge_container_impl;

} // end namespace pm::graph

/** @class GenericGraph
    @brief @ref generic "Generic type" for all graph classes.

    Defines various types and constants.
*/

template <typename GraphTop, typename dir_val=typename GraphTop::dir>
class GenericGraph : public Generic<GraphTop> {
protected:
   GenericGraph() {}
   GenericGraph(const GenericGraph&) {}
#if POLYMAKE_DEBUG
   ~GenericGraph() { POLYMAKE_DEBUG_METHOD(GenericGraph,dump); }
   void dump() const { cerr << this->top() << std::flush; }
#endif

public:
   typedef dir_val dir;
   static const bool is_directed=dir::value==graph::Directed::value;
   static const bool is_multigraph=dir::multigraph;
   typedef GenericGraph generic_type;
   typedef graph::Graph<dir> persistent_type;
   typedef typename Generic<GraphTop>::top_type top_type;

   template <typename Result>
   struct rebind_generic {
      typedef GenericGraph<Result, dir> type;
   };

   int nodes() const
   {
      AssertOVERLOADED(GenericGraph,top_type,nodes);
      return this->top().nodes();
   }

   // Shortcuts for operations on adjacency matrices.
   // Note that in-place modifications are not defined for multigraphs.

   template <typename Graph2>
   typename enable_if<typename cons<top_type, Graph2>::head, !is_multigraph>::type&
   operator+= (const GenericGraph<Graph2>& g2)
   {
      if (POLYMAKE_DEBUG || !Unwary<GraphTop>::value || !Unwary<Graph2>::value) {
         if (this->top().nodes() != g2.top().nodes())
            throw std::runtime_error("GenericGraph::operator+= - dimension mismatch");
      }
      adjacency_matrix(this->top()) += adjacency_matrix(g2.top());
      return this->top();
   }

   template <typename Graph2>
   typename enable_if<typename cons<top_type, Graph2>::head, !is_multigraph>::type&
   operator-= (const GenericGraph<Graph2>& g2)
   {
      if (POLYMAKE_DEBUG || !Unwary<GraphTop>::value || !Unwary<Graph2>::value) {
         if (this->top().nodes() != g2.top().nodes())
            throw std::runtime_error("GenericGraph::operator-= - dimension mismatch");
      }
      adjacency_matrix(this->top()) -= adjacency_matrix(g2.top());
      return this->top();
   }

   template <typename Graph2>
   typename enable_if<typename cons<top_type, Graph2>::head, !is_multigraph>::type&
   operator*= (const GenericGraph<Graph2>& g2)
   {
      if (POLYMAKE_DEBUG || !Unwary<GraphTop>::value || !Unwary<Graph2>::value) {
         if (this->top().nodes() != g2.top().nodes())
            throw std::runtime_error("GenericGraph::operator*= - dimension mismatch");
      }
      adjacency_matrix(this->top()) *= adjacency_matrix(g2.top());
      return this->top();
   }

   template <typename Graph2>
   typename enable_if<typename cons<top_type, Graph2>::head, !is_multigraph>::type&
   operator^= (const GenericGraph<Graph2>& g2)
   {
      if (POLYMAKE_DEBUG || !Unwary<GraphTop>::value || !Unwary<Graph2>::value) {
         if (this->top().nodes() != g2.top().nodes())
            throw std::runtime_error("GenericGraph::operator^= - dimension mismatch");
      }
      adjacency_matrix(this->top()) ^= adjacency_matrix(g2.top());
      return this->top();
   }

   template <typename Graph2>
   graph::Graph<typename if_else<dir::value==Graph2::dir::value, dir, graph::Directed>::type>
   operator+ (const GenericGraph<Graph2>& g2) const
   {
      if (POLYMAKE_DEBUG || !Unwary<GraphTop>::value || !Unwary<Graph2>::value) {
         if (this->top().nodes() != g2.top().nodes())
            throw std::runtime_error("GenericGraph::operator+ - dimension mismatch");
      }
      graph::Graph<typename if_else<dir::value==Graph2::dir::value, dir, graph::Directed>::type> result(this->top());
      return result+=g2;
   }

   template <typename Graph2>
   graph::Graph<typename dir::non_multi_type>
   operator- (const GenericGraph<Graph2>& g2) const
   {
      if (POLYMAKE_DEBUG || !Unwary<GraphTop>::value || !Unwary<Graph2>::value) {
         if (this->top().nodes() != g2.top().nodes())
            throw std::runtime_error("GenericGraph::operator- - dimension mismatch");
      }
      graph::Graph<typename dir::non_multi_type> result(this->top());
      return result-=g2;
   }

   template <typename Graph2>
   graph::Graph<typename if_else<dir::value==Graph2::dir::value, dir, graph::Directed>::type>
   operator* (const GenericGraph<Graph2>& g2) const
   {
      if (POLYMAKE_DEBUG || !Unwary<GraphTop>::value || !Unwary<Graph2>::value) {
         if (this->top().nodes() != g2.top().nodes())
            throw std::runtime_error("GenericGraph::operator* - dimension mismatch");
      }
      graph::Graph<typename if_else<dir::value==Graph2::dir::value, dir, graph::Directed>::type> result(this->top());
      return result*=g2;
   }

   template <typename Graph2>
   graph::Graph<typename if_else<dir::value==Graph2::dir::value, dir, graph::Directed>::type>
   operator^ (const GenericGraph<Graph2>& g2) const
   {
      if (POLYMAKE_DEBUG || !Unwary<GraphTop>::value || !Unwary<Graph2>::value) {
         if (this->top().nodes() != g2.top().nodes())
            throw std::runtime_error("GenericGraph::operator^ - dimension mismatch");
      }
      graph::Graph<typename if_else<dir::value==Graph2::dir::value, dir, graph::Directed>::type> result(this->top());
      return result^=g2;
   }

   graph::Graph<typename dir::non_multi_type>
   operator~ () const
   {
      graph::Graph<typename dir::non_multi_type> Gcompl(nodes());
      adjacency_matrix(Gcompl) = ~adjacency_matrix(*this);
      return Gcompl;
   }

   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& in, GenericGraph& me)
   {
      return in.top() >> adjacency_matrix(me);
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const GenericGraph& me)
   {
      return out.top() << adjacency_matrix(me);
   }
};

template <typename Graph>
class Nodes :
   public redirected_container< Nodes<Graph>,
                                list( Container< typename Graph::node_container_ref >,
                                      MasqueradedTop ) >,
   public GenericSet<Nodes<Graph>, int, operations::cmp> {
protected:
   Nodes();
   ~Nodes();
public:
   typename Graph::node_container_ref get_container()
   {
      return this->hidden().template pretend<typename Graph::node_container_ref>();
   }
   typename Graph::const_node_container_ref get_container() const
   {
      return this->hidden().template pretend<typename Graph::const_node_container_ref>();
   }
};

template <typename Graph>
class Edges :
   public cascade_impl< Edges<Graph>,
                        list( Container< typename graph::edge_container_impl<Graph>::container >,
                              CascadeDepth< int2type<2> >,
                              MasqueradedTop ) >,
   public graph::edge_container_impl<Graph> {
protected:
   Edges();
   ~Edges();
public:
   using graph::edge_container_impl<Graph>::get_container;
   int size() const { return this->hidden().edges(); }
   int max_size() const { return size(); }
};

namespace graph {

template <typename Top, typename Graph>
struct edge_container_access {
   typedef typename Graph::out_edge_list_container container;
   typedef typename Graph::out_edge_list_container_ref container_ref;
   typedef typename Graph::const_out_edge_list_container_ref const_container_ref;

   container_ref get_container() { return static_cast<Top*>(this)->hidden().template pretend<container_ref>(); }
   const_container_ref get_container() const { return static_cast<const Top*>(this)->hidden().template pretend<const_container_ref>(); }
};

template <typename Graph, bool _is_masquerade>
struct edge_container_impl<Graph, true, _is_masquerade> :
   edge_container_access<Edges<Graph>, Graph> {};

template <typename Graph>
struct edge_container_impl<Graph, false, true> {
   typedef edge_container_impl<Graph, true, true> _super;

   class container :
      public modified_container_impl< container,
                                      list( Hidden< Graph >,
                                            Container< typename _super::container_ref >,
                                            Operation< operations::masquerade<uniq_edge_list> > ) >,
      public edge_container_access<container, Graph> {
   public:
      using edge_container_access<container, Graph>::get_container;
   };

   container& get_container() { return reinterpret_cast<container&>(static_cast<Edges<Graph>*>(this)->hidden()); }
   const container& get_container() const { return reinterpret_cast<const container&>(static_cast<const Edges<Graph>*>(this)->hidden()); }
};

template <typename Graph>
struct edge_container_impl<Graph, false, false> {
   typedef edge_container_impl<Graph, true, false> _super;
   typedef operations::construct_unary2<TruncatedContainer, BuildUnaryIt<uniq_edge_predicate> > operation;

   class container :
      public modified_container_impl< container,
                                      list( Hidden< Graph >,
                                            Container< typename _super::container_ref >,
                                            Operation< operation > ) >,
      public edge_container_access<container, Graph> {
   public:
      using edge_container_access<container, Graph>::get_container;
   };

   class const_container :
      public modified_container_impl< const_container,
                                      list( Hidden< Graph >,
                                            Container< typename _super::const_container_ref >,
                                            Operation< operation > ) >,
      public edge_container_access<const_container,Graph> {
   public:
      using edge_container_access<const_container,Graph>::get_container;
   };

   container& get_container() { return reinterpret_cast<container&>(static_cast<Edges<Graph>*>(this)->hidden()); }
   const const_container& get_container() const { return reinterpret_cast<const const_container&>(static_cast<const Edges<Graph>*>(this)->hidden()); }
};

} // end namespace graph

template <typename Graph, bool is_multigraph=Graph::is_multigraph>
class AdjacencyMatrix;

template <typename Graph>
class AdjacencyMatrix<Graph, false> :
   public GenericIncidenceMatrix< AdjacencyMatrix<Graph, false> > {
protected:
   AdjacencyMatrix();
   ~AdjacencyMatrix();
public:
   typedef bool value_type;
   typedef bool reference;
   typedef const bool const_reference;

   Graph& hidden() { return reinterpret_cast<Graph&>(*this); }
   const Graph& hidden() const { return reinterpret_cast<const Graph&>(*this); }
};

template <typename Graph>
class AdjacencyMatrix<Graph, true> :
   public GenericMatrix< AdjacencyMatrix<Graph, true>, int > {
protected:
   AdjacencyMatrix();
   ~AdjacencyMatrix();
public:
   typedef int value_type;
   typedef int reference;
   typedef const int const_reference;

   Graph& hidden() { return reinterpret_cast<Graph&>(*this); }
   const Graph& hidden() const { return reinterpret_cast<const Graph&>(*this); }
};

template <typename Graph, bool is_multigraph>
struct check_container_feature< AdjacencyMatrix<Graph, is_multigraph>, Symmetric > :
   bool2type< !Graph::is_directed > {};

template <typename Graph>
struct check_container_feature< AdjacencyMatrix<Graph, true>, pure_sparse > : True {};

template <typename Graph>
class matrix_random_access_methods< AdjacencyMatrix<Graph, false> > {
public:
   bool operator() (int i, int j) const
   {
      return static_cast<const AdjacencyMatrix<Graph>&>(*this).hidden().edge_exists(i,j);
   }
};

template <typename Graph>
class matrix_random_access_methods< AdjacencyMatrix<Graph, true> > {
public:
   int operator() (int i, int j) const
   {
      return count_it(static_cast<const AdjacencyMatrix<Graph>&>(*this).hidden().all_edges(i,j));
   }
};

template <typename Graph, bool is_multigraph>
class Rows< AdjacencyMatrix<Graph, is_multigraph> >
   : public redirected_container< Rows< AdjacencyMatrix<Graph, is_multigraph> >,
                                  list( Container< typename Graph::adjacency_rows_container_ref >,
                                        Hidden< Graph > ) > {
public:
   typename Graph::adjacency_rows_container_ref get_container()
   {
      return this->hidden().template pretend<typename Graph::adjacency_rows_container_ref>();
   }
   typename Graph::const_adjacency_rows_container_ref get_container() const
   {
      return this->hidden().template pretend<typename Graph::const_adjacency_rows_container_ref>();
   }
};

template <typename Graph, bool is_multigraph>
class Cols< AdjacencyMatrix<Graph, is_multigraph> >
   : public redirected_container< Cols< AdjacencyMatrix<Graph, is_multigraph> >,
                                  list( Container< typename Graph::adjacency_cols_container >,
                                        Hidden< Graph > ) > {
public:
   typename Graph::adjacency_cols_container_ref get_container()
   {
      return this->hidden().template pretend<typename Graph::adjacency_cols_container_ref>();
   }
   typename Graph::const_adjacency_cols_container_ref get_container() const
   {
      return this->hidden().template pretend<typename Graph::const_adjacency_cols_container_ref>();
   }
};

template <typename Graph> inline
Nodes<typename Unwary<Graph>::type>&
nodes(GenericGraph<Graph>& G)
{
   return reinterpret_cast<Nodes<typename Unwary<Graph>::type>&>(G.top());
}

template <typename Graph> inline
const Nodes<typename Unwary<Graph>::type>&
nodes(const GenericGraph<Graph>& G)
{
   return reinterpret_cast<const Nodes<typename Unwary<Graph>::type>&>(G.top());
}

template <typename Graph> inline
Edges<typename Unwary<Graph>::type>&
edges(GenericGraph<Graph>& G)
{
   return reinterpret_cast<Edges<typename Unwary<Graph>::type>&>(G.top());
}

template <typename Graph> inline
const Edges<typename Unwary<Graph>::type>&
edges(const GenericGraph<Graph>& G)
{
   return reinterpret_cast<const Edges<typename Unwary<Graph>::type>&>(G.top());
}

template <typename Graph> inline
AdjacencyMatrix<typename Unwary<Graph>::type>&
adjacency_matrix(GenericGraph<Graph>& G)
{
   return reinterpret_cast<AdjacencyMatrix<typename Unwary<Graph>::type>&>(G.top());
}

template <typename Graph> inline
const AdjacencyMatrix<typename Unwary<Graph>::type>&
adjacency_matrix(const GenericGraph<Graph>& G)
{
   return reinterpret_cast<const AdjacencyMatrix<typename Unwary<Graph>::type>&>(G.top());
}

template <typename Graph> inline
typename Unwary<Graph>::type::out_edge_list_container_ref
out_edge_lists(GenericGraph<Graph>& G)
{
   return G.top().template pretend<typename Unwary<Graph>::type::out_edge_list_container_ref>();
}
template <typename Graph> inline
typename Unwary<Graph>::type::const_out_edge_list_container_ref
out_edge_lists(const GenericGraph<Graph>& G)
{
   return G.top().template pretend<typename Unwary<Graph>::type::const_out_edge_list_container_ref>();
}

template <typename Graph> inline
typename Unwary<Graph>::type::in_edge_list_container_ref
in_edge_lists(GenericGraph<Graph>& G)
{
   return G.top().template pretend<typename Unwary<Graph>::type::in_edge_list_container_ref>();
}
template <typename Graph> inline
typename Unwary<Graph>::type::const_in_edge_list_container_ref
in_edge_lists(const GenericGraph<Graph>& G)
{
   return G.top().template pretend<typename Unwary<Graph>::type::const_in_edge_list_container_ref>();
}

template <typename Graph1, typename Graph2> inline
bool operator== (const GenericGraph<Graph1>& G1, const GenericGraph<Graph2>& G2)
{
   if (G1.nodes() != G2.nodes()) return false;
   return adjacency_matrix(G1)==adjacency_matrix(G2);
}

template <typename Graph1, typename Graph2> inline
bool operator!= (const GenericGraph<Graph1>& G1, const GenericGraph<Graph2>& G2)
{
   return !(G1==G2);
}

struct is_graph;

template <typename Graph, typename dir>
struct spec_object_traits< GenericGraph<Graph,dir> > :
   spec_or_model_traits<Graph,is_opaque> {
   typedef is_graph generic_tag;
   static const bool IO_ends_with_eol=true;
};

template <typename Graph>
struct spec_object_traits< Nodes<Graph> > :
   spec_object_traits<is_container> {
   typedef Graph masquerade_for;
   static const bool is_always_const=true;
};

template <typename Graph>
struct spec_object_traits< Edges<Graph> > :
   spec_object_traits<is_container> {
   typedef Graph masquerade_for;
   static const bool is_always_const=true;
};

template <typename Graph, bool is_multigraph>
struct spec_object_traits< AdjacencyMatrix<Graph, is_multigraph> > :
   spec_object_traits<is_container> {
   typedef Graph masquerade_for;
   static const bool is_always_const=object_traits<Graph>::is_always_const || is_multigraph;
   static const int is_resizeable=object_traits<Graph>::is_resizeable;
};

template <typename Graph>
class WaryGraph : public GenericGraph<Wary<Graph>, typename Graph::dir> {
public:
   int out_degree(int n) const
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::out_degree - node id out of range or deleted");
      return this->top().out_degree(n);
   }
   int in_degree(int n) const
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::in_degree - node id out of range or deleted");
      return this->top().in_degree(n);
   }
   int degree(int n) const
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::degree - node id out of range or deleted");
      return this->top().degree(n);
   }

   bool node_exists(int n) const
   {
      if (this->top().node_out_of_range(n))
         throw std::runtime_error("Graph::node_exists - node id out of range");
      return this->top().node_exists(n);
   }
   void delete_node(int n)
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::delete_node - node id out of range or already deleted");
      this->top().delete_node(n);
   }

   int edge(int n1, int n2)
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("Graph::edge - node id out of range or deleted");
      return this->top().edge(n1,n2);
   }
   int edge(int n1, int n2) const
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("Graph::edge - node id out of range or deleted");
      return this->top().edge(n1,n2);
   }
   int add_edge(int n1, int n2)
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("Graph::add_edge - node id out of range or deleted");
      return this->top().add_edge(n1,n2);
   }

   bool edge_exists(int n1, int n2) const
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("Graph::edge_exists - node id out of range or deleted");
      return this->top().edge_exists(n1,n2);
   }
   typename Graph::parallel_edge_iterator all_edges(int n1, int n2)
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("Graph::all_edges - node id out of range or deleted");
      return this->top().all_edges(n1,n2);
   }
   typename Graph::parallel_edge_const_iterator all_edges(int n1, int n2) const
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("Graph::all_edges - node id out of range or deleted");
      return this->top().all_edges(n1,n2);
   }

   void delete_edge(int n1, int n2)
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("Graph::delete_edge - node id out of range or deleted");
      this->top().delete_edge(n1,n2);
   }
   void delete_all_edges(int n1, int n2)
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("Graph::delete_all_edges - node id out of range or deleted");
      this->top().delete_all_edges(n1,n2);
   }

   void contract_edge(int n1, int n2)
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("Graph::contract_edge - node id out of range or deleted");
      if (n1==n2)
         throw std::runtime_error("Graph::contract_edge - can't contract a loop");
      this->top().contract_edge(n1,n2);
   }

   typename Graph::out_edge_list_ref out_edges(int n)
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::out_edges - node id out of range or deleted");
      return this->top().out_edges(n);
   }
   typename Graph::const_out_edge_list_ref out_edges(int n) const
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::out_edges - node id out of range or deleted");
      return this->top().out_edges(n);
   }

   typename Graph::in_edge_list_ref in_edges(int n)
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::in_edges - node id out of range or deleted");
      return this->top().in_edges(n);
   }
   typename Graph::const_in_edge_list_ref in_edges(int n) const
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::in_edges - node id out of range or deleted");
      return this->top().in_edges(n);
   }

   typename Graph::out_adjacent_node_list_ref out_adjacent_nodes(int n)
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::out_adjacent_nodes - node id out of range or deleted");
      return this->top().out_adjacent_nodes(n);
   }
   typename Graph::const_out_adjacent_node_list_ref out_adjacent_nodes(int n) const
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::out_adjacent_nodes - node id out of range or deleted");
      return this->top().out_adjacent_nodes(n);
   }
   typename Graph::in_adjacent_node_list_ref in_adjacent_nodes(int n)
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::in_adjacent_nodes - node id out of range or deleted");
      return this->top().in_adjacent_nodes(n);
   }
   typename Graph::const_in_adjacent_node_list_ref in_adjacent_nodes(int n) const
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::in_adjacent_nodes - node id out of range or deleted");
      return this->top().in_adjacent_nodes(n);
   }
   typename Graph::adjacent_node_list_ref adjacent_nodes(int n)
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::adjacent_nodes - node id out of range or deleted");
      return this->top().adjacent_nodes(n);
   }
   typename Graph::const_adjacent_node_list_ref adjacent_nodes(int n) const
   {
      if (this->top().invalid_node(n))
         throw std::runtime_error("Graph::adjacent_nodes - node id out of range or deleted");
      return this->top().adjacent_nodes(n);
   }
};

template <typename GraphRef, template <typename> class Iterator>
class generic_of_GraphComponents {};

template <typename GraphRef, template <typename> class Iterator>
class GraphComponents : public generic_of_GraphComponents<GraphRef,Iterator> {
protected:
   alias<GraphRef> graph;
public:
   typedef typename alias<GraphRef>::arg_type arg_type;
   typedef typename deref<GraphRef>::type Graph;
   typename alias<GraphRef>::const_reference get_graph() const { return *graph; }

   GraphComponents(arg_type G) : graph(G) {}

   typedef Iterator<Graph> iterator;
   typedef iterator const_iterator;
   typedef typename iterator::value_type value_type;
   typedef const value_type reference;
   typedef reference const_reference;

   iterator begin() const { return get_graph(); }
   iterator end() const { return iterator(); }
   reference front() const { return *begin(); }
   int size() const { return count_it(begin()); }
   bool empty() const { return get_graph().nodes()==0; }
};

template <typename GraphRef, template <typename> class Iterator>
struct spec_object_traits< GraphComponents<GraphRef,Iterator> > : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_lazy=true, is_always_const=true;
};

} // end namespace pm

namespace polymake {
   using pm::GenericGraph;
   using pm::graph::Directed;
   using pm::graph::Undirected;
   using pm::graph::DirectedMulti;
   using pm::graph::UndirectedMulti;
   using pm::Nodes;
   using pm::Edges;
   using pm::AdjacencyMatrix;
}

#endif // POLYMAKE_GENERIC_GRAPH_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
