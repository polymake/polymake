/* Copyright (c) 1997-2016
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

#ifndef POLYMAKE_INDEXED_SUBGRAPH_H
#define POLYMAKE_INDEXED_SUBGRAPH_H

#include "polymake/IndexedSubset.h"
#include "polymake/GenericGraph.h"
#include <cassert>

namespace pm {

template <typename IteratorPair, typename Operation>
class subgraph_node_iterator : public IteratorPair {
   typedef IteratorPair base_t;
public:
   typedef subgraph_node_iterator<typename iterator_traits<IteratorPair>::iterator, Operation>
      iterator;
   typedef subgraph_node_iterator<typename iterator_traits<IteratorPair>::const_iterator, Operation>
      const_iterator;

   typedef binary_helper<IteratorPair, Operation> helper;
   typedef typename helper::operation operation;
protected:
   operation op;
public:
   subgraph_node_iterator() {}

   template <typename Operation2>
   subgraph_node_iterator(const subgraph_node_iterator<typename iterator_traits<IteratorPair>::iterator, Operation2>& it)
      : base_t(it)
      , op(helper::create(it.op)) {}

   subgraph_node_iterator(const IteratorPair& it_arg, const Operation& op_arg=Operation())
      : base_t(it_arg)
      , op(helper::create(op_arg)) {}

   template <typename SourceIterator1, typename SourceIterator2,
             typename enable=typename std::enable_if<is_const_compatible_with<SourceIterator1, typename IteratorPair::first_type>::value &&
                                                     is_const_compatible_with<SourceIterator2, typename IteratorPair::second_type>::value>::type>
   subgraph_node_iterator(const SourceIterator1& first_arg,
                          const SourceIterator2& second_arg,
                          const Operation& op_arg=Operation())
      : base_t(first_arg, second_arg)
      , op(helper::create(op_arg)) {}

   typedef typename operation::out_edge_list out_edge_list_ref;
   typedef typename operation::in_edge_list in_edge_list_ref;
   typedef typename operation::out_adjacent_node_list out_adjacent_node_list_ref;
   typedef typename operation::in_adjacent_node_list in_adjacent_node_list_ref;

   out_edge_list_ref out_edges() const
   {
      return op.out_edges(*this, *helper::get2(this->second));
   }
   in_edge_list_ref in_edges() const
   {
      return op.in_edges(*this, *helper::get2(this->second));
   }

   int out_degree() const
   {
      return op.out_degree(*this, *helper::get2(this->second));
   }
   int in_degree() const
   {
      return op.in_degree(*this, *helper::get2(this->second));
   }

   out_adjacent_node_list_ref out_adjacent_nodes() const
   {
      return op.out_adjacent_nodes(*this, *helper::get2(this->second));
   }
   in_adjacent_node_list_ref in_adjacent_nodes() const
   {
      return op.in_adjacent_nodes(*this, *helper::get2(this->second));
   }
};

template <typename IteratorPair, typename Operation, typename Feature>
struct check_iterator_feature<subgraph_node_iterator<IteratorPair, Operation>, Feature>
   : check_iterator_feature<IteratorPair, Feature> {};

struct subgraph_node_access_constructor : binary_transform_constructor<> {
   template <typename IteratorPair, typename Operation, typename ExpectedFeatures>
   struct defs : binary_transform_constructor<>::defs<IteratorPair, Operation, ExpectedFeatures> {
      typedef subgraph_node_iterator<IteratorPair, Operation> iterator;
   };
};

struct subgraph_edge_accessor {
   typedef int first_argument_type;
   typedef void second_argument_type;
   typedef int result_type;
   template <typename Iterator2>
   int operator() (int edge_id, const Iterator2&) const { return edge_id; }

   template <typename IteratorPair>
   class mix_in : public IteratorPair {
      typedef typename IteratorPair::first_type Iterator1;      // runs along the full edge list
   public:
      mix_in() {}

      template <typename SourceIterator1, typename SourceIterator2,
                typename enable=typename std::enable_if<is_const_compatible_with<SourceIterator1, typename IteratorPair::first_type>::value &&
                                                        is_const_compatible_with<SourceIterator2, typename IteratorPair::second_type>::value>::type>
      mix_in(const SourceIterator1& first_arg, const SourceIterator2& second_arg)
         : IteratorPair(first_arg, second_arg) {}

      int from_node() const
      {
         return (Iterator1::symmetric || Iterator1::row_oriented)
                ? *Iterator1::second
                : this->second.index();
      }

      int to_node() const
      {
         return (Iterator1::symmetric || Iterator1::row_oriented)
                ? this->second.index()
                : *Iterator1::second;
      }
   };
};

template <typename EdgeListRef>
class subgraph_edge_list
   : public container_pair_impl< subgraph_edge_list<EdgeListRef>,
                                 mlist< Container1Tag< EdgeListRef >,
                                        Container2Tag< constant_value_container<int> > > > {
protected:
   alias<EdgeListRef> edges;
   int own_index;
public:
   subgraph_edge_list(typename alias<EdgeListRef>::arg_type edges_arg, int index_arg)
      : edges(edges_arg), own_index(index_arg) {}

   typename alias<EdgeListRef>::reference get_container1() { return *edges; }
   typename alias<EdgeListRef>::const_reference get_container1() const { return *edges; }
   constant_value_container<int> get_container2() const { return own_index; }

   int dim() const { return get_container1().dim(); }
};

template <typename EdgeListRef>
struct check_container_feature<subgraph_edge_list<EdgeListRef>, sparse_compatible> : std::true_type {};

template <typename Iterator1Ref, typename Iterator2Ref>
struct construct_subgraph_edge_list {
   typedef Iterator1Ref first_argument_type;
   typedef Iterator2Ref second_argument_type;
   typedef subgraph_edge_list<typename deref<Iterator1Ref>::type::reference> edge_container;
   typedef IndexedSlice<edge_container, typename deref<Iterator2Ref>::type::reference,
                        mlist< HintTag<sparse>,
                               OperationTag< pair< subgraph_edge_accessor,
                                                   operations::apply2< BuildUnaryIt<operations::index2element> > > > > >
      result_type;

   result_type operator() (first_argument_type it1, second_argument_type it2) const
   {
      return result_type(edge_container(*it1, it1.index()), *it2);
   }
};

template <typename GraphRef, typename SetRef, typename TParams, bool TEnabled>
class IndexedSubgraph_random_access_methods {};

template <typename GraphRef, typename SetRef, typename TParams>
class IndexedSubgraph_base
   : public container_pair_base<GraphRef, typename Diligent<SetRef>::type> {
   typedef container_pair_base<GraphRef, typename Diligent<SetRef>::type> base_t;
   template <typename, typename, typename, bool> friend class IndexedSubgraph_random_access_methods;
public:
   typedef typename deref<GraphRef>::type graph_type;
   typedef typename deref<SetRef>::type node_set_type;
protected:
   typedef typename mtagged_list_extract<TParams, RenumberTag, std::false_type>::type renumber_nodes;

   typename inherit_const<graph_type, GraphRef>::type& get_graph() { return base_t::get_container1(); }
   const graph_type& get_graph() const { return base_t::get_container1(); }
   const alias<typename Diligent<SetRef>::type>& get_node_set_alias() const { return this->src2; }
   typename alias<typename Diligent<SetRef>::type>::const_reference get_node_set() const { return base_t::get_container2(); }

   IndexedSubgraph_base(typename base_t::first_arg_type graph_arg, typename alias<SetRef>::arg_type node_set_arg)
      : base_t(graph_arg, diligent(node_set_arg)) {}

   typedef typename mlist_prepend_if<renumber_nodes::value,
                                     mlist< RenumberTag<std::true_type>, ExpectedFeaturesTag<indexed> >,
                                     HiddenTag<IndexedSubgraph_base> >::type
      node_selector_params;

   template <typename Source>
   class node_selector
      : public indexed_subset_impl< node_selector<Source>,
                                    typename mlist_concat< Container1Tag< Source >, Container2Tag< SetRef >,
                                                           node_selector_params >::type > {
      typedef indexed_subset_impl<node_selector> base_t;
   public:
      typename base_t::container1& get_container1()
      {
         return this->hidden().get_graph().template pretend<typename base_t::container1&>();
      }
      const typename base_t::container1& get_container1() const
      {
         return this->hidden().get_graph().template pretend<const typename base_t::container1&>();
      }
      const node_set_type& get_container2() const
      {
         return this->hidden().get_node_set();
      }
   };

   template <typename Source, typename AddParams>
   class masquerade_container
      : public modified_container_pair_impl< masquerade_container<Source, AddParams>,
                                             typename mlist_concat< Container1Tag< node_selector<Source> >,
                                                                    Container2Tag< constant_value_container<typename Diligent<SetRef>::type> >,
                                                                    HiddenTag< IndexedSubgraph_base >,
                                                                    AddParams >::type > {
      typedef modified_container_pair_impl<masquerade_container> base_t;
   public:
      const typename base_t::container2& get_container2() const
      {
         return constant(this->hidden().get_node_set_alias());
      }
   };

   typedef typename std::conditional<renumber_nodes::value,
                                     BuildBinaryIt<construct_subgraph_edge_list>,
                                     operations::construct_binary2<IndexedSubset, HintTag<sparse> > >::type
      edge_list_operation;
   typedef typename std::conditional<renumber_nodes::value,
                                     operations::construct_binary2<IndexedSlice, HintTag<sparse> >,
                                     operations::construct_binary2<LazySet2, set_intersection_zipper> >::type
      adjacent_node_list_operation;

public:
   template <typename Iterator>
   class node_accessor {
   public:
      typedef Iterator first_argument_type;
      typedef SetRef second_argument_type;
      typedef typename function_argument<SetRef>::const_type second_arg_type;
      typedef typename iterator_traits<Iterator>::reference result_type;
      typedef typename constant_value_container<typename Diligent<SetRef>::type>::const_iterator ConstIterator2;
   private:
      typename Iterator::out_edge_list_ref out_intermed(const Iterator& it, std::false_type) const
      {
         return it.out_edges();
      }
      typename Iterator::in_edge_list_ref in_intermed(const Iterator& it, std::false_type) const
      {
         return it.in_edges();
      }

      subgraph_edge_list<typename Iterator::out_edge_list_ref> out_intermed(const Iterator& it, std::true_type) const
      {
         return subgraph_edge_list<typename Iterator::out_edge_list_ref>(it.out_edges(), it.index());
      }
      subgraph_edge_list<typename Iterator::in_edge_list_ref> in_intermed(const Iterator& it, std::true_type) const
      {
         return subgraph_edge_list<typename Iterator::in_edge_list_ref>(it.in_edges(), it.index());
      }

      static const bool is_const=std::is_same<Iterator, typename container_traits<typename masquerade<Nodes, GraphRef>::type>::const_iterator>::value;
      typedef typename std::conditional<is_const,
                                        typename deref<GraphRef>::type::out_edge_list_container::const_iterator,
                                        typename deref<GraphRef>::type::out_edge_list_container::iterator>::type
         out_edge_lists_iterator;
      typedef typename std::conditional<is_const,
                                        typename deref<GraphRef>::type::in_edge_list_container::const_iterator,
                                        typename deref<GraphRef>::type::in_edge_list_container::iterator>::type
         in_edge_lists_iterator;
   public:
      typedef typename binary_op_builder<edge_list_operation, out_edge_lists_iterator, ConstIterator2>::operation::result_type
         out_edge_list;
      typedef typename binary_op_builder<edge_list_operation, in_edge_lists_iterator, ConstIterator2>::operation::result_type
         in_edge_list;
      typedef typename binary_op_builder<adjacent_node_list_operation, void, void,
                                         typename Iterator::out_adjacent_node_list_ref, SetRef>::operation::result_type
         out_adjacent_node_list;
      typedef typename binary_op_builder<adjacent_node_list_operation, void, void,
                                         typename Iterator::in_adjacent_node_list_ref, SetRef>::operation::result_type
         in_adjacent_node_list;

      out_edge_list out_edges(const Iterator& it, second_arg_type set) const
      {
         return out_edge_list(out_intermed(it, renumber_nodes()),set);
      }
      in_edge_list in_edges(const Iterator& it, second_arg_type set) const
      {
         return in_edge_list(in_intermed(it, renumber_nodes()),set);
      }
      out_adjacent_node_list out_adjacent_nodes(const Iterator& it, second_arg_type set) const
      {
         return out_adjacent_node_list(it.out_adjacent_nodes(),set);
      }
      in_adjacent_node_list in_adjacent_nodes(const Iterator& it, second_arg_type set) const
      {
         return in_adjacent_node_list(it.in_adjacent_nodes(),set);
      }
      int out_degree(const Iterator& it, second_arg_type set) const
      {
         return out_edges(it,set).size();
      }
      int in_degree(const Iterator& it, second_arg_type set) const
      {
         return in_edges(it,set).size();
      }
   };

protected:
   template <typename Source>
   class masquerade_node_container
      : public masquerade_container<Source,
                                    mlist< OperationTag< node_accessor<typename container_traits<Source>::iterator> >,
                                           IteratorConstructorTag< subgraph_node_access_constructor > > > {};
public:
   typedef masquerade_node_container<typename std::conditional<attrib<GraphRef>::is_const,
                                                               typename graph_type::const_node_container_ref,
                                                               typename graph_type::node_container_ref>::type>
      node_container;
   typedef masquerade_node_container<typename graph_type::const_node_container_ref>
      const_node_container;
   typedef typename inherit_const<node_container, GraphRef>::type& node_container_ref;
   typedef const const_node_container& const_node_container_ref;

   typedef masquerade_container<typename std::conditional<attrib<GraphRef>::is_const,
                                                          typename graph_type::const_out_edge_list_container_ref,
                                                          typename graph_type::out_edge_list_container_ref>::type,
                                OperationTag< edge_list_operation > >
      out_edge_list_container;
   typedef masquerade_container<typename graph_type::const_out_edge_list_container_ref,
                                OperationTag< edge_list_operation > >
      const_out_edge_list_container;
   typedef typename inherit_const<out_edge_list_container, GraphRef>::type& out_edge_list_container_ref;
   typedef const const_out_edge_list_container& const_out_edge_list_container_ref;

   typedef masquerade_container<typename std::conditional<attrib<GraphRef>::is_const,
                                                          typename graph_type::const_in_edge_list_container_ref,
                                                          typename graph_type::in_edge_list_container_ref>::type,
                                OperationTag< edge_list_operation > >
      in_edge_list_container;
   typedef masquerade_container<typename graph_type::const_in_edge_list_container_ref,
                                OperationTag< edge_list_operation > >
      const_in_edge_list_container;
   typedef typename inherit_const<in_edge_list_container, GraphRef>::type& in_edge_list_container_ref;
   typedef const const_in_edge_list_container& const_in_edge_list_container_ref;

   typedef typename out_edge_list_container::value_type out_edge_list;
   typedef typename in_edge_list_container::value_type in_edge_list;
   typedef typename const_out_edge_list_container::value_type const_out_edge_list;
   typedef typename const_in_edge_list_container::value_type const_in_edge_list;
   typedef out_edge_list out_edge_list_ref;
   typedef const_out_edge_list const_out_edge_list_ref;
   typedef in_edge_list in_edge_list_ref;
   typedef const_in_edge_list const_in_edge_list_ref;

   typedef masquerade_container<typename graph_type::const_adjacency_rows_container_ref,
                                OperationTag< adjacent_node_list_operation > >
      adjacency_rows_container;

   typedef masquerade_container<typename graph_type::const_adjacency_cols_container_ref,
                                OperationTag< adjacent_node_list_operation > >
      adjacency_cols_container;

   typedef const adjacency_rows_container& adjacency_rows_container_ref;
   typedef adjacency_rows_container_ref const_adjacency_rows_container_ref;
   typedef const adjacency_cols_container& adjacency_cols_container_ref;
   typedef adjacency_cols_container_ref const_adjacency_cols_container_ref;

   typedef typename adjacency_rows_container::value_type out_adjacent_node_list;
   typedef typename adjacency_cols_container::value_type in_adjacent_node_list;
   typedef out_adjacent_node_list out_adjacent_node_list_ref;
   typedef out_adjacent_node_list const_out_adjacent_node_list_ref;
   typedef in_adjacent_node_list in_adjacent_node_list_ref;
   typedef in_adjacent_node_list const_in_adjacent_list_ref;
};

template <typename GraphRef, typename SetRef, typename TParams>
struct IndexedSubgraph_random_access {
   static constexpr bool enabled= container_traits<typename deref<GraphRef>::type::node_container_ref>::is_random &&
                                  (!tagged_list_extract_integral<TParams, RenumberTag>(false) ||
                                   subset_classifier::index_helper<masquerade<Nodes, GraphRef>, SetRef, false>::random);
   typedef IndexedSubgraph_random_access_methods<GraphRef, SetRef, TParams, enabled> type;
};

template <typename GraphRef, typename SetRef, typename TParams=mlist<>>
class IndexedSubgraph
   : public IndexedSubgraph_base<GraphRef, SetRef, TParams>,
     public IndexedSubgraph_random_access<GraphRef, SetRef, TParams>::type,
     public GenericGraph< IndexedSubgraph<GraphRef, SetRef, TParams>, typename deref<GraphRef>::type::dir > {
   typedef IndexedSubgraph_base<GraphRef, SetRef, TParams> base_t;
   template <typename, typename, typename, bool> friend class IndexedSubgraph_random_access_methods;
protected:
   int nodes_impl(std::false_type) const { return this->get_node_set().size(); }
   int nodes_impl(std::true_type) const { return this->get_graph().nodes()-this->get_node_set().base().size(); }
   int dim_impl(std::false_type) const { return this->get_graph().dim(); }
   int dim_impl(std::true_type) const { return nodes(); }
   int first_node_impl(std::false_type) const { return this->get_node_set().front(); }
   int first_node_impl(std::true_type) const { return -1; }

public:
   IndexedSubgraph(typename base_t::first_arg_type graph_arg, typename alias<SetRef>::arg_type node_set_arg)
      : base_t(graph_arg, node_set_arg) {}

   int nodes() const { return nodes_impl(complement_helper<SetRef>()); }
   int edges() const;
   int dim() const { return dim_impl(typename base_t::renumber_nodes()); }

   bool has_gaps() const
   {
      return !base_t::renumber_nodes::value &&
             !(std::is_same<typename deref<SetRef>::type, sequence>::value && first_node_impl(complement_helper<SetRef>())==0);
   }

   template <typename MasqueradeRef>
   MasqueradeRef pretend()
   {
      return reinterpret_cast<MasqueradeRef>(*this);
   }
   template <typename MasqueradeRef>
   typename attrib<MasqueradeRef>::plus_const pretend() const
   {
      return reinterpret_cast<MasqueradeRef>(*this);
   }
};

template <typename GraphRef, typename SetRef, typename TParams>
int IndexedSubgraph<GraphRef, SetRef, TParams>::edges() const
{
   int cnt=0;
   for (typename Nodes<IndexedSubgraph>::const_iterator n_it=entire(pm::nodes(*this)); !n_it.at_end(); ++n_it)
      cnt+=n_it.out_degree();
   if (deref<GraphRef>::type::dir::value) cnt/=2;
   return cnt;
}

template <typename GraphRef, typename SetRef, typename TParams>
class IndexedSubgraph_random_access_methods<GraphRef, SetRef, TParams, true> {
   typedef IndexedSubgraph<GraphRef, SetRef, TParams> master;
   typedef IndexedSubgraph_base<GraphRef, SetRef, TParams> base;
   static const bool is_directed_local= deref<GraphRef>::type::dir::value==graph::Directed::value;

   master& me() { return static_cast<master&>(*this); }
   const master& me() const { return static_cast<const master&>(*this); }

   int node_index(int n, std::false_type) const
   {
      assert(me().get_node_set().contains(n));
      return n;
   }
   int node_index(int n, std::true_type) const
   {
      return me().get_node_set()[n];
   }
   int node_index(int n) const
   {
      return node_index(n, typename base::renumber_nodes());
   }
public:
   typename base::out_edge_list out_edges(int n)
   {
      return typename base::out_edge_list(me().get_graph().out_edges(node_index(n)), me().get_node_set());
   }
   typename base::const_out_edge_list out_edges(int n) const
   {
      return typename base::const_out_edge_list(me().get_graph().out_edges(node_index(n)), me().get_node_set());
   }
   typename base::in_edge_list in_edges(int n)
   {
      return typename base::in_edge_list(me().get_graph().in_edges(node_index(n)), me().get_node_set());
   }
   typename base::const_in_edge_list in_edges(int n) const
   {
      return typename base::const_in_edge_list(me().get_graph().in_edges(node_index(n)), me().get_node_set());
   }

   int out_degree(int n) const
   {
      return out_edges(n).size();
   }
   int in_degree(int n) const
   {
      return in_edges(n).size();
   }
   int degree (int n) const
   {
      int d=out_degree(n);
      if (is_directed_local) d+=in_degree(n);
      return d;
   }

   typename base::out_adjacent_node_list out_adjacent_nodes(int n) const
   {
      return typename base::out_adjacent_node_list(me().get_graph().out_adjacent_nodes(node_index(n)), me().get_node_set());
   }
   typename base::in_adjacent_node_list in_adjacent_nodes(int n) const
   {
      return typename base::in_adjacent_node_list(me().get_graph().in_adjacent_nodes(node_index(n)), me().get_node_set());
   }

   int edge(int n1, int n2)
   {
      return me().get_graph().edge(node_index(n1), node_index(n2));
   }
   int edge(int n1, int n2) const
   {
      return me().get_graph().edge(node_index(n1), node_index(n2));
   }

   bool edge_exists(int n1, int n2) const
   {
      return me().get_graph().edge_exists(node_index(n1), node_index(n2));
   }
   void delete_edge(int n1, int n2)
   {
      me().get_graph().delete_edge(node_index(n1), node_index(n2));
   }
};

template <typename GraphRef, typename SetRef, typename TParams>
inline
const IndexedSubgraph<GraphRef, SetRef, typename mtagged_list_replace<TParams, RenumberTag<std::true_type>>::type>&
renumber_nodes(const IndexedSubgraph<GraphRef, SetRef, TParams>& G)
{
   return reinterpret_cast<const IndexedSubgraph<GraphRef, SetRef, typename mtagged_list_replace<TParams, RenumberTag<std::true_type>>::type>&>(G);
}

template <typename GraphRef, typename SetRef, typename TParams>
struct spec_object_traits< IndexedSubgraph<GraphRef, SetRef, TParams> >
   : spec_object_traits<is_opaque> {
   static const bool is_temporary=true,
                     is_always_const=effectively_const<GraphRef>::value;
   static const int is_resizeable=0;
};

template <typename Graph, typename Set> inline
IndexedSubgraph<unwary_t<Graph>&, const typename Concrete<Set>::type&>
induced_subgraph(GenericGraph<Graph>& G, const Set& nodes)
{
   if (POLYMAKE_DEBUG || !Unwary<Graph>::value) {
      if (!set_within_range(nodes, G.nodes()))
         throw std::runtime_error("induced_subgraph - node indices out of range");
   }
   return IndexedSubgraph<unwary_t<Graph>&, const typename Concrete<Set>::type&>(G.top(), concrete(nodes));
}

template <typename Graph, typename Set> inline
IndexedSubgraph<const unwary_t<Graph>&, const typename Concrete<Set>::type&>
induced_subgraph(const GenericGraph<Graph>& G, const Set& nodes)
{
   if (POLYMAKE_DEBUG || !Unwary<Graph>::value) {
      if (!set_within_range(nodes, G.nodes()))
         throw std::runtime_error("induced_subgraph - node indices out of range");
   }
   return IndexedSubgraph<const unwary_t<Graph>&, const typename Concrete<Set>::type&>(G.top(), concrete(nodes));
}

} // end namespace pm

#endif // POLYMAKE_INDEXED_GRAPH_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
