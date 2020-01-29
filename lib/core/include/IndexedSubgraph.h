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

   Int out_degree() const
   {
      return op.out_degree(*this, *helper::get2(this->second));
   }
   Int in_degree() const
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
   typedef Int first_argument_type;
   typedef void second_argument_type;
   typedef Int result_type;
   template <typename Iterator2>
   Int operator() (Int edge_id, const Iterator2&) const { return edge_id; }

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

      Int from_node() const
      {
         return (Iterator1::symmetric || Iterator1::row_oriented)
                ? *Iterator1::second
                : this->second.index();
      }

      Int to_node() const
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
                                 mlist< Container1RefTag< EdgeListRef >,
                                        Container2RefTag< same_value_container<Int> > > > {
protected:
   using alias_t = alias<EdgeListRef>;
   alias_t edges;
   Int own_index;
public:
   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   subgraph_edge_list(Arg&& edges_arg, Int index_arg)
      : edges(std::forward<Arg>(edges_arg))
      , own_index(index_arg) {}

   decltype(auto) get_container1() { return *edges; }
   decltype(auto) get_container1() const { return *edges; }
   auto get_container2() const { return same_value_container<Int>(own_index); }

   Int dim() const { return get_container1().dim(); }
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

template <typename GraphRef, typename SetRef, typename Params, bool Enabled>
class IndexedSubgraph_random_access_methods {};

template <typename GraphRef, typename SetRef, typename Params>
class IndexedSubgraph_base
   : public container_pair_base<GraphRef, SetRef> {
   using base_t = container_pair_base<GraphRef, SetRef>;
   template <typename, typename, typename, bool> friend class IndexedSubgraph_random_access_methods;
public:
   using graph_type = typename deref<GraphRef>::type;
protected:
   using renumber_nodes = typename mtagged_list_extract<Params, RenumberTag, std::false_type>::type;

   decltype(auto) get_graph() { return base_t::get_container1(); }
   decltype(auto) get_graph() const { return base_t::get_container1(); }
   const typename base_t::second_alias_t& get_node_set_alias() const { return this->src2; }
   decltype(auto) get_node_set() const { return base_t::get_container2(); }

   using container_pair_base<GraphRef, SetRef>::container_pair_base;

   using node_selector_params = typename mlist_prepend_if<renumber_nodes::value,
                                                          mlist< RenumberTag<std::true_type>, ExpectedFeaturesTag<indexed> >,
                                                          HiddenTag<IndexedSubgraph_base> >::type;

   template <typename Source>
   class node_selector
      : public indexed_subset_impl< node_selector<Source>,
                                    typename mlist_concat< Container1RefTag< Source >, Container2RefTag< SetRef >,
                                                           node_selector_params >::type > {
      using base_t = indexed_subset_impl<node_selector>;
   public:
      typename base_t::container1& get_container1()
      {
         return this->hidden().get_graph().template pretend<typename base_t::container1&>();
      }
      const typename base_t::container1& get_container1() const
      {
         return this->hidden().get_graph().template pretend<const typename base_t::container1&>();
      }
      decltype(auto) get_container2() const
      {
         return this->hidden().get_node_set();
      }
   };

   template <typename Source, typename AddParams>
   class masquerade_container
      : public modified_container_pair_impl< masquerade_container<Source, AddParams>,
                                             typename mlist_concat< Container1Tag< node_selector<Source> >,
                                                                    Container2RefTag< same_value_container<SetRef> >,
                                                                    HiddenTag< IndexedSubgraph_base >,
                                                                    AddParams >::type > {
      typedef modified_container_pair_impl<masquerade_container> base_t;
   public:
      decltype(auto) get_container2() const
      {
         return as_same_value_container(this->hidden().get_node_set_alias());
      }
   };

   using edge_list_operation = std::conditional_t<renumber_nodes::value,
                                                  BuildBinaryIt<construct_subgraph_edge_list>,
                                                  operations::construct_binary2<IndexedSubset, HintTag<sparse>>>;
   using adjacent_node_list_operation = std::conditional_t<renumber_nodes::value,
                                                           operations::construct_binary2<IndexedSlice, HintTag<sparse> >,
                                                           operations::construct_binary2<LazySet2, set_intersection_zipper>>;

public:
   template <typename Iterator>
   class node_accessor {
   public:
      typedef Iterator first_argument_type;
      typedef SetRef second_argument_type;
      typedef typename iterator_traits<Iterator>::reference result_type;
      typedef const pure_type_t<SetRef>& second_arg_type;
      typedef ptr_wrapper<const pure_type_t<SetRef>, false> ConstIterator2;
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

      static constexpr bool is_const=std::is_same<Iterator, typename container_traits<typename masquerade<Nodes, GraphRef>::type>::const_iterator>::value;
      using out_edge_lists_iterator = std::conditional_t<is_const,
                                                         typename deref<GraphRef>::type::out_edge_list_container::const_iterator,
                                                         typename deref<GraphRef>::type::out_edge_list_container::iterator>;
      using in_edge_lists_iterator = std::conditional_t<is_const,
                                                        typename deref<GraphRef>::type::in_edge_list_container::const_iterator,
                                                        typename deref<GraphRef>::type::in_edge_list_container::iterator>;
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
         return out_edge_list(out_intermed(it, renumber_nodes()), set);
      }
      in_edge_list in_edges(const Iterator& it, second_arg_type set) const
      {
         return in_edge_list(in_intermed(it, renumber_nodes()), set);
      }
      out_adjacent_node_list out_adjacent_nodes(const Iterator& it, second_arg_type set) const
      {
         return out_adjacent_node_list(it.out_adjacent_nodes(), set);
      }
      in_adjacent_node_list in_adjacent_nodes(const Iterator& it, second_arg_type set) const
      {
         return in_adjacent_node_list(it.in_adjacent_nodes(), set);
      }
      Int out_degree(const Iterator& it, second_arg_type set) const
      {
         return out_edges(it, set).size();
      }
      Int in_degree(const Iterator& it, second_arg_type set) const
      {
         return in_edges(it, set).size();
      }
   };

protected:
   template <typename Source>
   class masquerade_node_container
      : public masquerade_container<Source,
                                    mlist< OperationTag< node_accessor<typename container_traits<Source>::iterator> >,
                                           IteratorConstructorTag< subgraph_node_access_constructor > > > {};
public:
   typedef masquerade_node_container<std::conditional_t<attrib<GraphRef>::is_const,
                                                        typename graph_type::const_node_container_ref,
                                                        typename graph_type::node_container_ref>>
      node_container;
   typedef masquerade_node_container<typename graph_type::const_node_container_ref>
      const_node_container;
   typedef typename inherit_const<node_container, GraphRef>::type& node_container_ref;
   typedef const const_node_container& const_node_container_ref;

   typedef masquerade_container<std::conditional_t<attrib<GraphRef>::is_const,
                                                   typename graph_type::const_out_edge_list_container_ref,
                                                   typename graph_type::out_edge_list_container_ref>,
                                OperationTag< edge_list_operation > >
      out_edge_list_container;
   typedef masquerade_container<typename graph_type::const_out_edge_list_container_ref,
                                OperationTag< edge_list_operation > >
      const_out_edge_list_container;
   typedef typename inherit_const<out_edge_list_container, GraphRef>::type& out_edge_list_container_ref;
   typedef const const_out_edge_list_container& const_out_edge_list_container_ref;

   typedef masquerade_container<std::conditional_t<attrib<GraphRef>::is_const,
                                                   typename graph_type::const_in_edge_list_container_ref,
                                                   typename graph_type::in_edge_list_container_ref>,
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

template <typename GraphRef, typename SetRef, typename Params>
struct IndexedSubgraph_random_access {
   static constexpr bool enabled= container_traits<typename deref<GraphRef>::type::node_container_ref>::is_random &&
                                  (!tagged_list_extract_integral<Params, RenumberTag>(false) ||
                                   subset_classifier::index_helper<masquerade<Nodes, GraphRef>, SetRef, false>::random);
   typedef IndexedSubgraph_random_access_methods<GraphRef, SetRef, Params, enabled> type;
};

template <typename GraphRef, typename SetRef, typename Params=mlist<>>
class IndexedSubgraph
   : public IndexedSubgraph_base<GraphRef, SetRef, Params>
   , public IndexedSubgraph_random_access<GraphRef, SetRef, Params>::type
   , public GenericGraph< IndexedSubgraph<GraphRef, SetRef, Params>, typename deref<GraphRef>::type::dir > {
   using base_t = IndexedSubgraph_base<GraphRef, SetRef, Params>;
   template <typename, typename, typename, bool> friend class IndexedSubgraph_random_access_methods;
protected:
   bool has_gaps_impl(std::false_type) const { return true; }
   bool has_gaps_impl(std::true_type) const { return this->get_node_set().front() != 0; }

   Int dim_impl(std::false_type) const { return this->get_graph().dim(); }
   Int dim_impl(std::true_type) const { return nodes(); }

public:
   using IndexedSubgraph_base<GraphRef, SetRef, Params>::IndexedSubgraph_base;

   Int nodes() const
   {
      return this->get_node_set().size();
   }
   Int edges() const;
   Int dim() const
   {
      return dim_impl(typename base_t::renumber_nodes());
   }

   bool has_gaps() const
   {
      return !base_t::renumber_nodes::value && has_gaps_impl(std::is_same<pure_type_t<SetRef>, sequence>());
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

template <typename GraphRef, typename SetRef, typename Params>
Int IndexedSubgraph<GraphRef, SetRef, Params>::edges() const
{
   Int cnt = 0;
   for (auto n_it = entire(pm::nodes(*this)); !n_it.at_end(); ++n_it)
      cnt += n_it.out_degree();
   if (deref<GraphRef>::type::dir::value) cnt /= 2;
   return cnt;
}

template <typename GraphRef, typename SetRef, typename Params>
class IndexedSubgraph_random_access_methods<GraphRef, SetRef, Params, true> {
   typedef IndexedSubgraph<GraphRef, SetRef, Params> master;
   typedef IndexedSubgraph_base<GraphRef, SetRef, Params> base;
   static const bool is_directed_local= deref<GraphRef>::type::dir::value==graph::Directed::value;

   master& me() { return static_cast<master&>(*this); }
   const master& me() const { return static_cast<const master&>(*this); }

   Int node_index(Int n, std::false_type) const
   {
      assert(me().get_node_set().contains(n));
      return n;
   }
   Int node_index(Int n, std::true_type) const
   {
      return me().get_node_set()[n];
   }
   Int node_index(Int n) const
   {
      return node_index(n, typename base::renumber_nodes());
   }
public:
   typename base::out_edge_list out_edges(Int n)
   {
      return typename base::out_edge_list(me().get_graph().out_edges(node_index(n)), me().get_node_set());
   }
   typename base::const_out_edge_list out_edges(Int n) const
   {
      return typename base::const_out_edge_list(me().get_graph().out_edges(node_index(n)), me().get_node_set());
   }
   typename base::in_edge_list in_edges(Int n)
   {
      return typename base::in_edge_list(me().get_graph().in_edges(node_index(n)), me().get_node_set());
   }
   typename base::const_in_edge_list in_edges(Int n) const
   {
      return typename base::const_in_edge_list(me().get_graph().in_edges(node_index(n)), me().get_node_set());
   }

   Int out_degree(Int n) const
   {
      return out_edges(n).size();
   }
   Int in_degree(Int n) const
   {
      return in_edges(n).size();
   }
   Int degree(Int n) const
   {
      Int d = out_degree(n);
      if (is_directed_local) d += in_degree(n);
      return d;
   }

   typename base::out_adjacent_node_list out_adjacent_nodes(Int n) const
   {
      return typename base::out_adjacent_node_list(me().get_graph().out_adjacent_nodes(node_index(n)), me().get_node_set());
   }
   typename base::in_adjacent_node_list in_adjacent_nodes(Int n) const
   {
      return typename base::in_adjacent_node_list(me().get_graph().in_adjacent_nodes(node_index(n)), me().get_node_set());
   }

   Int edge(Int n1, Int n2)
   {
      return me().get_graph().edge(node_index(n1), node_index(n2));
   }
   Int edge(Int n1, Int n2) const
   {
      return me().get_graph().edge(node_index(n1), node_index(n2));
   }

   bool edge_exists(Int n1, Int n2) const
   {
      return me().get_graph().edge_exists(node_index(n1), node_index(n2));
   }
   void delete_edge(Int n1, Int n2)
   {
      me().get_graph().delete_edge(node_index(n1), node_index(n2));
   }
};

template <typename GraphRef, typename SetRef, typename Params>
const IndexedSubgraph<GraphRef, SetRef, typename mtagged_list_replace<Params, RenumberTag<std::true_type>>::type>&
renumber_nodes(const IndexedSubgraph<GraphRef, SetRef, Params>& G)
{
   return reinterpret_cast<const IndexedSubgraph<GraphRef, SetRef, typename mtagged_list_replace<Params, RenumberTag<std::true_type>>::type>&>(G);
}

template <typename GraphRef, typename SetRef, typename Params>
struct spec_object_traits< IndexedSubgraph<GraphRef, SetRef, Params> >
   : spec_object_traits<is_opaque> {
   static constexpr bool
      is_temporary = true,
      is_always_const = is_effectively_const<GraphRef>::value;
   static constexpr int is_resizeable = 0;
};

template <typename TGraph, typename IndexSet,
          typename = std::enable_if_t<is_generic_graph<TGraph>::value &&
                                      isomorphic_to_container_of<pure_type_t<IndexSet>, Int>::value>>
auto induced_subgraph(TGraph&& G, IndexSet&& node_indices)
     // gcc 5 needs this crutch
     -> IndexedSubgraph<unwary_t<TGraph>, add_const_t<unwary_t<IndexSet>>>
{
   if (POLYMAKE_DEBUG || is_wary<TGraph>()) {
      if (!set_within_range(node_indices, G.top().nodes()))
         throw std::runtime_error("induced_subgraph - node indices out of range");
   }
   using result_type = IndexedSubgraph<unwary_t<TGraph>, add_const_t<unwary_t<IndexSet>>>;
   return result_type(unwary(std::forward<TGraph>(G)),
                      prepare_index_set(std::forward<IndexSet>(node_indices), [&](){ return unwary(G).dim(); }));
}

} // end namespace pm

#endif // POLYMAKE_INDEXED_GRAPH_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
