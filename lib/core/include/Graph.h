/* Copyright (c) 1997-2019
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

/** @file Graph.h
    @brief Implementation of pm::graph::Graph class
*/

#ifndef POLYMAKE_GRAPH_H
#define POLYMAKE_GRAPH_H

#include "polymake/internal/sparse2d.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/internal/assoc.h"
#include "polymake/internal/converters.h"
#include "polymake/SelectedSubset.h"
#include "polymake/GenericGraph.h"
#include "polymake/EmbeddedList.h"
#include "polymake/hash_map"
#include "polymake/list"
#include "polymake/vector"
#include "polymake/meta_list.h"
#include <cassert>

namespace pm {
namespace graph {

using pm::sparse2d::restriction_kind;
using pm::sparse2d::full;
using pm::sparse2d::dying;
using pm::sparse2d::relocate;
using pm::relocate;

template <typename TDir> class Table;
template <typename TDir, restriction_kind restriction=full> struct node_entry;
template <typename Traits> struct dir_permute_entries;
template <typename Traits> struct undir_permute_entries;

template <typename TDir, typename E, typename... TParams> class NodeMap;
template <typename TDir, typename E, typename... TParams> class EdgeMap;
template <typename TDir, typename E, typename... TParams> class NodeHashMap;
template <typename TDir, typename E, typename... TParams> class EdgeHashMap;

struct edge_agent_base {
   int n_edges, n_alloc;
   edge_agent_base() : n_edges(0), n_alloc(0) {}

   static const int bucket_shift=8, bucket_size=1<<bucket_shift, bucket_mask=bucket_size-1;
   static int min_buckets(int b) { return b>=10 ? b : 10; }

   template <typename MapList>
   bool extend_maps(MapList& maps);
};

template <typename TDir>
struct edge_agent : edge_agent_base {
   Table<TDir>* table;
   edge_agent() : table(nullptr) {}

   typedef sparse2d::cell<int> Cell;

   template <bool for_copy>
   void init(Table<TDir> *t, bool_constant<for_copy>);

   template <typename NumberConsumer>
   void renumber(const NumberConsumer& nc);

   void reset() { n_alloc=0; table=nullptr; }

   void added(Cell *c)
   {
      if (table)
         table->_edge_added(*this,c);
      else
         n_alloc=0;
      ++n_edges;
   }
   void removed(Cell *c)
   {
      --n_edges;
      if (table)
         table->_edge_removed(c);
      else
         n_alloc=0;
   }
};

template <typename TDir, bool TOut_edges>
class it_traits : public sparse2d::it_traits<int, TOut_edges, TDir::value> {
   typedef sparse2d::it_traits<int, TOut_edges, TDir::value> base_t;
public:
   it_traits(int index_arg=0) : base_t(index_arg) {}

   AVL::Ptr<typename base_t::Node>& link(typename base_t::Node *n, AVL::link_index X) const
   {
      return (TDir::value && n->key<0) ? n->links[X-AVL::L] : base_t::link(n,X);
   }

   const it_traits& get_it_traits() const { return *this; }
};

template <typename TDir, bool TOut_edges /* =false */, restriction_kind restriction>
class traits_base : public it_traits<TDir, TOut_edges> {
public:
   typedef it_traits<TDir, TOut_edges> traits_for_iterator;
   typedef typename traits_for_iterator::Node Node;
protected:
   mutable AVL::Ptr<Node> root_links[3];
public:
   typedef int mapped_type;

   static const bool
      symmetric=TDir::value,
      row_oriented=TOut_edges /* =false */,
      allow_multiple=TDir::multigraph;

   typedef AVL::tree< sparse2d::traits<traits_base, symmetric, restriction> > own_tree;
   typedef AVL::tree< sparse2d::traits<traits_base<TDir, (!symmetric && !row_oriented), restriction>, symmetric, restriction> >
      cross_tree;
protected:
   typedef node_entry<TDir, restriction> entry;
   typedef sparse2d::ruler<entry, edge_agent<TDir> > own_ruler;
   typedef own_ruler cross_ruler;

   Node* head_node() const
   {
      return reinterpret_cast<Node*>(const_cast<traits_base*>(this));
   }

   const entry& get_node_entry() const
   {
      return *entry::reverse_cast(static_cast<const own_tree*>(this));
   }
   entry& get_node_entry()
   {
      return *entry::reverse_cast(static_cast<own_tree*>(this));
   }

   const cross_ruler& get_cross_ruler() const
   {
      return get_node_entry().get_ruler();
   }
   cross_ruler& get_cross_ruler()
   {
      return get_node_entry().get_ruler();
   }

   int visit_by_copy(Node* n) const
   {
      return 2*this->get_line_index() - n->key;
   }

   void notify_add(Node* n)
   {
      get_cross_ruler().prefix().added(n);
   }
   void notify_remove(Node* n)
   {
      get_cross_ruler().prefix().removed(n);
   }
public:
   typedef int arg_type;
   traits_base(int index_arg) : traits_for_iterator(index_arg) {}

   const cross_tree& get_cross_tree(int i) const
   {
      return get_cross_ruler()[i].out();
   }
   cross_tree& get_cross_tree(int i)
   {
      return get_cross_ruler()[i].out();
   }

   friend class Table<TDir>;
   template <typename> friend struct sparse2d::sym_permute_entries;
   template <typename> friend struct dir_permute_entries;
   template <typename> friend struct undir_permute_entries;
};

template <typename TDir, restriction_kind restriction>
class traits_base<TDir, true, restriction> {
protected:
   typedef sparse2d::cell<int> Node;

   mutable AVL::Ptr<Node> root_links[3];
public:
   typedef int mapped_type;

   static const bool
      symmetric=false,
      row_oriented=true,
      allow_multiple=TDir::multigraph;

   typedef it_traits<TDir, true> traits_for_iterator;
   typedef traits_base<TDir, false, restriction> cross_traits_base;

   static AVL::Ptr<Node>& link(Node* n, AVL::link_index X)
   {
      return n->links[X-AVL::L + 3];
   }

   int get_line_index() const
   {
      return get_node_entry().in().get_line_index();
   }
   traits_for_iterator get_it_traits() const { return get_line_index(); }

   typedef AVL::tree< sparse2d::traits<traits_base, symmetric, restriction> > own_tree;
   typedef AVL::tree< sparse2d::traits<cross_traits_base, symmetric, restriction> > cross_tree;
protected:
   typedef node_entry<TDir, restriction> entry;
   typedef sparse2d::ruler<entry, edge_agent<TDir> > own_ruler;
   typedef own_ruler cross_ruler;

   Node* head_node() const
   {
      return reinterpret_cast<Node*>(reinterpret_cast<char*>(const_cast<traits_base*>(this))-
                                     sizeof(cross_traits_base));
   }

   const entry& get_node_entry() const
   {
      return *entry::reverse_cast(static_cast<const own_tree*>(this));
   }
   entry& get_node_entry()
   {
      return *entry::reverse_cast(static_cast<own_tree*>(this));
   }

   const cross_ruler& get_cross_ruler() const
   {
      return get_node_entry().get_ruler();
   }
   cross_ruler& get_cross_ruler()
   {
      return get_node_entry().get_ruler();
   }

   int visit_by_copy(Node* n) const
   {
      return 2*get_line_index() - n->key;
   }

   void notify_add(Node* n)
   {
      get_cross_ruler().prefix().added(n);
   }
   void notify_remove(Node* n)
   {
      get_cross_ruler().prefix().removed(n);
   }

public:
   typedef const traits_base& arg_type;

   const cross_tree& get_cross_tree(int i) const
   {
      return get_cross_ruler()[i].in();
   }
   cross_tree& get_cross_tree(int i)
   {
      return get_cross_ruler()[i].in();
   }

   static void prepare_move_between_trees(Node* n, const int old_line_index, const int new_line_index)
   {
      n->key += new_line_index - old_line_index;
   }

   friend class Table<TDir>;
};

template <typename TDir, restriction_kind restriction, bool _symmetric=TDir::value /* =true */>
struct node_entry_trees {
   typedef AVL::tree< sparse2d::traits<traits_base<TDir, false, restriction>, _symmetric, restriction> >
      out_tree_type;
   typedef out_tree_type in_tree_type;
   out_tree_type _out;

   explicit node_entry_trees(int index_arg) : _out(index_arg) {}

   out_tree_type& out() { return _out; }
   const out_tree_type& out() const { return _out; }
   out_tree_type& in() { return _out; }
   const out_tree_type& in() const { return _out; }
   int degree() const { return _out.size(); }

   out_tree_type& cross_tree(out_tree_type*) { return _out; }

   static const node_entry<TDir, restriction>*
   reverse_cast(const out_tree_type* t)
   {
      return static_cast<const node_entry<TDir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_out));
   }
   static node_entry<TDir, restriction>*
   reverse_cast(out_tree_type* t)
   {
      return static_cast<node_entry<TDir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_out));
   }
};

template <typename TDir, restriction_kind restriction>
struct node_entry_trees<TDir, restriction, false> {
   typedef AVL::tree< sparse2d::traits<traits_base<TDir, true, restriction>, false, restriction> >
      out_tree_type;
   typedef AVL::tree< sparse2d::traits<traits_base<TDir, false, restriction>, false, restriction> >
      in_tree_type;
   in_tree_type _in;
   out_tree_type _out;

   explicit node_entry_trees(int index_arg) : _in(index_arg) {}

   out_tree_type& out() { return _out; }
   const out_tree_type& out() const { return _out; }
   in_tree_type& in() { return _in; }
   const in_tree_type& in() const { return _in; }
   int degree() const { return _out.size()+_in.size(); }

   out_tree_type& cross_tree(in_tree_type*)  { return _out; }
   in_tree_type&  cross_tree(out_tree_type*) { return _in; }

   static const node_entry<TDir, restriction>*
   reverse_cast(const out_tree_type* t)
   {
      return static_cast<const node_entry<TDir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_out));
   }
   static node_entry<TDir, restriction>*
   reverse_cast(out_tree_type* t)
   {
      return static_cast<node_entry<TDir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_out));
   }
   static const node_entry<TDir, restriction>*
   reverse_cast(const in_tree_type* t)
   {
      return static_cast<const node_entry<TDir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_in));
   }
   static node_entry<TDir, restriction>*
   reverse_cast(in_tree_type* t)
   {
      return static_cast<node_entry<TDir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_in));
   }
};

template <typename TDir, restriction_kind restriction>
struct node_entry
   : public node_entry_trees<TDir, restriction> {
   typedef TDir dir;
   typedef sparse2d::ruler<node_entry, edge_agent<TDir> > ruler;

   explicit node_entry(int index_arg) : node_entry_trees<TDir, restriction>(index_arg) {}

   int get_line_index() const { return this->in().get_line_index(); }

   const ruler& get_ruler() const
   {
      return ruler::reverse_cast(this, get_line_index());
   }
   ruler& get_ruler()
   {
      return ruler::reverse_cast(this, get_line_index());
   }

   friend void relocate(node_entry* from, node_entry* to)
   {
      relocate(&(from->out()), &(to->out()));
      if (TDir::value==Directed::value) relocate(&(from->in()), &(to->in()));
   }
};

template <typename Table>
struct dir_permute_entries {
   typedef typename Table::ruler ruler;
   typedef typename Table::entry entry_t;
   typedef typename Table::out_tree_type out_tree_t;
   typedef typename Table::in_tree_type in_tree_t;
   typedef typename out_tree_t::Node Node;

   explicit dir_permute_entries(int& free_node_id)
      : free_node_id_ptr(&free_node_id) {}

   static void relocate(entry_t* from, entry_t* to)
   {
      relocate_tree(&from->_out, &to->_out, std::false_type());
      relocate_tree(&from->_in, &to->_in, std::false_type());
   }

   static void complete_in_trees(ruler* R)
   {
      int nfrom=0;
      for (entry_t& entry : *R) {
         for (auto e=entry._out.begin(); !e.at_end(); ++e) {
            Node* node=e.operator->();
            (*R)[node->key-nfrom]._in.push_back_node(node);
         }
         ++nfrom;
      }
   }

   void operator()(ruler* Rold, ruler* R)
   {
      inv_perm_store.resize(R->size(), -1);
      int nto=0;
      for (entry_t& entry : *R) {
         const int old_n=entry._in.line_index;
         if (old_n >= 0)
            inv_perm_store[old_n]=nto;
         ++nto;
      }

      nto=0;
      for (entry_t& entry : *R) {
         const int old_nto=entry._in.line_index;
         if (old_nto >= 0) {
            entry._in.line_index=nto;
            for (auto e=(*Rold)[old_nto]._in.begin(); !e.at_end(); ++e) {
               Node* node=e.operator->();
               const int old_nfrom=node->key-old_nto, nfrom=inv_perm_store[old_nfrom];
               node->key=nfrom+nto;
               (*R)[nfrom]._out.push_back_node(node);
            }
         } else {
            *free_node_id_ptr=~nto;
            free_node_id_ptr=&entry._in.line_index;
         }
         ++nto;
      }
      *free_node_id_ptr=std::numeric_limits<int>::min();

      complete_in_trees(R);
   }

   template <typename Perm, typename InvPerm>
   void copy(const ruler* R_src, ruler* R_dst, const Perm& perm, const InvPerm& inv_perm)
   {
      const int n=R_dst->size();
      auto p_it=perm.begin();
      for (int dst_nto=0; dst_nto < n; ++dst_nto, ++p_it) {
         const int src_nto=*p_it;
         const in_tree_t& src_in_tree=(*R_src)[src_nto]._in;
         if (src_in_tree.line_index >= 0) {
            for (auto e=src_in_tree.begin(); !e.at_end(); ++e) {
               const Node* node=e.operator->();
               const int src_nfrom=node->key-src_nto;
               const int dst_nfrom=inv_perm[src_nfrom];
               out_tree_t& t=(*R_dst)[dst_nfrom]._out;
               t.push_back_node(new(t.allocate_node()) Node(dst_nfrom+dst_nto));
            }
         } else {
            *free_node_id_ptr=~dst_nto;
            free_node_id_ptr=&(*R_dst)[dst_nto]._in.line_index;
         }
      }
      *free_node_id_ptr=std::numeric_limits<int>::min();

      complete_in_trees(R_dst);
   }

   std::vector<int> inv_perm_store;
   int* free_node_id_ptr;
};

template <typename Traits>
struct undir_permute_entries
   : sparse2d::sym_permute_entries<Traits> {

   using typename Traits::entry_t;

   explicit undir_permute_entries(int& free_node_id)
      : free_node_id_ptr(&free_node_id) {}

   static void relocate(entry_t* from, entry_t* to)
   {
      relocate_tree(&from->_out, &to->_out, std::false_type());
   }

   void deleted_node(entry_t& entry, int n)
   {
      *free_node_id_ptr=~n;
      free_node_id_ptr=&entry._out.line_index;
   }

   void finalize_deleted_nodes()
   {
      *free_node_id_ptr=std::numeric_limits<int>::min();
   }

   int* free_node_id_ptr;
};

struct NodeMapBase {
   ptr_pair<NodeMapBase> ptrs;
   long refc;
   void* _table;

   NodeMapBase() : refc(1), _table(0) {}

   virtual ~NodeMapBase() {}
   virtual void init()=0;
   virtual void reset(int n=0)=0;
   virtual void resize(size_t n_alloc_new, int n, int nnew)=0;
   virtual void shrink(size_t n_alloc_new, int n)=0;
   virtual void move_entry(int n_from, int n_to)=0;
   virtual void revive_entry(int n)=0;
   virtual void delete_entry(int n)=0;
   virtual void permute_entries(const std::vector<int>& inv_perm)=0;
};

template <typename> class EdgeMapDataAccess;

struct EdgeMapBase {
   ptr_pair<EdgeMapBase> ptrs;
   long refc;
   void* _table;

   EdgeMapBase() : refc(1), _table(0)  {}
   virtual ~EdgeMapBase() {}
   virtual bool is_detachable() const = 0;
   virtual void reset()=0;

   virtual void revive_entry(int e)=0;
   virtual void delete_entry(int e)=0;
   virtual void realloc(size_t n_alloc)=0;
   virtual void add_bucket(int n)=0;
};

struct EdgeMapDenseBase : public EdgeMapBase {
   void **buckets;
   size_t n_alloc;

   EdgeMapDenseBase() : buckets(nullptr) {}

   void alloc(size_t n)
   {
      n_alloc = n;
      buckets = new void*[n];
      std::fill_n(buckets, n, nullptr);
   }
   void realloc(size_t new_n_alloc)
   {
      if (new_n_alloc > n_alloc) {
         void** old_buckets = buckets;
         buckets = new void*[new_n_alloc];
         std::fill_n(std::copy(old_buckets, old_buckets + n_alloc, buckets), new_n_alloc - n_alloc, nullptr);
         delete[] old_buckets;
         n_alloc = new_n_alloc;
      }
   }

   void destroy()
   {
      delete[] buckets;
      buckets = nullptr;
      n_alloc = 0;
   }
};

template <typename MapList>
bool edge_agent_base::extend_maps(MapList& maps)
{
   if (n_edges & bucket_mask) return false;

   const int new_bucket=n_edges >> bucket_shift;
   if (new_bucket >= n_alloc) {
      n_alloc += min_buckets(n_alloc / 5);
      for (auto& map : maps) {
         map.realloc(n_alloc);
         map.add_bucket(new_bucket);
      }
   } else {
      for (auto& map : maps)
         map.add_bucket(new_bucket);
   }
   return true;
}

template <typename Data>
class EdgeMapDataAccess {
public:
   typedef int argument_type;
   typedef Data& result_type;

   static Data* index2addr(void **buckets, int i)
   {
      return reinterpret_cast<Data*>(buckets[i>>edge_agent_base::bucket_shift])+(i&edge_agent_base::bucket_mask);
   }
   result_type operator() (int i) const
   {
      return *index2addr(buckets,i);
   }

   EdgeMapDataAccess(void** arg=0) : buckets(arg) {}
   EdgeMapDataAccess(const EdgeMapDataAccess<typename attrib<Data>::minus_const>& op) : buckets(op.buckets) {}
protected:
   void **buckets;
};

} // end namespace graph

template <typename Data>
struct operation_cross_const_helper< graph::EdgeMapDataAccess<Data> > {
   typedef graph::EdgeMapDataAccess<typename attrib<Data>::minus_const> operation;
   typedef graph::EdgeMapDataAccess<typename attrib<Data>::plus_const> const_operation;
};

namespace graph {

template <typename TDir>
class Table {
public:
   typedef TDir dir;
   static const bool is_directed=dir::value==Directed::value;
   typedef node_entry<dir> entry;
   typedef typename entry::ruler ruler;
protected:
   ruler *R;
   typedef sparse2d::cell<int> Cell;

   typedef EmbeddedList<NodeMapBase, &NodeMapBase::ptrs> node_map_list;
   typedef EmbeddedList<EdgeMapBase, &EdgeMapBase::ptrs> edge_map_list;
   mutable node_map_list node_maps;
   mutable edge_map_list edge_maps;
   std::vector<int> free_edge_ids;
   int n_nodes, free_node_id;

   friend struct edge_agent<dir>;
   friend class Graph<dir>;
public:
   Table()
      : R(ruler::construct(0)), n_nodes(0), free_node_id(std::numeric_limits<int>::min()) {}

   explicit Table(int n)
      : R(ruler::construct(n)), n_nodes(n), free_node_id(std::numeric_limits<int>::min()) {}

   Table(const Table& t)
      : R(ruler::construct(*t.R)), n_nodes(t.n_nodes), free_node_id(t.free_node_id)
   {
      R->prefix().n_edges=t.edges();
   }

protected:
   template <typename TSet>
   static std::enable_if_t<check_container_feature<TSet, sparse_compatible>::value, int>
   get_dim_of(const TSet& s)
   {
      return s.dim();
   }
   template <typename TSet>
   static std::enable_if_t<!check_container_feature<TSet, sparse_compatible>::value, int>
   get_dim_of(const TSet& s)
   {
      return s.empty() ? 0 : s.back()+1;
   }
public:
   template <typename TSet>
   explicit Table(const GenericSet<TSet>& s)
      : R(ruler::construct(get_dim_of(s.top())))
      , n_nodes(R->size())
      , free_node_id(std::numeric_limits<int>::min())
   {
      if (!std::is_same<TSet, sequence>::value || s.top().size()!=n_nodes)
         init_delete_nodes(sequence(0,n_nodes)-s);
   }

   template <typename TSet>
   Table(const GenericSet<TSet>& s, int dim)
      : R(ruler::construct(dim))
      , n_nodes(dim)
      , free_node_id(std::numeric_limits<int>::min())
   {
      if (!std::is_same<TSet, sequence>::value || s.top().size()!=n_nodes)
         init_delete_nodes(sequence(0,n_nodes)-s);
   }

protected:
   void detach_node_maps()
   {
      for (auto it=entire(node_maps); !it.at_end(); ) {
         NodeMapBase* m=it.operator->();  ++it;
         m->reset(); m->_table=nullptr;
         detach(*m);
      }
   }
   void detach_edge_maps()
   {
      for (auto it=entire(edge_maps); !it.at_end(); ) {
         EdgeMapBase* m=it.operator->();  ++it;
         m->reset(); m->_table=nullptr;
         detach(*m);
      }
   }
public:
   ~Table()
   {
      detach_node_maps();
      detach_edge_maps();
      typedef typename node_entry<dir,dying>::ruler dying_ruler;
      dying_ruler::destroy(reinterpret_cast<dying_ruler*>(R));
   }

   Table& operator= (const Table& t)
   {
      this->~Table();
      new(this) Table(t);
      return *this;
   }

   void swap(Table& t)
   {
      std::swap(R,t.R);
      node_maps.swap(t.node_maps);
      edge_maps.swap(t.edge_maps);
      std::swap(n_nodes, t.n_nodes);
      std::swap(free_node_id, t.free_node_id);
      std::swap(free_edge_ids, t.free_edge_ids);
      for (auto& map : node_maps)
         map._table=this;
      for (auto& map : t.node_maps)
         map._table=&t;
      for (auto& map : edge_maps)
         map._table=this;
      for (auto& map : t.edge_maps)
         map._table=&t;
   }

   void clear(int n=0)
   {
      for (auto& map : node_maps) map.reset(n);
      for (auto& map : edge_maps) map.reset();
      R->prefix().table=nullptr;
      R=ruler::resize_and_clear(R,n);
      edge_agent<dir>& h=R->prefix();
      if (!edge_maps.empty()) h.table=this;
      h.n_alloc=0;
      h.n_edges=0;
      n_nodes=n;
      if (n) for (auto& map : node_maps) map.init();
      free_node_id=std::numeric_limits<int>::min();
      free_edge_ids.clear();
   }

   struct shared_clear {
      int n;
      shared_clear(int n_arg) : n(n_arg) {}

      void operator() (void *p, const Table&) const { new(p) Table(n); }
      void operator() (Table& t) const { t.clear(n); }
   };

   typedef typename entry::out_tree_type out_tree_type;
   typedef typename entry::in_tree_type in_tree_type;

   int dim() const { return R->size(); }
   int nodes() const { return n_nodes; }
   int edges() const { return R->prefix().n_edges; }

   bool node_exists(int n) const { return (*R)[n].get_line_index()>=0; }
   bool node_out_of_range(int n) const { return n<0 || n>=R->size(); }
   bool invalid_node(int n) const { return node_out_of_range(n) || !node_exists(n); }

   entry& operator[] (int n) { return (*R)[n]; }
   const entry& operator[] (int n) const { return (*R)[n]; }

protected:
   int revive_node()
   {
      int n=~free_node_id;
      entry& e=(*R)[n];
      free_node_id=e.in().line_index;
      e.in().line_index=n;
      for (auto& map : node_maps) map.revive_entry(n);
      ++n_nodes;
      return n;
   }

   // is only called when the maps do not contain any gaps
   void _resize(int nnew)
   {
      R=ruler::resize(R,nnew);
      for (auto& map : node_maps) map.resize(R->max_size(), n_nodes, nnew);
      n_nodes=nnew;
   }

   void _edge_added(edge_agent<dir>& h, Cell *c)
   {
      int id;
      if (free_edge_ids.empty()) {
         id=h.n_edges;
         if (h.extend_maps(edge_maps)) {
            c->data=id;
            return;
         }
      } else {
         id=free_edge_ids.back();
         free_edge_ids.pop_back();
      }
      c->data=id;
      for (auto& map : edge_maps) map.revive_entry(id);
   }

   void _edge_removed(Cell *c)
   {
      const int id=c->data;
      for (auto& map : edge_maps) map.delete_entry(id);
      free_edge_ids.push_back(id);
   }
public:
   int add_node()
   {
      if (free_node_id != std::numeric_limits<int>::min())
         return revive_node();
      int n=R->size();
      _resize(n+1);
      return n;
   }

   void delete_node(int n)
   {
      entry& e=(*R)[n];
      e.out().clear();
      if (is_directed) e.in().clear();
      e.in().line_index=free_node_id;
      free_node_id=~n;
      for (auto& map : node_maps) map.delete_entry(n);
      --n_nodes;
   }

protected:
   template <typename List>
   void init_delete_nodes(const List& l)
   {
      for (auto it = entire(l); !it.at_end(); ++it) {
         const int n=*it;
         entry& e=(*R)[n];
         e.in().line_index=free_node_id;
         free_node_id=~n;
         --n_nodes;
      }
   }

   template <typename Tree>
   void renumber_nodes_in_edges(Tree& t, int /*nnew*/, int diff, Directed)
   {
      for (auto e=entire(t); !e.at_end(); ++e)
         e->key -= diff;
   }
   void renumber_nodes_in_edges(in_tree_type& t, int nnew, int diff, Undirected)
   {
      const int diag=2*t.line_index;
      for (auto e=entire(t); !e.at_end(); ) {
         Cell& c=*e; ++e;
         c.key -= diff << (c.key==diag);
      }
      t.line_index=nnew;
   }

   template <bool delete_isolated>
   struct squeeze_node_chooser {
      int operator() (const entry& t) const
      {
         if (t.get_line_index()<0) return -1;
         return delete_isolated && t.degree()==0;
      }
   };

   struct resize_node_chooser {
      int nnew;

      int operator() (entry& t) const
      {
         int n=t.get_line_index();
         if (n<0) return -1;
         if (n>=nnew) {
            t.in().clear();
            if (is_directed) t.out().clear();
            return 1;
         }
         return 0;
      }

      resize_node_chooser(int n_arg) : nnew(n_arg) {}
   };

   template <typename NumberConsumer, typename NodeChooser>
   void squeeze_nodes(const NumberConsumer& nc, NodeChooser to_delete)
   {
      int n=0, nnew=0;
      for (auto t=R->begin(), end=R->end(); t!=end; ++t, ++n) {
         const int what=to_delete(*t);
         if (what==0) {
            if (int diff=n-nnew) {
               if (is_directed) t->in().line_index=nnew;
               renumber_nodes_in_edges(t->out(), nnew, diff, dir());
               if (is_directed) renumber_nodes_in_edges(t->in(), nnew, diff, dir());
               relocate(t.operator->(), &t[-diff]);
               for (auto& map : node_maps) map.move_entry(n, nnew);
            }
            nc(n, nnew);  ++nnew;
         } else {
            if (what>0) {
               for (auto& map : node_maps) map.delete_entry(n);
               --n_nodes;
            }
            destroy_at(t.operator->());
         }
      }
      if (nnew < n) {
         R=ruler::resize(R, nnew, false);
         for (auto it=entire(node_maps); !it.at_end(); ++it) it->shrink(R->max_size(), nnew);
      }
      free_node_id=std::numeric_limits<int>::min();
   }

public:
   template <typename NumberConsumer>
   void squeeze_nodes(const NumberConsumer& nc)
   {
      squeeze_nodes(nc, squeeze_node_chooser<false>());
   }

   template <typename NumberConsumer>
   void squeeze_edges(const NumberConsumer& nc)
   {
      for (auto& map : edge_maps) {
         if (!map.is_detachable())
            throw std::runtime_error("can't renumber edge IDs - non-trivial data attached");
      }
      R->prefix().renumber(nc);
      detach_edge_maps();
   }

   void resize(int n)
   {
      if (n > n_nodes) {
         while (free_node_id != std::numeric_limits<int>::min()) {
            revive_node();
            if (n==n_nodes) return;
         }
         _resize(n);
      } else if (n < n_nodes) {
         if (free_node_id != std::numeric_limits<int>::min())
            squeeze_nodes(operations::binary_noop(), resize_node_chooser(n));
         else
            _resize(n);
      }
   }

protected:
   struct undir_perm_traits {
      typedef typename Table::ruler ruler;
      typedef out_tree_type tree_t;
      typedef typename Table::entry entry_t;
      static tree_t& tree(entry_t& e) { return e.out(); }
      static const tree_t& tree(const entry_t& e) { return e.out(); }

      static bool is_alive(const entry_t& e) { return e.get_line_index() >= 0; }

      void handle_dead_entry(entry_t& e, int n)
      {
         static_cast<undir_permute_entries<undir_perm_traits>&>(*this).deleted_node(e, n);
      }
      void finalize_dead_entries()
      {
         static_cast<undir_permute_entries<undir_perm_traits>&>(*this).finalize_deleted_nodes();
      }
   };

   dir_permute_entries<Table> permute_entries(Directed)
   {
      return dir_permute_entries<Table>(free_node_id);
   }
   undir_permute_entries<undir_perm_traits> permute_entries(Undirected)
   {
      return undir_permute_entries<undir_perm_traits>(free_node_id);
   }

public:
   template <typename TPerm, typename _inverse>
   void permute_nodes(const TPerm& perm, _inverse)
   {
      auto permuter=permute_entries(dir());
      R=ruler::permute(R, perm, permuter, _inverse());
      for (auto& map : node_maps) map.permute_entries(permuter.inv_perm_store);
   }

   template <typename TPerm, typename TInvPerm>
   void copy_permuted(const Table& src, const TPerm& perm, const TInvPerm& inv_perm)
   {
      permute_entries(dir()).copy(src.R, R, perm, inv_perm);
      n_nodes=src.n_nodes;
      R->prefix().n_edges=src.edges();
   }

#if POLYMAKE_DEBUG
public:
   void check(const char* prefix) const
   {
      for (const entry *r=R->begin(), *end=R->end(); r!=end; ++r) {
         check(r->out(), prefix, "(out)");
         if (is_directed) check(r->in(), prefix, "(in)");
      }
   }
protected:
   template <typename Tree>
   void check(const Tree& t, const char* prefix, const char* direction) const
   {
      std::ostringstream label;
      label << prefix << "node " << t.get_line_index() << direction << ": ";
      t.check(label.str().c_str());
   }
#endif // POLYMAKE_DEBUG
public:
   void attach(NodeMapBase& m) const
   {
      Table *me=const_cast<Table*>(this);
      m._table=me;
      me->node_maps.push_back(m);
   }

   void detach(NodeMapBase& m)
   {
      node_maps.remove(m);
   }

   void attach(EdgeMapBase& m) const
   {
      Table *me=const_cast<Table*>(this);
      m._table=me;
      me->edge_maps.push_back(m);
   }

   void detach(EdgeMapBase& m)
   {
      edge_maps.remove(m);
      if (edge_maps.empty()) {
         R->prefix().reset();
         free_edge_ids.clear();
      }
   }

   ruler& get_ruler() { return *R; }
   const ruler& get_ruler() const { return *R; }

   template <bool for_copy>
   const edge_agent<dir>& get_edge_agent(bool_constant<for_copy> C) const
   {
      edge_agent<dir>& h=R->prefix();
      if (!h.table) h.init(const_cast<Table*>(this), C);
      return h;
   }
};

struct edge_accessor
   : public sparse2d::cell_accessor< sparse2d::cell<int> > {

   template <typename Iterator>
   class mix_in : public Iterator {
   public:
      mix_in() {}

      template <typename SourceIterator, typename suitable=typename suitable_arg_for_iterator<SourceIterator, Iterator>::type>
      mix_in(const SourceIterator& cur_arg)
         : Iterator(prepare_iterator_arg<Iterator>(cur_arg)) {}

      int from_node() const
      {
         return (Iterator::symmetric || Iterator::row_oriented)
                ? this->get_line_index()
                : sparse2d::cell_index_accessor<const Iterator&>()(*this);
      }
      int to_node() const
      {
         return (Iterator::symmetric || Iterator::row_oriented)
                ? sparse2d::cell_index_accessor<const Iterator&>()(*this)
                : this->get_line_index();
      }

   protected:
      int& edge_id() const { return (**this).data; }

      template <typename> friend struct edge_agent;
   };
};

class truncate_after_index
{
public:
   typedef void argument_type;
   typedef bool result_type;

   truncate_after_index(int v=0) : val(v) {}

   template <typename Iterator>
   bool operator() (const Iterator& it) const { return it.index() <= val; }

private:
   int val;
};

template <typename Tree>
class incident_edge_list
   : public modified_tree< incident_edge_list<Tree>,
                           mlist< OperationTag< pair< edge_accessor, BuildUnaryIt<sparse2d::cell_index_accessor> > >,
                                  HiddenTag< Tree > > > {
   typedef modified_tree<incident_edge_list> base_t;
   template <typename> friend class Graph;
protected:
   ~incident_edge_list() = delete;

   template <typename Iterator>
   void copy(Iterator src)
   {
      auto dst=this->begin();
      for (; !src.at_end(); ++src) {
         int idiff=1;
         while (!dst.at_end()) {
            idiff=dst.index()-src.index();
            if (idiff<0)
               this->erase(dst++);
            else
               break;
            idiff=1;
         }
         if (idiff>0)
            this->insert(dst, src.index());
         else
            ++dst;
      }
      while (!dst.at_end()) this->erase(dst++);
   }

   // merge not needed
   template <typename Iterator>
   bool init_from_set(Iterator src, std::false_type)
   {
      auto dst=this->end();
      const int diag= Tree::symmetric ? this->hidden().get_line_index() : 0;
      for (; !src.at_end(); ++src) {
         const int i=*src;
         if (Tree::symmetric && i > diag) return true;
         this->insert(dst, i);
      }
      return false;
   }

   // merge needed
   template <typename Iterator>
   void init_from_set(Iterator src, std::true_type)
   {
      auto dst=this->begin();
      for (; !src.at_end(); ++src) {
         const int i=*src;
         int idiff=1;
         while (!dst.at_end()) {
            idiff=dst.index()-i;
            if (idiff<=0) ++dst;
            if (idiff>=0) break;
            idiff=1;
         }
         if (idiff>0) this->insert(dst, i);
      }
   }

   template <typename Iterator, typename need_merge>
   void init_from_edge_list(Iterator src, need_merge, std::false_type)
   {
      init_from_set(make_unary_transform_iterator(src, BuildUnaryIt<operations::index2element>()), need_merge());
   }

   template <typename Iterator, typename need_merge>
   void init_from_edge_list(Iterator src, need_merge, std::true_type)
   {
      init_from_set(make_equal_range_contractor(make_unary_transform_iterator(src, BuildUnaryIt<operations::index2element>())), need_merge());
   }

   template <typename Input>
   void init_multi_from_sparse(Input& src)
   {
      const int d = dim();
      if (!src.get_option(TrustedValue<std::true_type>()) &&
          src.get_dim(false) != d)
         throw std::runtime_error("multigraph input - dimension mismatch");

      auto dst = this->end();
      const int diag = Tree::symmetric ? this->hidden().get_line_index() : 0;

      while (!src.at_end()) {
         const int index = src.index(d);
         if (Tree::symmetric && index > diag) {
            src.skip_item();
            src.skip_rest();
            break;
         }
         int count;
         for (src >> count; count; --count)
            this->insert(dst, index);
      }
   }

   template <typename Input>
   void init_multi_from_dense(Input& src)
   {
      if (!src.get_option(TrustedValue<std::true_type>()) &&
          src.size() != dim())
         throw std::runtime_error("multigraph input - dimension mismatch");

      auto dst = this->end();
      const int diag = Tree::symmetric ? this->hidden().get_line_index() : 0;

      for (int i=0; !src.at_end(); ++i) {
         if (Tree::symmetric && i > diag) {
            src.skip_rest();
            break;
         }
         int count;
         for (src >> count; count; --count)
            this->insert(dst, i);
      }
   }

public:
   static const bool multigraph=Tree::allow_multiple;

   template <typename Input>
   std::enable_if_t<!multigraph, typename mproject2nd<Input, void>::type>
   read(Input& in)
   {
      typedef typename Input::template list_cursor< std::list<int> >::type cursor;
      cursor src=in.begin_list((std::list<int>*)0);
      if (init_from_set(list_reader<int, cursor&>(src), std::false_type())) src.skip_rest();
      src.finish();
   }

   template <typename Input>
   std::enable_if_t<multigraph, typename mproject2nd<Input, void>::type>
   read(Input& in)
   {
      typedef typename Input::template list_cursor< SparseVector<int> >::type cursor;
      cursor src=in.begin_list((SparseVector<int>*)0);
      if (src.sparse_representation())
         init_multi_from_sparse(src.set_option(SparseRepresentation<std::true_type>()));
      else
         init_multi_from_dense(src.set_option(SparseRepresentation<std::false_type>()));
      src.finish();
   }

   int dim() const { return this->max_size(); }

   incident_edge_list& operator= (const incident_edge_list& l)
   {
      copy(entire(l));
      return *this;
   }

   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& in, incident_edge_list& me)
   {
      me.read(in.top());
      return in.top();
   }

   using parallel_edge_iterator = std::conditional_t<multigraph,
                                     input_truncator<typename base_t::iterator, truncate_after_index>,
                                     single_position_iterator<typename base_t::iterator> >;

   using parallel_edge_const_iterator = std::conditional_t<multigraph,
                                     input_truncator<typename base_t::const_iterator, truncate_after_index>,
                                     single_position_iterator<typename base_t::const_iterator> >;

private:
   parallel_edge_const_iterator all_edges_to(int n2, std::false_type) const
   {
      return this->find(n2);
   }

   parallel_edge_iterator all_edges_to(int n2, std::false_type)
   {
      return this->find(n2);
   }

   parallel_edge_const_iterator all_edges_to(int n2, std::true_type) const
   {
      return parallel_edge_const_iterator(this->find_nearest(n2, first_of_equal()), truncate_after_index(n2));
   }

   parallel_edge_iterator all_edges_to(int n2, std::true_type)
   {
      return parallel_edge_iterator(this->find_nearest(n2, first_of_equal()), truncate_after_index(n2));
   }

   void delete_all_edges_to(int n2, std::false_type)
   {
      this->erase(n2);
   }

   void delete_all_edges_to(int n2, std::true_type)
   {
      for (parallel_edge_iterator e=all_edges_to(n2, std::true_type()); !e.at_end(); )
         this->erase(e++);
   }
public:
   parallel_edge_const_iterator all_edges_to(int n2) const
   {
      return all_edges_to(n2, bool_constant<multigraph>());
   }

   parallel_edge_iterator all_edges_to(int n2)
   {
      return all_edges_to(n2, bool_constant<multigraph>());
   }

   void delete_all_edges_to(int n2)
   {
      delete_all_edges_to(n2, bool_constant<multigraph>());
   }
};

template <typename Tree>
class lower_incident_edge_list
   : public modified_container_impl< lower_incident_edge_list<Tree>,
                                     mlist< HiddenTag< incident_edge_list<Tree> >,
                                            IteratorConstructorTag< input_truncator_constructor >,
                                            OperationTag< BuildUnaryIt<uniq_edge_predicate> > > > {
public:
   int dim() const { return this->hidden().dim(); }
};


template <typename Tree>
class multi_adjacency_line
   : public modified_container_impl< multi_adjacency_line<Tree>,
                                     mlist< HiddenTag< incident_edge_list<Tree> >,
                                            IteratorConstructorTag< range_folder_constructor >,
                                            OperationTag< equal_index_folder > > >,
   public GenericVector<multi_adjacency_line<Tree>, int> {
protected:
   ~multi_adjacency_line() = delete;

public:
   int dim() const { return this->hidden().dim(); }
};


template <typename EntryRef>
struct valid_node_selector {
   typedef EntryRef argument_type;
   typedef typename deref<EntryRef>::type entry_type;
   typedef bool result_type;

   bool operator() (argument_type t) const
   {
      return t.get_line_index()>=0;
   }

   using out_tree_type = typename entry_type::out_tree_type;
   using in_tree_type = typename entry_type::in_tree_type;
   using out_edge_list = incident_edge_list<out_tree_type>;
   using in_edge_list = incident_edge_list<in_tree_type>;
   using out_adjacent_node_list = std::conditional_t<out_edge_list::multigraph, multi_adjacency_line<out_tree_type>, incidence_line<out_tree_type>>;
   using in_adjacent_node_list = std::conditional_t<out_edge_list::multigraph, multi_adjacency_line<in_tree_type>, incidence_line<in_tree_type>>;
   using adjacent_node_list = out_adjacent_node_list;
   using out_edge_list_ref = typename inherit_ref<out_edge_list, EntryRef>::type;
   using in_edge_list_ref = typename inherit_ref<in_edge_list, EntryRef>::type;
   using out_adjacent_node_list_ref = typename inherit_ref<out_adjacent_node_list, EntryRef>::type;
   using in_adjacent_node_list_ref = typename inherit_ref<in_adjacent_node_list, EntryRef>::type;
   using adjacent_node_list_ref = out_adjacent_node_list_ref;

   out_edge_list_ref out_edges(EntryRef t) const
   {
      return reinterpret_cast<out_edge_list_ref>(t.out());
   }
   in_edge_list_ref in_edges(EntryRef t) const
   {
      return reinterpret_cast<in_edge_list_ref>(t.in());
   }

   typename out_tree_type::const_iterator out_edge_impl(EntryRef t, int n2, std::true_type) const
   {
      typename out_tree_type::const_iterator e=t.out().find(n2);
      if (e.at_end()) throw no_match("non-existing edge");
      return e;
   }
   typename in_tree_type::const_iterator in_edge_impl(EntryRef t, int n2, std::true_type) const
   {
      typename in_tree_type::const_iterator e=t.in().find(n2);
      if (e.at_end()) throw no_match("non-existing edge");
      return e;
   }

   typename out_tree_type::iterator out_edge_impl(EntryRef t, int n2, std::false_type) const
   {
      return t.out().insert(n2);
   }
   typename in_tree_type::iterator in_edge_impl(EntryRef t, int n2, std::false_type) const
   {
      return t.in().insert(n2);
   }
   int out_edge(EntryRef t, int n2) const
   {
      return out_edge_impl(t, n2, bool_constant<attrib<EntryRef>::is_const>())->data;
   }
   int in_edge(EntryRef t, int n2) const
   {
      return in_edge_impl(t, n2, bool_constant<attrib<EntryRef>::is_const>())->data;
   }

   int out_degree(EntryRef t) const
   {
      return t.out().size();
   }
   int in_degree(EntryRef t) const
   {
      return t.in().size();
   }
   int degree(EntryRef t) const
   {
      return t.degree();
   }

   out_adjacent_node_list_ref out_adjacent_nodes(EntryRef t) const
   {
      return reinterpret_cast<out_adjacent_node_list_ref>(t.out());
   }
   in_adjacent_node_list_ref in_adjacent_nodes(EntryRef t) const
   {
      return reinterpret_cast<in_adjacent_node_list_ref>(t.in());
   }
   adjacent_node_list_ref adjacent_nodes(EntryRef t) const
   {
      return reinterpret_cast<adjacent_node_list_ref>(t.out());
   }
};

template <typename TOut_edges, template <typename> class MasqueradeLine, typename NodeIterator=void>
class line_factory;

template <typename Iterator, typename Accessor>
struct valid_node_iterator : public unary_predicate_selector<Iterator, Accessor> {
   typedef unary_predicate_selector<Iterator, Accessor> base_t;
   template <typename, template <typename> class, typename> friend class line_factory;
public:
   typedef valid_node_iterator<typename iterator_traits<Iterator>::iterator, Accessor> iterator;
   typedef valid_node_iterator<typename iterator_traits<Iterator>::const_iterator, Accessor> const_iterator;
   typedef random_access_iterator_tag iterator_category;

   valid_node_iterator() {}

   template <typename Accessor2>
   valid_node_iterator(const valid_node_iterator<typename iterator_traits<Iterator>::iterator, Accessor2>& it)
      : base_t(it) {}

   valid_node_iterator(const Iterator& cur_arg, const Accessor& acc_arg=Accessor())
      : base_t(cur_arg,acc_arg) {}

   valid_node_iterator& operator++ () { base_t::operator++(); return *this; }
   const valid_node_iterator operator++ (int) { valid_node_iterator copy(*this); base_t::operator++(); return copy; }

   valid_node_iterator& operator-- () { base_t::operator--(); return *this; }
   const valid_node_iterator operator-- (int) { valid_node_iterator copy(*this); base_t::operator--(); return copy; }

   // random access is based on the absolute node index, not on the number of valid nodes in between!
   valid_node_iterator& operator+= (int i)
   {
      static_cast<Iterator&>(*this)+=i;
      return *this;
   }
   valid_node_iterator operator+ (int i) const { valid_node_iterator copy(*this); return copy+=i; }
   friend
   valid_node_iterator operator+ (int i, const valid_node_iterator& me) { return me+i; }

   valid_node_iterator& operator-= (int i)
   {
      static_cast<Iterator&>(*this)-=i;
      return *this;
   }
   valid_node_iterator operator- (int i) const { valid_node_iterator copy(*this); return copy-=i; }
   ptrdiff_t operator- (const valid_node_iterator& it) const { return static_cast<const Iterator&>(*this)-it; }

   int index() const { return (**this).get_line_index(); }

   typedef typename base_t::helper::operation::out_edge_list_ref out_edge_list_ref;
   typedef typename base_t::helper::operation::in_edge_list_ref in_edge_list_ref;
   typedef typename base_t::helper::operation::out_adjacent_node_list_ref out_adjacent_node_list_ref;
   typedef typename base_t::helper::operation::in_adjacent_node_list_ref in_adjacent_node_list_ref;
   typedef typename base_t::helper::operation::adjacent_node_list_ref adjacent_node_list_ref;

   out_edge_list_ref out_edges() const
   {
      return this->pred.out_edges(**this);
   }
   in_edge_list_ref in_edges() const
   {
      return this->pred.in_edges(**this);
   }

   int out_edge(int n2) const
   {
      return this->pred.out_edge(**this, n2);
   }
   int in_edge(int n2) const
   {
      return this->pred.in_edge(**this, n2);
   }
   int edge(int n2) const
   {
      return this->pred.out_edge(**this, n2);
   }

   int out_degree() const
   {
      return this->pred.out_degree(**this);
   }
   int in_degree() const
   {
      return this->pred.in_degree(**this);
   }
   int degree() const
   {
      return this->pred.degree(**this);
   }

   out_adjacent_node_list_ref out_adjacent_nodes() const
   {
      return this->pred.out_adjacent_nodes(**this);
   }
   in_adjacent_node_list_ref in_adjacent_nodes() const
   {
      return this->pred.in_adjacent_nodes(**this);
   }
   adjacent_node_list_ref adjacent_nodes() const
   {
      return this->pred.adjacent_nodes(**this);
   }
};

struct valid_node_access_constructor : unary_predicate_selector_constructor {
   template <typename Iterator, typename Accessor, typename ExpectedFeatures>
   struct defs : unary_predicate_selector_constructor::defs<Iterator,Accessor,ExpectedFeatures> {
      typedef valid_node_iterator<Iterator,Accessor> iterator;
   };
};

template <typename TDir>
class valid_node_container
   : public modified_container_impl< valid_node_container<TDir>,
                                     mlist< ContainerTag< typename Table<TDir>::ruler >,
                                            OperationTag< BuildUnary<valid_node_selector> >,
                                            IteratorConstructorTag< valid_node_access_constructor >,
                                            HiddenTag< Table<TDir> > > > {
   typedef modified_container_impl<valid_node_container> base_t;
protected:
   ~valid_node_container();
public:
   typedef random_access_iterator_tag container_category;

   typename base_t::container& get_container() { return this->hidden().get_ruler(); }
   const typename base_t::container& get_container() const { return this->hidden().get_ruler(); }

   typename base_t::reference operator[] (int n) { return get_container()[n]; }
   typename base_t::const_reference operator[] (int n) const { return get_container()[n]; }

   int dim() const { return get_container().size(); }
};

template <typename TDir>
class node_container
   : public modified_container_impl< node_container<TDir>,
                                     mlist< HiddenTag< valid_node_container<TDir> >,
                                            OperationTag< BuildUnaryIt<operations::index2element> > > >,
     public GenericSet<node_container<TDir>, int, operations::cmp> {
protected:
   ~node_container();
public:
   int dim() const { return this->get_container().dim(); }
};

} // end namespace graph

template <typename Iterator, typename Accessor, typename Feature>
struct check_iterator_feature<graph::valid_node_iterator<Iterator, Accessor>, Feature>
   : check_iterator_feature<unary_predicate_selector<Iterator, Accessor>, Feature> {};

template <typename Iterator, typename Accessor>
struct check_iterator_feature<graph::valid_node_iterator<Iterator, Accessor>, indexed>
   : std::true_type {};

template <typename TDir>
struct check_container_feature<graph::valid_node_container<TDir>, sparse_compatible>
   : std::true_type {};

template <typename TDir>
struct check_container_feature<graph::node_container<TDir>, sparse_compatible>
   : std::true_type {};

namespace graph {

template <typename TOut_edges, template <typename> class MasqueradeLine, typename EntryRef>
class line_factory {
public:
   typedef EntryRef argument_type;
   using tree_type = std::conditional_t<TOut_edges::value, typename deref<EntryRef>::type::out_tree_type,
                                                           typename deref<EntryRef>::type::in_tree_type>;
   typedef typename inherit_ref<MasqueradeLine<tree_type>, EntryRef>::type result_type;

   result_type operator() (argument_type e) const
   {
      return impl(e, TOut_edges());
   }
private:
   result_type impl(argument_type e, std::true_type) const
   {
      return reinterpret_cast<result_type>(e.out());
   }
   result_type impl(argument_type e, std::false_type) const
   {
      return reinterpret_cast<result_type>(e.in());
   }
};

template <typename TOut_edges, template <typename> class MasqueradeLine>
class line_factory<TOut_edges, MasqueradeLine, void> : operations::incomplete {};

} // end namespace graph

template <typename TOut_edges, template <typename> class MasqueradeLine, typename Iterator, typename EntryRef>
struct unary_op_builder<graph::line_factory<TOut_edges, MasqueradeLine, void>, Iterator, EntryRef>
   : empty_op_builder< graph::line_factory<TOut_edges, MasqueradeLine, EntryRef> > {};

namespace graph {

template <typename TDir, typename TOut_edges, template <typename> class MasqueradeLine>
class line_container
   : public modified_container_impl< line_container<TDir, TOut_edges, MasqueradeLine>,
                                     mlist< HiddenTag< valid_node_container<TDir> >,
                                            OperationTag< line_factory<TOut_edges, MasqueradeLine> > > > {
protected:
   ~line_container();
public:
   Table<TDir>&       get_table()       { return this->hidden().hidden(); }
   const Table<TDir>& get_table() const { return this->hidden().hidden(); }
   void resize(int n) { get_table().clear(n); }
};

template <typename TDir, bool undirected=TDir::value>
struct edge_container_helper {
   typedef line_container<TDir, std::true_type, incident_edge_list> type;
};
template <typename TDir>
struct edge_container_helper<TDir, true> {
   typedef line_container<TDir, std::true_type, lower_incident_edge_list> type;
};

template <typename TDir>
class edge_container
   : public cascade_impl< edge_container<TDir>,
                          mlist< HiddenTag< typename edge_container_helper<TDir>::type >,
                                 CascadeDepth< int_constant<2> > > > {
protected:
   ~edge_container();
public:
   int size() const { return this->hidden().get_table().edges(); }
   int max_size() const { return size(); }
};

template <typename TDir>
template <bool for_copy>
void edge_agent<TDir>::init(Table<TDir>* t, bool_constant<for_copy>)
{
   table=t;
   n_alloc=min_buckets(n_edges+bucket_mask>>bucket_shift);
   if (!for_copy) {
      // if the table was cloned, edge ids were copied too
      int id=0;
      for (typename edge_container<TDir>::iterator e=reinterpret_cast<edge_container<TDir>*>(t)->begin(); !e.at_end(); ++e, ++id)
         e.edge_id()=id;
   }
}

template <typename TDir>
template <typename NumberConsumer>
void edge_agent<TDir>::renumber(const NumberConsumer& nc)
{
   int id=0;
   for (typename edge_container<TDir>::iterator e=reinterpret_cast<edge_container<TDir>*>(table)->begin(); !e.at_end(); ++e, ++id) {
      nc(e.edge_id(), id);
      e.edge_id()=id;
   }
   assert(id == n_edges);
}

} // end namespace graph

template <typename TDir, typename TOut_edges, template <typename> class MasqueradeLine>
struct check_container_feature<graph::line_container<TDir, TOut_edges, MasqueradeLine>, sparse_compatible> : std::true_type {};

template <typename Tree>
struct check_container_feature<graph::incident_edge_list<Tree>, sparse_compatible> : std::true_type {};

template <typename Tree>
struct check_container_feature<graph::lower_incident_edge_list<Tree>, sparse_compatible> : std::true_type {};

template <typename Tree>
struct check_container_feature<graph::multi_adjacency_line<Tree>, pure_sparse> : std::true_type {};

template <typename Tree>
struct spec_object_traits< graph::multi_adjacency_line<Tree> >
   : spec_object_traits<is_container> {
   typedef Tree masquerade_for;
   static const int is_resizeable=0;
   static const bool is_always_const=true;
};

namespace graph {

/** @class Graph
    @brief Directed or undirected finite graphs.

    @a NodeAttr and @a EdgeAttr specify the type of additional data associated
    with nodes and edges (sometimes also called node and edge attributes.) The
    default setting @a nothing denotes the absence of any attributes, it
    doesn't waste extra memory.

    The nodes of the graph are referred to via integer indices, starting with
    0; they are stored in a contiguous array.  This allows constant-time
    random node access, while inserting or deleting of nodes incurs storage
    reallocation.  However, due to some kind of forecasting strategy of memory
    allocation (similar to that deployed in std::vector), the amortized
    cost of node insertion is proportional to <i>#nodes log(#nodes)</i>.

    The edges are organized in incidence lists, which are implemented as a 2-d
    mesh of AVL trees.  Hence, a random access to an edge (with given source
    and target nodes) takes a logarithmical time of the source node degree.
    Multiple edges (i.e., with the same source and target nodes) are not
    allowed; they could be modeled, however, by choosing some container class
    as the edge attribute.

    The kind of the graph decides about how the incidence edge lists are
    organized.  In the @em directed case each edge naturally appears in the
    outgoing list of its source node and in the ingoing list of its target
    node.  In the @em undirected case the notions of outgoing and ingoing
    edges are the same; each edge manages to appear in the outgoing lists of
    both adjacent nodes, although it is stored only once.  The @i skew case is
    a special variant of the indirect case, intended for numerical edge
    attributes only. Depending on the reading direction, the edge attribute
    changes its sign: @c{ edge(n1,n2) == -edge(n2,n1) }.

    The whole data structure is attached to the Graph object via a smart
    pointer with @ref refcounting "reference counting".

*/
template <typename TDir>
class Graph
   : public GenericGraph<Graph<TDir>, TDir> {
protected:
   typedef Table<TDir> table_type;
public:
   /// Create an empty Graph with 0 nodes.
   Graph() {}

   /// Create a Graph with @a n isolated nodes (without edges).
   explicit Graph(int n)
      : data(n) {}

   Graph(const GenericGraph<Graph>& G2)
      : data(G2.top().data) {}

   template <typename Graph2>
   Graph(const GenericGraph<Graph2, TDir>& G2)
      : data(G2.top().dim())
   {
      copy_impl(pm::nodes(G2).begin(), std::false_type(), std::false_type(), G2.top().has_gaps());
   }

   template <typename Graph2, typename TDir2>
   explicit Graph(const GenericGraph<Graph2, TDir2>& G2)
      : data(G2.top().dim())
   {
      const bool need_merge= Graph2::is_directed && !Graph::is_directed,
           need_contraction= Graph2::is_multigraph && !Graph::is_multigraph;
      copy_impl(pm::nodes(G2).begin(), bool_constant<need_merge>(), bool_constant<need_contraction>(), G2.top().has_gaps());
   }

   template <typename TMatrix>
   explicit Graph(const GenericIncidenceMatrix<TMatrix>& m,
                  std::enable_if_t<!TDir::multigraph, mlist<TMatrix>**> =nullptr)
      : data(m.rows())
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (!TMatrix::is_symmetric && m.rows() != m.cols())
            throw std::runtime_error("Graph - non-quadratic source adjacency matrix");
      }
      const bool need_merge= !TMatrix::is_symmetric && !Graph::is_directed;
      copy_impl(rows(m).begin(), bool_constant<need_merge>());
   }

   // construct graph with gaps
   template <typename TSet>
   explicit Graph(const GenericSet<TSet, int>& s)
      : data(s.top()) {}

   template <typename TSet>
   Graph(const GenericSet<TSet, int>& s, int dim)
      : data(s.top(), dim) {}

   /// assignment from Graph of the same type
   Graph& operator= (const Graph& G2)
   {
      data=G2.data;
      return *this;
   }

   /// assignment from GenericGraph
   Graph& operator= (const GenericGraph<Graph>& G2)
   {
      *this=G2.top();
      return *this;
   }

   /// assignment from GenericGraph with other flavor of directedness
   template <typename Graph2, typename TDir2>
   Graph& operator= (const GenericGraph<Graph2, TDir2>& G2)
   {
      clear(G2.top().dim());
      const bool need_merge= Graph2::is_directed && !Graph::is_directed,
           need_contraction= Graph2::is_multigraph && !Graph::is_multigraph;
      copy_impl(pm::nodes(G2).begin(), bool_constant<need_merge>(), bool_constant<need_contraction>(), G2.top().has_gaps());
      return *this;
   }

   /// number of nodes
   int nodes() const { return data->nodes(); }

   /// resize the Graph to given number of nodes
   void resize(int n) { data->resize(n); }

   /// clear all edges and resize (to zero nodes by default)
   void clear(int n=0) { data.apply(typename table_type::shared_clear(n)); }

   /// true of nodes are not (known to be) consecutively ordered
   bool has_gaps() const { return data->free_node_id != std::numeric_limits<int>::min(); }

   /// renumber the nodes
   friend Graph renumber_nodes(const Graph& me)
   {
      if (!me.has_gaps()) return me;
      Graph G(me.nodes());
      std::vector<int> renumber(me.dim());
      int i=0;
      for (auto n=entire(pm::nodes(me)); !n.at_end(); ++n, ++i)
         renumber[n.index()]=i;
      for (auto e=entire(pm::edges(me)); !e.at_end(); ++e)
         G.edge(renumber[e.from_node()], renumber[e.to_node()]);
      return G;
   }

   /// "output dimension"; relevant for proper output in the polymake shell
   int dim() const { return data->dim(); }

   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& in, Graph& me)
   {
      me.read(in.top().begin_list(&rows(pm::adjacency_matrix(me))));
      return in.top();
   }

   // for IndexDispenser
   class ResizeTraits {
      Graph *G;
   protected:
      void resize(int n) { G->resize(n); }
   public:
      ResizeTraits(Graph& G_arg) : G(&G_arg) {}
   };

   /// swap function
   void swap(Graph& G) { data.swap(G.data); }

   /// relocate the data
   friend void relocate(Graph* from, Graph* to)
   {
      relocate(&from->data, &to->data);
   }

   template <typename NumberConsumer>
   void squeeze(const NumberConsumer& nc)
   {
      data->squeeze_nodes(nc);
   }
      
   /// force renumbering of the nodes in consecutive order
   void squeeze()
   {
      data->squeeze_nodes(operations::binary_noop());
   }

   template <typename NumberConsumer>
   void squeeze_isolated(const NumberConsumer& nc)
   {
      data->squeeze_nodes(nc, typename table_type::template squeeze_node_chooser<true>());
   }

   void squeeze_isolated()
   {
      data->squeeze_nodes(operations::binary_noop(), typename table_type::template squeeze_node_chooser<true>());
   }

   template <typename NumberConsumer>
   void squeeze_edges(const NumberConsumer& nc)
   {
      data->squeeze_edges(nc);
   }

   void squeeze_edges()
   {
      data->squeeze_edges(operations::binary_noop());
   }

   /// delete a node
   void delete_node(int n)
   {
      data->delete_node(n);
   }

   /// permute the nodes
   template <typename TPerm>
   std::enable_if_t<isomorphic_to_container_of<TPerm, int>::value>
   permute_nodes(const TPerm& perm)
   {
      data->permute_nodes(perm, std::false_type());
   }

   /// inverse permutation of nodes
   template <typename TInvPerm>
   std::enable_if_t<isomorphic_to_container_of<TInvPerm, int>::value>
   permute_inv_nodes(const TInvPerm& inv_perm)
   {
      data->permute_nodes(inv_perm, std::true_type());
   }

   /// permuted copy
   template <typename TPerm, typename TInvPerm>
   std::enable_if_t<isomorphic_to_container_of<TPerm, int>::value &&
                    isomorphic_to_container_of<TInvPerm, int>::value,
                    Graph>
   copy_permuted(const TPerm& perm, const TInvPerm& inv_perm) const
   {
      Graph result(dim());
      result.data->copy_permuted(*data, perm, inv_perm);
      return result;
   }

   /// node type
   typedef graph::node_container<TDir> node_container;
   /// node reference type
   typedef node_container& node_container_ref;
   /// constant node reference type
   typedef const node_container& const_node_container_ref;

   template <template <typename> class MasqueradeLine>
   struct edge_access {
      typedef line_container<TDir, std::true_type, MasqueradeLine> out;
      typedef line_container<TDir, std::false_type, MasqueradeLine> in;
   };

   typedef typename edge_access<incident_edge_list>::out out_edge_list_container;
   typedef out_edge_list_container& out_edge_list_container_ref;
   typedef const out_edge_list_container& const_out_edge_list_container_ref;

   typedef typename out_edge_list_container::value_type out_edge_list;
   typedef out_edge_list& out_edge_list_ref;
   typedef const out_edge_list& const_out_edge_list_ref;

   typedef typename out_edge_list::parallel_edge_iterator       parallel_edge_iterator;
   typedef typename out_edge_list::parallel_edge_const_iterator parallel_edge_const_iterator;

   typedef typename edge_access<incident_edge_list>::in in_edge_list_container;
   typedef in_edge_list_container& in_edge_list_container_ref;
   typedef const in_edge_list_container& const_in_edge_list_container_ref;

   typedef typename in_edge_list_container::value_type in_edge_list;
   typedef in_edge_list& in_edge_list_ref;
   typedef const in_edge_list& const_in_edge_list_ref;

   using adjacency_rows_container = std::conditional_t<TDir::multigraph, typename edge_access<multi_adjacency_line>::out, typename edge_access<incidence_line>::out>;
   typedef adjacency_rows_container& adjacency_rows_container_ref;
   typedef const adjacency_rows_container& const_adjacency_rows_container_ref;

   using adjacency_cols_container = std::conditional_t<TDir::multigraph, typename edge_access<multi_adjacency_line>::in, typename edge_access<incidence_line>::in>;
   typedef adjacency_cols_container& adjacency_cols_container_ref;
   typedef const adjacency_cols_container& const_adjacency_cols_container_ref;

   typedef typename adjacency_rows_container::value_type out_adjacent_node_list;
   typedef typename adjacency_cols_container::value_type in_adjacent_node_list;
   using adjacent_node_list = std::conditional_t<Graph::is_directed, nothing, out_adjacent_node_list>;
   typedef typename assign_const<out_adjacent_node_list, TDir::multigraph>::type& out_adjacent_node_list_ref;
   typedef const out_adjacent_node_list& const_out_adjacent_node_list_ref;
   typedef typename assign_const<in_adjacent_node_list, TDir::multigraph>::type& in_adjacent_node_list_ref;
   typedef const in_adjacent_node_list& const_in_adjacent_node_list_ref;
   using adjacent_node_list_ref = std::conditional_t<Graph::is_directed, nothing, out_adjacent_node_list_ref>;
   using const_adjacent_node_list_ref = std::conditional_t<Graph::is_directed, nothing, const_out_adjacent_node_list_ref>;

   template <typename MasqueradeRef>
   MasqueradeRef pretend()
   {
      return reinterpret_cast<MasqueradeRef>(*data);
   }

   template <typename MasqueradeRef>
   MasqueradeRef pretend() const
   {
      return reinterpret_cast<MasqueradeRef>(*data);
   }

   /// number of edges
   int edges() const { return data->edges(); }

   /// add a node; may reuse a currently unused node
   int add_node() { return data->add_node(); }

   /// true if node is unused
   bool invalid_node(int n) const { return data->invalid_node(n); }
   /// true if node number is higher than currently reserved maximum number of nodes
   bool node_out_of_range(int n) const { return data->node_out_of_range(n); }

   /// out-degree of a node
   int out_degree(int n) const
   {
      return (*data)[n].out().size();
   }

   /// in-degree of a node
   int in_degree(int n) const
   {
      return (*data)[n].in().size();
   }

   /// total degree of a node
   int degree(int n) const
   {
      return (*data)[n].degree();
   }

   /// true if node exists
   bool node_exists(int n) const
   {
      return data->node_exists(n);
   }
private:
   typedef typename table_type::entry node_entry_type;
   typedef valid_node_selector<node_entry_type&> node_acc;
   typedef valid_node_selector<const node_entry_type&> const_node_acc;
public:
   /// Return the number of the edge between two given nodes.
   /// The edge is created if it did not exist before.
   /// In a multigraph, one (arbitrary) edge from a parallel bundle will be returned.
   int edge(int n1, int n2)
   {
      node_acc n;
      return n.out_edge((*data)[n1], n2);
   }

   /// Return the number of the edge between two given nodes.
   /// If the edge does not exist, an exception is raised.
   int edge(int n1, int n2) const
   {
      const_node_acc n;
      return n.out_edge((*data)[n1], n2);
   }

private:
   int add_edge_impl(int n1, int n2, std::false_type) { return edge(n1, n2); }
   int add_edge_impl(int n1, int n2, std::true_type) { return (*data)[n1].out().insert_new(n2)->data; }
public:
   /// Create a new edge between two given nodes and return its number.
   /// In a multigraph, a new edge is always created; the exact position of the new edge among its parallel twins can't be predicted.
   /// In a normal graph, a new edge is only created if the nodes were not adjacent before.
   int add_edge(int n1, int n2)
   {
      return add_edge_impl(n1, n2, bool_constant<Graph::is_multigraph>());
   }

   /// Check whether there is an edge between the two given nodes.
   bool edge_exists(int n1, int n2) const
   {
      return (*data)[n1].out().exists(n2);
   }

   /// Return the iterator over all edges connecting two given nodes.
   parallel_edge_iterator all_edges(int n1, int n2)
   {
      return out_edges(n1).all_edges_to(n2);
   }

   /// Return the iterator over all edges connecting two given nodes.
   parallel_edge_const_iterator all_edges(int n1, int n2) const
   {
      return out_edges(n1).all_edges_to(n2);
   }

   /// Delete the edge (if it exists) connecting the two given nodes.
   void delete_edge(int n1, int n2)
   {
      (*data)[n1].out().erase(n2);
   }

   /// Delete an edge pointed by the given iterator over a sequence of (parallel) edges.
   void delete_edge(const parallel_edge_iterator& where)
   {
      (*data)[where.get_line_index()].out().erase(where);
   }

   /// Delete all (parallel) edges connecting the two given nodes.
   void delete_all_edges(int n1, int n2)
   {
      out_edges(n1).delete_all_edges_to(n2);
   }

   /// Contract the edge between the two given nodes.
   /// The second node is deleted afterwards.
   /// If the specified edge belongs to one or more triangles in a multigraph, the lateral edges become parallel after contraction.
   /// In a normal graph, the lateral edges incident to n2 are deleted.
   void contract_edge(int n1, int n2)
   {
      relink_edges((*data)[n2].out(), (*data)[n1].out(), n2, n1);
      if (Graph::is_directed)
         relink_edges((*data)[n2].in(), (*data)[n1].in(), n2, n1);
      data->delete_node(n2);
   }

private:
   template <typename Tree>
   void relink_edges(Tree& tree_from, Tree& tree_to, int node_from, int node_to)
   {
      for (auto it=tree_from.begin(); !it.at_end(); ) {
         const auto c=it.operator->();  ++it;
         if (c->key == node_from + node_to) {
            // this is an edge to be contracted
            tree_from.destroy_node(c);

         } else if (c->key == node_from * 2) {
            // a loop
            c->key = node_to * 2;
            if (tree_to.insert_node(c)) {
               if (Graph::is_directed) {
                  (*data)[node_from].in().remove_node(c);
                  (*data)[node_to].in().insert_node(c);
               }
            } else {
               c->key = node_from * 2;
               tree_from.destroy_node(c);
            }

         } else {
            tree_from.prepare_move_between_trees(c, node_from, node_to);
            if (tree_to.insert_node(c)) {
               (*data)[c->key - node_to].cross_tree(&tree_from).update_node(c);
            } else {
               tree_to.prepare_move_between_trees(c, node_to, node_from);
               tree_from.destroy_node(c);
            }
         }
      }
      // forget the nodes
      tree_from.init();
   }

public:
   template <template <typename> class MasqueradeLine>
   typename edge_access<MasqueradeLine>::out::reference
   out_edges(int n)
   {
      return reinterpret_cast<typename edge_access<MasqueradeLine>::out::reference>((*data)[n].out());
   }
   template <template <typename> class MasqueradeLine>
   typename edge_access<MasqueradeLine>::out::const_reference
   out_edges(int n) const
   {
      return reinterpret_cast<typename edge_access<MasqueradeLine>::out::const_reference>((*data)[n].out());
   }

   /// reference to list of outgoing edges
   out_edge_list_ref out_edges(int n)
   {
      return this->template out_edges<incident_edge_list>(n);
   }
   /// constant reference to list of outgoing edges
   const_out_edge_list_ref out_edges(int n) const
   {
      return this->template out_edges<incident_edge_list>(n);
   }

   template <template <typename> class MasqueradeLine>
   typename edge_access<MasqueradeLine>::in::reference
   in_edges(int n)
   {
      return reinterpret_cast<typename edge_access<MasqueradeLine>::in::reference>((*data)[n].in());
   }
   template <template <typename> class MasqueradeLine>
   const typename edge_access<MasqueradeLine>::in::const_reference
   in_edges(int n) const
   {
      return reinterpret_cast<typename edge_access<MasqueradeLine>::in::const_reference>((*data)[n].in());
   }

   /// reference to list of incoming edges
   in_edge_list_ref in_edges(int n)
   {
      return this->template in_edges<incident_edge_list>(n);
   }
   /// constant reference to list of incoming edges
   const_in_edge_list_ref in_edges(int n) const
   {
      return this->template in_edges<incident_edge_list>(n);
   }

   /// reference to list of nodes which are adjacent via out-arcs
   out_adjacent_node_list_ref out_adjacent_nodes(int n)
   {
      return out_adjacent_nodes_impl(n, bool_constant<TDir::multigraph>());
   }
   /// constant reference to list of nodes which are adjacent via out-arcs
   const_out_adjacent_node_list_ref out_adjacent_nodes(int n) const
   {
      return out_adjacent_nodes_impl(n, bool_constant<TDir::multigraph>());
   }
   /// reference to list of nodes which are adjacent via in-arcs
   in_adjacent_node_list_ref in_adjacent_nodes(int n)
   {
      return in_adjacent_nodes_impl(n, bool_constant<TDir::multigraph>());
   }
   /// constant reference to list of nodes which are adjacent via in-arcs
   const_in_adjacent_node_list_ref in_adjacent_nodes(int n) const
   {
      return in_adjacent_nodes_impl(n, bool_constant<TDir::multigraph>());
   }
   /// reference to list of all adjacent nodes
   adjacent_node_list_ref adjacent_nodes(int n)
   {
      static_assert(!Graph::is_directed, "adjacent_nodes undefined for a directed graph");
      return out_adjacent_nodes_impl(n, bool_constant<TDir::multigraph>());
   }
   /// constant reference to list of all adjacent nodes
   const_adjacent_node_list_ref adjacent_nodes(int n) const
   {
      static_assert(!Graph::is_directed, "adjacent_nodes undefined for a directed graph");
      return out_adjacent_nodes_impl(n, bool_constant<TDir::multigraph>());
   }

private:
   out_adjacent_node_list_ref out_adjacent_nodes_impl(int n, std::false_type)
   {
      return this->template out_edges<incidence_line>(n);
   }
   const_out_adjacent_node_list_ref out_adjacent_nodes_impl(int n, std::false_type) const
   {
      return this->template out_edges<incidence_line>(n);
   }
   in_adjacent_node_list_ref in_adjacent_nodes_impl(int n, std::false_type)
   {
      return this->template in_edges<incidence_line>(n);
   }
   const_in_adjacent_node_list_ref in_adjacent_nodes_impl(int n, std::false_type) const
   {
      return this->template in_edges<incidence_line>(n);
   }

   const_out_adjacent_node_list_ref out_adjacent_nodes_impl(int n, std::true_type) const
   {
      return this->template out_edges<multi_adjacency_line>(n);
   }
   const_in_adjacent_node_list_ref in_adjacent_nodes_impl(int n, std::true_type) const
   {
      return this->template in_edges<multi_adjacency_line>(n);
   }

public:
   template <typename E, typename... TParams>
   struct NodeMapData : public NodeMapBase {
      typedef typename mlist_wrap<TParams...>::type params;
      typedef typename mtagged_list_extract<params, DefaultValueTag, operations::clear<E>>::type default_value_supplier;

      E* data;
      size_t n_alloc;
      default_value_supplier dflt;
      std::allocator<E> _allocator;

      table_type& table() { return *reinterpret_cast<table_type*>(_table); }
      const table_type& ctable() const { return *reinterpret_cast<const table_type*>(_table); }

      void alloc(size_t n)
      {
         n_alloc=n;
         data=_allocator.allocate(n);
      }

      void init()
      {
         for (auto it=entire(get_index_container()); !it.at_end(); ++it)
            construct_at(data+*it, dflt());
      }

      template <typename Init>
      void init(const Init& val,
                std::enable_if_t<can_initialize<Init, E>::value, void**> =nullptr)
      {
         for (auto it=entire(get_index_container()); !it.at_end(); ++it)
            construct_at(data+*it, val);
      }

      template <typename Iterator>
      void init(Iterator&& src_it,
                std::enable_if_t<assess_iterator_value<Iterator, can_initialize, E>::value, void**> =nullptr)
      {
         decltype(auto) src=ensure_private_mutable(std::forward<Iterator>(src_it));
         for (auto it=entire(get_index_container()); !it.at_end(); ++it, ++src)
            construct_at(data+*it, *src);
      }

      void copy(const NodeMapData& m)
      {
         dflt=m.dflt;
         typename node_container::const_iterator src=m.get_index_container().begin();
         for (auto it=entire(get_index_container()); !it.at_end(); ++it, ++src)
            construct_at(data+*it, m.data[*src]);
      }

      void reset(int n=0)
      {
         if (!std::is_trivially_destructible<E>::value)
            for (auto it=entire(get_index_container()); !it.at_end(); ++it)
               destroy_at(data+*it);
         if (n) {
            if (size_t(n) != n_alloc) {
               _allocator.deallocate(data, n_alloc);
               alloc(n);
            }
         } else {
            _allocator.deallocate(data, n_alloc);
            data=nullptr;  n_alloc=0;
         }
      }

      void clear(int n=0)
      {
         for (auto it=entire(get_index_container()); !it.at_end(); ++it)
            dflt.assign(data[*it]);
      }

      void resize(size_t new_n_alloc, int n, int nnew)
      {
         if (n_alloc < new_n_alloc) {
            E *new_data=_allocator.allocate(new_n_alloc), *src=data, *dst=new_data;
            for (E *end=new_data+std::min(n,nnew); dst<end; ++src, ++dst)
               relocate(src, dst);
            if (nnew>n) {
               for (E *end=new_data+nnew; dst<end; ++dst)
                  construct_at(dst, dflt());
            } else {
               for (E *end=data+n; src<end; ++src)
                  destroy_at(src);
            }
            if (data) _allocator.deallocate(data,n_alloc);
            data=new_data;  n_alloc=new_n_alloc;

         } else if (nnew>n) {
            for (E *d=data+n, *end=data+nnew; d<end; ++d)
               construct_at(d, dflt());
         } else {
            for (E *d=data+nnew, *end=data+n; d<end; ++d)
               destroy_at(d);
         }
      }

      void shrink(size_t new_n_alloc, int n)
      {
         if (n_alloc != new_n_alloc) {
            E *new_data=_allocator.allocate(new_n_alloc);
            for (E *src=data, *dst=new_data, *end=new_data+n; dst<end; ++src, ++dst)
               relocate(src, dst);
            _allocator.deallocate(data, n_alloc);
            data=new_data;  n_alloc=new_n_alloc;
         }
      }

      void move_entry(int n_from, int n_to) { relocate(data+n_from, data+n_to); }
      void revive_entry(int n) { construct_at(data+n, dflt()); }
      void delete_entry(int n) { if (!std::is_trivially_destructible<E>::value) destroy_at(data+n); }

      void permute_entries(const std::vector<int>& inv_perm)
      {
         E* new_data=_allocator.allocate(n_alloc);
         int n_from=0;
         for (const int n_to : inv_perm) {
            if (n_to >= 0)
               relocate(data+n_from, new_data+n_to);
            ++n_from;
         }
         _allocator.deallocate(data, n_alloc);
         data=new_data;
      }

      typedef E value_type;

      NodeMapData() {}

      explicit NodeMapData(const default_value_supplier& dflt_arg)
         : data(nullptr)
         , n_alloc(0)
         , dflt(dflt_arg) {}

      ~NodeMapData() {if (_table) { reset(); table().detach(*this); } }

      typedef const_node_container_ref index_container_ref;
      index_container_ref get_index_container() const { return reinterpret_cast<index_container_ref>(ctable()); }
   };

   template <typename E, typename... TParams>
   struct EdgeMapData
      : public EdgeMapDenseBase {
      typedef typename mlist_wrap<TParams...>::type params;
      typedef typename mtagged_list_extract<params, DefaultValueTag, operations::clear<E>>::type default_value_supplier;

      default_value_supplier dflt;
      std::allocator<E> _allocator;

      table_type& table() { return *reinterpret_cast<table_type*>(_table); }
      const table_type& ctable() const { return *reinterpret_cast<const table_type*>(_table); }

      E* index2addr(int i) const
      {
         return EdgeMapDataAccess<E>::index2addr(buckets,i);
      }

      void alloc(const edge_agent_base& h)
      {
         EdgeMapDenseBase::alloc(h.n_alloc);
         void **b=buckets;
         for (int n=h.n_edges; n>0; n-=h.bucket_size, ++b)
            *b=_allocator.allocate(h.bucket_size);
      }

      void init()
      {
         for (auto it=entire(get_index_container()); !it.at_end(); ++it)
            construct_at(index2addr(*it), dflt());
      }

      template <typename Init>
      void init(const Init& val,
                std::enable_if_t<can_initialize<Init, E>::value, void**> =nullptr)
      {
         for (auto it=entire(get_index_container()); !it.at_end(); ++it)
            construct_at(index2addr(*it), val);
      }

      template <typename Iterator>
      void init(Iterator&& src_it,
                std::enable_if_t<assess_iterator_value<Iterator, can_initialize, E>::value, void**> =nullptr)
      {
         decltype(auto) src=ensure_private_mutable(std::forward<Iterator>(src_it));
         for (auto it=entire(get_index_container()); !it.at_end(); ++it, ++src)
            construct_at(index2addr(*it), *src);
      }

      void copy(const EdgeMapData& m)
      {
         dflt=m.dflt;
         typename edge_container<TDir>::const_iterator src=m.get_index_container().begin();
         for (auto it=entire(get_index_container()); !it.at_end(); ++it, ++src)
            construct_at(index2addr(*it), *m.index2addr(*src));
      }

      void reset()
      {
         if (!std::is_trivially_destructible<E>::value)
            for (auto it=entire(get_index_container()); !it.at_end(); ++it)
               destroy_at(index2addr(*it));
         for (E **b=reinterpret_cast<E**>(buckets), **b_end=b+n_alloc; b<b_end; ++b)
            if (*b) _allocator.deallocate(*b,edge_agent_base::bucket_size);
         EdgeMapDenseBase::destroy();
      }

      bool is_detachable() const
      {
         return std::is_trivially_destructible<E>::value;
      }

      void clear()
      {
         if (!std::is_pod<E>::value) {
            operations::clear<E> clr;
            for (auto it=entire(get_index_container()); !it.at_end(); ++it)
               clr(*index2addr(*it));
         }
      }

      void revive_entry(int e) { construct_at(index2addr(e), dflt()); }
      void delete_entry(int e) { if (!std::is_trivially_destructible<E>::value) destroy_at(index2addr(e)); }

      void add_bucket(int n)
      {
         E *d=_allocator.allocate(edge_agent_base::bucket_size);
         construct_at(d, dflt());
         buckets[n]=d;
      }

      typedef E value_type;

      EdgeMapData() {}
      explicit EdgeMapData(const default_value_supplier& dflt_arg) : dflt(dflt_arg) {}
      ~EdgeMapData() { if (_table) { reset(); table().detach(*this); } }

      typedef const edge_container<TDir>& index_container_ref;
      index_container_ref get_index_container() const { return reinterpret_cast<index_container_ref>(ctable()); }
   };

   template <typename E, typename... TParams>
   struct NodeHashMapData : public NodeMapBase {
      typedef typename mlist_wrap<TParams...>::type params;
      typedef typename mtagged_list_extract<params, DefaultValueTag, operations::clear<E>>::type default_value_supplier;

      typedef hash_map<int, E, TParams...> hash_map_t;
      hash_map_t data;

      void init() {}
      void reset(int=0) { data.clear(); }
      void clear() { data.clear(); }
      void shrink(size_t,int) {}
      void revive_entry(int) {}

      void resize(size_t, int n, int nnew)
      {
         while (n>nnew) data.erase(--n);
      }
      void move_entry(int n_from, int n_to)
      {
         auto it=data.find(n_from);
         if (it != data.end()) {
            data.insert(n_to, std::move(it->second));
            data.erase(it);
         }
      }
      void delete_entry(int n) { data.erase(n); }

      void permute_entries(const std::vector<int>& inv_perm)
      {
         hash_map_t new_data;
         int n_from=0;
         for (const int n_to : inv_perm) {
            if (n_to >= 0) {
               auto it=data.find(n_from);
               if (it != data.end())
                  new_data.insert(n_to, std::move(it->second));
            }
         }
         data.swap(new_data);
      }

      void copy(const NodeHashMapData& m) { data=m.data; }

      table_type& table() { return *reinterpret_cast<table_type*>(_table); }
      const table_type& ctable() const { return *reinterpret_cast<const table_type*>(_table); }

      NodeHashMapData() {}
      NodeHashMapData(const default_value_supplier& dflt_arg) : data(dflt_arg) {}
      ~NodeHashMapData() { if (_table) table().detach(*this); }
   };

   template <typename E, typename... TParams>
   struct EdgeHashMapData : public EdgeMapBase {
      typedef typename mlist_wrap<TParams...>::type params;
      typedef typename mtagged_list_extract<params, DefaultValueTag, operations::clear<E>>::type default_value_supplier;

      typedef hash_map<int, E, TParams...> hash_map_t;
      hash_map_t data;

      void reset() { data.clear(); }
      bool is_detachable() const { return true; }
      void clear() { data.clear(); }
      void revive_entry(int) {}
      void realloc(size_t) {}
      void add_bucket(int) {}
      void delete_entry(int e) { data.erase(e); }

      void copy(const EdgeHashMapData& m) { data=m.data; }

      table_type& table() { return *reinterpret_cast<table_type*>(_table); }
      const table_type& ctable() const { return *reinterpret_cast<const table_type*>(_table); }

      EdgeHashMapData() {}
      explicit EdgeHashMapData(const default_value_supplier& dflt_arg) : data(dflt_arg) {}
      ~EdgeHashMapData() { if (_table) table().detach(*this); }
   };

// Due to a bug in clang >=3.8 (https://llvm.org/bugs/show_bug.cgi?id=26938), the SharedMap
// class template cant be resolved in the definition of NodeMap if this forward declaration
// and the friend declaration in map2graph_connector exist at the same time.
// The friend statement suffices as forward declaration.
// TODO: An upper limit on the version will be set depending on the bug resolution.
#if !(defined(__clang__) && ((!defined(__APPLE__) && __clang_major__ == 3 && __clang_minor__ >= 8) || ( defined(__APPLE__) && ( __clang_major__ == 7 && __clang_minor__ >= 3 || __clang_major__ >= 8 ) ) ) )
   template <typename BaseMap> class SharedMap;
#endif

protected:
   template <typename E, typename... TParams, bool for_copy> static
   void prepare_attach(const table_type& t, NodeMapData<E, TParams...>& m, bool_constant<for_copy>)
   {
      m.alloc(t.get_ruler().max_size());
      t.attach(m);
   }

   template <typename E, typename... TParams, bool for_copy> static
   void prepare_attach(const table_type& t, EdgeMapData<E, TParams...>& m, bool_constant<for_copy> C)
   {
      m.alloc(t.get_edge_agent(C));
      t.attach(m);
   }

   template <typename E, typename... TParams, bool for_copy> static
   void prepare_attach(const table_type& t, NodeHashMapData<E, TParams...>& m, bool_constant<for_copy>)
   {
      t.attach(m);
   }

   template <typename E, typename... TParams, bool for_copy> static
   void prepare_attach(const table_type& t, EdgeHashMapData<E, TParams...>& m, bool_constant<for_copy> C)
   {
      (void)t.get_edge_agent(C);
      t.attach(m);
   }

   class divorce_maps;

   class map2graph_connector : public shared_alias_handler {
   protected:
      virtual void divorce(const table_type& t)=0;
      virtual ~map2graph_connector() {}
      friend class divorce_maps;
      friend class Graph<TDir>;
   };

   class divorce_maps : public shared_alias_handler {
   public:
      template <typename Rep>
      Rep* operator() (Rep *body, std::true_type) const
      {
         if (al_set.n_aliases)
            for (auto alias_ptr : al_set)
               static_cast<map2graph_connector*>(alias_ptr->to_handler())->divorce(body->obj);
         return body;
      }

      template <typename Rep>
      Rep* operator() (Rep *body, std::false_type)
      {
         if (al_set.n_aliases) al_set.forget();
         return body;
      }

      friend class Graph<TDir>;
      template <typename> friend class SharedMap;
   };
public:
   template <typename BaseMap>
   class SharedMap : protected map2graph_connector {
   public:
      typedef typename BaseMap::default_value_supplier default_value_supplier;
      typedef BaseMap map_type;
   protected:
      map_type *map;
      default_value_supplier dflt;

      map_type* copy(const table_type& t)
      {
         map_type* cp=new map_type;
         prepare_attach(t, *cp, std::true_type());
         cp->copy(*map);
         return cp;
      }

      void divorce();
      void divorce(const table_type& t);
      void leave()
      {
         if (--map->refc == 0) delete map;
      }
      map_type* mutable_access()
      {
         if (__builtin_expect(map->refc>1, 0)) divorce();
         return map;
      }

      template <bool may_need_detach>
      void attach_to(const Graph& G, bool_constant<may_need_detach>)
      {
         if (may_need_detach && map) {
            if (this->al_set.owner) {
               assert(this->al_set.owner != &G.data.get_divorce_handler().al_set);
               this->al_set.owner->remove(&this->al_set);
            }
            if (&map->table() == G.data.get()) {
               // attached via perl magic object or another Graph 
               this->al_set.enter(G.data.get_divorce_handler().al_set);
               return;
            }
            leave();
         }
         map=new map_type(dflt);
         prepare_attach(*G.data,*map, std::false_type());
         this->al_set.enter(G.data.get_divorce_handler().al_set);
      }

      SharedMap() : map(nullptr) {}

      explicit SharedMap(const default_value_supplier& dflt_arg) : map(nullptr), dflt(dflt_arg) {}

      explicit SharedMap(const Graph& G) { attach_to(G, std::false_type()); }

      SharedMap(const Graph& G, const default_value_supplier& dflt_arg) : dflt(dflt_arg) { attach_to(G, std::false_type()); }

      SharedMap(const SharedMap& m) : map(m.map), dflt(m.dflt) { map->refc++; }

      ~SharedMap() { if (map) leave(); }

      SharedMap& operator= (const SharedMap& m)
      {
         if (m.map) m.map->refc++;
         if (map) leave();
         map=m.map;
         dflt=m.dflt;
         return *this;
      }
   public:
      void clear();

      friend class Graph<TDir>;

      friend void relocate(SharedMap* from, SharedMap* to)
      {
         relocate(static_cast<shared_alias_handler*>(from), static_cast<shared_alias_handler*>(to));
         to->dflt=from->dflt;
         to->map=from->map;
      }

      bool invalid_node(int n) const { return map->ctable().invalid_node(n); }
   };
public:
   template <typename Map>
   void attach(SharedMap<Map>& m) const
   {
      m.attach_to(*this, std::true_type());
      m.map->init();
   }

   template <typename Map, typename Init>
   void attach(SharedMap<Map>& m, Init&& init,
               std::enable_if_t<can_initialize<Init, typename Map::value_type>::value ||
                                assess_iterator_value<Init, can_initialize, typename Map::value_type>::value, void**> =nullptr) const
   {
      m.attach_to(*this, std::true_type());
      m.map->init(std::forward<Init>(init));
   }

protected:
   template <typename TContainer>
   static void prepare_attach_static(TContainer& c, int, io_test::as_set)
   {
      unwary(c).clear();
   }
   template <typename TContainer, bool TAllowSparse>
   static void prepare_attach_static(TContainer& c, int n, io_test::as_array<1, TAllowSparse>)
   {
      unwary(c).resize(n);
   }
   template <typename TContainer, bool TAllowSparse>
   static void prepare_attach_static(TContainer& c, int n, io_test::as_array<0, TAllowSparse>)
   {
      if (POLYMAKE_DEBUG || is_wary<TContainer>()) {
         if (get_dim(unwary(c)) != n)
            throw std::runtime_error("Graph::init_map - dimension mismatch");
      }
   }
public:
   template <typename TContainer>
   void init_node_map(TContainer& c) const
   {
      prepare_attach_static(c, this->nodes(), typename io_test::input_mode<unwary_t<TContainer>>::type());
   }

   template <typename TContainer>
   void init_edge_map(TContainer& c) const
   {
      prepare_attach_static(c, data->get_edge_agent(std::false_type()).n_edges, typename io_test::input_mode<unwary_t<TContainer>>::type());
   }

   /// Provide each edge with a unique integer id.
   /// The ids are assigned in the order of visiting them by Edges<Graph>::iterator,
   /// but you shouldn't rely on any special order later, after having added or deleted
   /// some edges, because the enumeration is effectively performed only once in the life
   /// of a Graph object.  Later calls to this function are no-ops.
   /// Moreover, the edges are also enumerated when an edge attribute map is attached
   /// to the Graph object for the first time.
   void enumerate_edges() const
   {
      data->get_edge_agent(std::false_type());
   }

   friend
   AdjacencyMatrix<Graph, TDir::multigraph>& serialize(Graph& me) { return adjacency_matrix(me); }

   friend
   const AdjacencyMatrix<Graph, TDir::multigraph>& serialize(const Graph& me) { return adjacency_matrix(me); }

protected:
   typedef shared_object<table_type, AliasHandlerTag<shared_alias_handler>, DivorceHandlerTag<divorce_maps>> shared_type;
   shared_type data;

   friend Graph& make_mutable_alias(Graph& alias, Graph& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }

   template <typename Input>
   void read_with_gaps(Input& in)
   {
      const int d = in.get_dim(false);
      clear(d);
      int n = 0;
      for (auto l = entire(out_edge_lists(*this));  !in.at_end();  ++l, ++n) {
         const int i = in.index(d);
         while (n < i) {
            ++l;
            data.get()->delete_node(n++);
         }
         in >> *l;
      }
      while (n < d)
         data.get()->delete_node(n++);
   }

   template <typename Input>
   void read(Input&& in)
   {
      if (in.sparse_representation()) {
         read_with_gaps(in.set_option(SparseRepresentation<std::true_type>()));
      } else {
         clear(in.size());
         for (auto l = entire(out_edge_lists(*this));  !in.at_end();  ++l)
            in >> *l;
      }
   }

   template <typename NodeIterator, typename need_merge, typename need_contraction>
   void copy_impl(NodeIterator src, need_merge, need_contraction, bool has_gaps)
   {
      if (has_gaps) {
         int n = 0, end = dim();
         for (auto l = entire(out_edge_lists(*this));  !src.at_end();  ++l, ++src, ++n) {
            const int i = src.index();
            while (n < i) {
               ++l;
               data.get()->delete_node(n++);
            }
            l->init_from_edge_list(src.out_edges().begin(), need_merge(), need_contraction());
         }
         while (n<end) data.get()->delete_node(n++);
      } else {
         for (auto l = entire(out_edge_lists(*this));  !l.at_end();  ++l, ++src)
            l->init_from_edge_list(src.out_edges().begin(), need_merge(), need_contraction());
      }
   }

   template <typename RowIterator, typename need_merge>
   void copy_impl(RowIterator src, need_merge)
   {
      for (auto l = entire(out_edge_lists(*this));  !l.at_end();  ++l, ++src)
         l->init_from_set(src->begin(), need_merge());
   }

   template <typename, typename, typename...> friend class NodeMap;
   template <typename, typename, typename...> friend class EdgeMap;
   template <typename, typename, typename...> friend class NodeHashMap;
   template <typename, typename, typename...> friend class EdgeHashMap;

   const Graph& get_graph() const { return *this; }     // for HashMaps wanting to attach

#if POLYMAKE_DEBUG
public:
   void check(const char *prefix="") const { data->check(prefix); }
   void dump() const __attribute__((used)) { cerr << adjacency_matrix(*this) << std::flush; }
   void dump_edges() const;
   void dumps() const __attribute__((used)) { dump(); dump_edges(); }
#endif
};

#if POLYMAKE_DEBUG
template <typename TDir>
void Graph<TDir>::dump_edges() const
{
   for (auto e=entire(pretend<const edge_container<TDir>&>()); !e.at_end(); ++e)
      cerr << *e << ':' << e.from_node() << '-' << e.to_node() << '\n';
   cerr << std::flush;
}
#endif


/// data structure to store data at the nodes of a Graph
template <typename TDir, typename E, typename... TParams>
class NodeMap
   : public Graph<TDir>::template SharedMap<typename Graph<TDir>::template NodeMapData<E, TParams...>>
   , public modified_container_impl< NodeMap<TDir, E, TParams...>,
                                     mlist< ContainerRefTag< typename Graph<TDir>::const_node_container_ref >,
                                            OperationTag< operations::random_access<ptr_wrapper<E, false>> > > > {
   typedef modified_container_impl<NodeMap> base_t;
   typedef typename Graph<TDir>::template SharedMap<typename Graph<TDir>::template NodeMapData<E, TParams...>> shared_base;
public:
   typedef typename shared_base::default_value_supplier default_value_supplier;
   typedef Graph<TDir> graph_type;
   typedef random_access_iterator_tag container_category;

   NodeMap() {}

   explicit NodeMap(const default_value_supplier& dflt_arg)
      : shared_base(dflt_arg) {}

   explicit NodeMap(const graph_type& G)
      : shared_base(G)
   {
      this->map->init();
   }

   NodeMap(const graph_type& G, const default_value_supplier& dflt_arg)
      : shared_base(G, dflt_arg)
   {
      this->map->init();
   }

   template <typename Init>
   NodeMap(const graph_type& G, Init&& init,
           std::enable_if_t<can_initialize<Init, E>::value ||
                            assess_iterator_value<Init, can_initialize, E>::value, void**> =nullptr)
      : shared_base(G)
   {
      this->map->init(std::forward<Init>(init));
   }

   template <typename Init>
   NodeMap(const graph_type& G, Init&& init,
           const default_value_supplier& dflt_arg,
           std::enable_if_t<can_initialize<Init, E>::value ||
                            assess_iterator_value<Init, can_initialize, E>::value, void**> =nullptr)
      : shared_base(G, dflt_arg)
   {
      this->map->init(std::forward<Init>(init));
   }

   decltype(auto) get_container()
   {
      return this->mutable_access()->get_index_container();
   }
   decltype(auto) get_container() const
   {
      return this->map->get_index_container();
   }

   auto get_operation()
   {
      return pointer2iterator(this->mutable_access()->data);
   }
   auto get_operation() const
   {
      return pointer2iterator(const_cast<const E*>(this->map->data));
   }

   friend int index_within_range(const NodeMap& me, int n)
   {
      if (n<0) n+=me.map->ctable().dim();
      if (me.invalid_node(n))
         throw std::runtime_error("NodeMap::operator[] - node id out of range or deleted");
      return n;
   }

   E& operator[] (int n)
   {
      return this->mutable_access()->data[n];
   }

   const E& operator[] (int n) const
   {
      return this->map->data[n];
   }

   friend class Graph<TDir>;

   friend void relocate(NodeMap* from, NodeMap* to)
   {
      relocate(static_cast<shared_base*>(from), static_cast<shared_base*>(to));
   }

#if POLYMAKE_DEBUG
private:
   void dump(std::false_type) const { cerr << "elements are not printable" << std::flush; }
   void dump(std::true_type) const { cerr << *this << std::flush; }
public:
   void dump() const __attribute__((used)) { dump(bool_constant<is_printable<E>::value>()); }
#endif
};

/// data structure to store data at the edges of a Graph
template <typename TDir, typename E, typename... TParams>
class EdgeMap
   : public Graph<TDir>::template SharedMap<typename Graph<TDir>::template EdgeMapData<E, TParams...> >
   , public modified_container_impl< EdgeMap<TDir, E, TParams...>,
                                     mlist< ContainerTag< const edge_container<TDir>& >,
                                            OperationTag< EdgeMapDataAccess<E> > > > {
   typedef typename Graph<TDir>::template SharedMap<typename Graph<TDir>::template EdgeMapData<E, TParams...>> shared_base;
public:
   typedef typename shared_base::default_value_supplier default_value_supplier;
   typedef Graph<TDir> graph_type;
   typedef random_access_iterator_tag container_category;

   EdgeMap() {}

   explicit EdgeMap(const default_value_supplier& dflt_arg)
      : shared_base(dflt_arg) {}

   explicit EdgeMap(const graph_type& G)
      : shared_base(G)
   {
      this->map->init();
   }

   EdgeMap(const graph_type& G, const default_value_supplier& dflt_arg)
      : shared_base(G, dflt_arg)
   {
      this->map->init();
   }

   template <typename Init>
   EdgeMap(const graph_type& G, Init&& init,
           std::enable_if_t<can_initialize<Init, E>::value ||
                            assess_iterator_value<Init, can_initialize, E>::value, void**> =nullptr)
      : shared_base(G)
   {
      this->map->init(std::forward<Init>(init));
   }

   template <typename Init>
   EdgeMap(const graph_type& G, Init&& init,
           const default_value_supplier& dflt_arg,
           std::enable_if_t<can_initialize<Init, E>::value ||
                            assess_iterator_value<Init, can_initialize, E>::value, void**> =nullptr)
      : shared_base(G, dflt_arg)
   {
      this->map->init(std::forward<Init>(init));
   }

   decltype(auto) get_container()
   {
      return this->mutable_access()->get_index_container();
   }
   decltype(auto) get_container() const
   {
      return this->map->get_index_container();
   }

   EdgeMapDataAccess<E> get_operation()
   {
      return this->mutable_access()->buckets;
   }
   EdgeMapDataAccess<const E> get_operation() const
   {
      return this->map->buckets;
   }

   E& operator[] (int e)
   {
      return *this->mutable_access()->index2addr(e);
   }
   const E& operator[] (int e) const
   {
      return *this->map->index2addr(e);
   }

   E& operator() (int n1, int n2)
   {
      typename graph_type::node_acc n;
      auto m = this->mutable_access();
      return *m->index2addr(n.out_edge(m->table()[n1], n2));
   }

   const E& operator() (int n1, int n2) const
   {
      typename graph_type::const_node_acc n;
      return *this->map->index2addr(n.out_edge(this->map->table()[n1], n2));
   }

   friend class Graph<TDir>;

   friend void relocate(EdgeMap* from, EdgeMap* to)
   {
      relocate(static_cast<shared_base*>(from), static_cast<shared_base*>(to));
   }

#if POLYMAKE_DEBUG
private:
   void dump(std::false_type) const { cerr << "elements are not printable" << std::flush; }
   void dump(std::true_type) const { cerr << *this << std::flush; }
public:
   void dump() const __attribute__((used)) { dump(bool_constant<is_printable<E>::value>()); }
#endif
};

template <typename TDir, typename E, typename... TParams>
class NodeHashMap
   : public Graph<TDir>::template SharedMap<typename Graph<TDir>::template NodeHashMapData<E, TParams...> >
   , public redirected_container< NodeHashMap<TDir, E, TParams...>,
                                  mlist< ContainerTag< hash_map<int, E, TParams...> > > > {
   typedef redirected_container<NodeHashMap> base_t;
   typedef typename Graph<TDir>::template SharedMap<typename Graph<TDir>::template NodeHashMapData<E, TParams...> > shared_base;
public:
   typedef typename shared_base::default_value_supplier default_value_supplier;
   typedef int key_type;
   typedef E mapped_type;
   typedef hash_map<int, E, TParams...> hash_map_t;

   NodeHashMap() {}

   explicit NodeHashMap(const default_value_supplier& dflt_arg) : shared_base(dflt_arg) {}

   template <typename Graph2>
   explicit NodeHashMap(const GenericGraph<Graph2, TDir>& G) : shared_base(G.top().get_graph()) {}

   template <typename Graph2>
   NodeHashMap(const GenericGraph<Graph2, TDir>& G, const default_value_supplier& dflt_arg) : shared_base(G.top().get_graph(), dflt_arg) {}

   hash_map_t& get_container()
   {
      return this->mutable_access()->data;
   }
   const hash_map_t& get_container() const
   {
      return this->map->data;
   }

   E& operator[] (int n)
   {
      if (this->invalid_node(n))
         throw std::runtime_error("NodeHashMap::operator[] - node id out of range or deleted");
      return get_container()[n];
   }

   const E& operator[] (int n) const
   {
      if (this->invalid_node(n))
         throw std::runtime_error("NodeHashMap::operator[] - node id out of range or deleted");
      typename base_t::const_iterator it=find(n);
      if (it==get_container().end()) throw no_match();
      return it->second;
   }

   typename base_t::iterator insert(int n, const E& v)
   {
      if (POLYMAKE_DEBUG) {
         if (this->invalid_node(n))
            throw std::runtime_error("NodeHashMap::insert - node index out of range or deleted");
      }
      return get_container().insert(n,v);
   }

   pair<typename base_t::iterator, bool> insert(const typename base_t::value_type& v)
   {
      if (POLYMAKE_DEBUG) {
         if (this->invalid_node(v.first))
            throw std::runtime_error("NodeHashMap::insert - node index out of range or deleted");
      }
      return get_container().insert(v);
   }

   typename base_t::const_iterator find(int n) const { return get_container().find(n); }
   void erase(int n) { return get_container().erase(n); }
   typename base_t::iterator erase(typename base_t::iterator where) { return get_container().erase(where); }

   friend class Graph<TDir>;

   friend void relocate(NodeHashMap* from, NodeHashMap* to)
   {
      relocate(static_cast<shared_base*>(from), static_cast<shared_base*>(to));
   }

#if POLYMAKE_DEBUG
private:
   void dump(std::false_type) const { cerr << "elements are not printable" << std::flush; }
   void dump(std::true_type) const { cerr << *this << std::flush; }
public:
   void dump() const __attribute__((used)) { dump(bool_constant<is_printable<E>::value>()); }
#endif
};

template <typename TDir, typename E, typename... TParams>
class EdgeHashMap
   : public Graph<TDir>::template SharedMap<typename Graph<TDir>::template EdgeHashMapData<E, TParams...>>
   , public redirected_container< EdgeHashMap<TDir, E, TParams...>,
                                  mlist< ContainerTag< hash_map<int, E, TParams...> > > > {
   typedef redirected_container<EdgeHashMap> base_t;
   typedef typename Graph<TDir>::template SharedMap<typename Graph<TDir>::template EdgeHashMapData<E, TParams...>> shared_base;
public:
   typedef typename shared_base::default_value_supplier default_value_supplier;
   typedef int key_type;
   typedef E mapped_type;
   typedef hash_map<int, E, TParams...> hash_map_t;

   EdgeHashMap() {}

   explicit EdgeHashMap(const default_value_supplier& dflt_arg) : shared_base(dflt_arg) {}

   template <typename Graph2>
   explicit EdgeHashMap(const GenericGraph<Graph2, TDir>& G) : shared_base(G.top().get_graph()) {}

   template <typename Graph2>
   EdgeHashMap(const GenericGraph<Graph2, TDir>& G, const default_value_supplier& dflt_arg) : shared_base(G.top().get_graph(), dflt_arg) {}

   hash_map_t& get_container()
   {
      return this->mutable_access()->data;
   }
   const hash_map_t& get_container() const
   {
      return this->map->data;
   }

   E& operator[] (int e) { return get_container()[e]; }

   const E& operator[] (int e) const
   {
      typename base_t::const_iterator it=find(e);
      if (it==get_container().end()) throw no_match();
      return it->second;
   }

   E& operator() (int n1, int n2)
   {
      typename Graph<TDir>::node_acc n;
      return get_container()[n.out_edge(this->map->table()[n1], n2)];
   }

   typename base_t::iterator insert(int e, const E& v)
   {
      return get_container().insert(e,v);
   }

   pair<typename base_t::iterator, bool> insert(const typename base_t::value_type& v)
   {
      return get_container().insert(v);
   }

   typename base_t::const_iterator find(int e) const
   {
      return get_container().find(e);
   }

   typename base_t::const_iterator find(int n1, int n2) const
   {
      typename Graph<TDir>::out_edge_list::const_iterator e=this->map->table()[n1].out().find(n2);
      return e.at_end() ? get_container().end() : find(*e);
   }

   void erase(int e)
   {
      return get_container().erase(e);
   }

   typename base_t::iterator erase(typename base_t::iterator where) { return get_container().erase(where); }

   void erase(int n1, int n2)
   {
      auto m = this->mutable_access();
      typename Graph<TDir>::node_acc n;
      auto e=n.out_edges(m->table()[n1]).find(n2);
      if (!e.at_end()) m->data.erase(*e);
   }

   friend class Graph<TDir>;

   friend void relocate(EdgeHashMap* from, EdgeHashMap* to)
   {
      relocate(static_cast<shared_base*>(from), static_cast<shared_base*>(to));
   }

#if POLYMAKE_DEBUG
private:
   void dump(std::false_type) const { cerr << "elements are not printable" << std::flush; }
   void dump(std::true_type) const { cerr << *this << std::flush; }
public:
   void dump() const __attribute__((used)) { dump(bool_constant<is_printable<E>::value>()); }
#endif
};

template <typename TDir>
template <typename TMap>
void Graph<TDir>::SharedMap<TMap>::divorce()
{
   map->refc--;
   map=copy(map->ctable());
}

template <typename TDir>
template <typename TMap>
void Graph<TDir>::SharedMap<TMap>::clear()
{
   if (__builtin_expect(map->refc>1,0)) {
      map->refc--;
      const table_type& t=map->ctable();
      map=new map_type(dflt);
      prepare_attach(t, *map, std::false_type());
   } else {
      map->clear();
   }
}

template <typename TDir>
template <typename TMap>
void Graph<TDir>::SharedMap<TMap>::divorce(const typename Graph<TDir>::table_type& t)
{
   if (map->refc>1) {
      map->refc--;
      map=copy(t);
   } else {
      map->table().detach(*map);
      t.attach(*map);
   }
}

} // end namespace graph

template <typename TDir, typename E, typename... TParams>
struct spec_object_traits< graph::NodeMap<TDir, E, TParams...> > : spec_object_traits<is_container> {
   static const int is_resizeable=0;
};

template <typename TDir, typename E, typename... TParams>
struct check_container_feature< graph::NodeMap<TDir, E, TParams...>, sparse_compatible>
   : std::true_type {};

template <typename TDir, typename E, typename... TParams>
struct spec_object_traits< graph::EdgeMap<TDir, E, TParams...> > : spec_object_traits<is_container> {
   static const int is_resizeable=0;
};

template <typename TDir>
struct spec_object_traits< graph::Graph<TDir> > : spec_object_traits<is_opaque> {
   static const int is_resizeable=1;
};

template <typename TDir>
struct spec_object_traits< Serialized<graph::Graph<TDir>> >
   : spec_object_traits< AdjacencyMatrix<graph::Graph<TDir>> > { };

template <typename TGraph, typename TPerm>
typename TGraph::persistent_type
permuted_nodes(const GenericGraph<TGraph>& g, const TPerm& perm)
{
   if (POLYMAKE_DEBUG || is_wary<TGraph>()) {
      if (g.top().dim() != perm.size())
         throw std::runtime_error("permuted_nodes - dimension mismatch");
   }
   std::vector<int> inv_perm(g.nodes());
   inverse_permutation(perm, inv_perm);
   return g.top().copy_permuted(perm, inv_perm);
}

template <typename TGraph, typename TPerm>
std::enable_if_t<container_traits<TPerm>::is_random, typename TGraph::persistent_type>
permuted_inv_nodes(const GenericGraph<TGraph>& g, const TPerm& inv_perm)
{
   if (POLYMAKE_DEBUG || is_wary<TGraph>()) {
      if (g.top().dim() != inv_perm.size())
         throw std::runtime_error("permuted_inv_nodes - dimension mismatch");
   }
   std::vector<int> perm(g.nodes());
   inverse_permutation(inv_perm, perm);
   return g.top().copy_permuted(perm, inv_perm);
}

template <typename TGraph, typename TPerm>
std::enable_if_t<!container_traits<TPerm>::is_random, typename TGraph::persistent_type>
permuted_inv_nodes(const GenericGraph<TGraph>& g, const TPerm& inv_perm)
{
   if (POLYMAKE_DEBUG || is_wary<TGraph>()) {
      if (g.top().dim() != inv_perm.size())
         throw std::runtime_error("permuted_inv_nodes - dimension mismatch");
   }
   std::vector<int> inv_perm_copy(inv_perm.size());
   copy_range(entire(inv_perm), inv_perm_copy.begin());
   return permuted_inv_rows(g,inv_perm_copy);
}

template <typename TDir>
class Wary< graph::Graph<TDir> >
   : public WaryGraph< graph::Graph<TDir> > {
protected:
   Wary();
   ~Wary();
};

template <typename TDir, typename E, typename... TParams>
class Wary< graph::EdgeMap<TDir, E, TParams...> >
   : public Generic< Wary< graph::EdgeMap<TDir, E, TParams...> > > {
protected:
   Wary();
   ~Wary();
public:
   E& operator() (int n1, int n2)
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("EdgeMap::operator() - node id out of range or deleted");
      return this->top()(n1,n2);
   }
   const E& operator() (int n1, int n2) const
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("EdgeMap::operator() - node id out of range or deleted");
      return this->top()(n1,n2);
   }
};

template <typename TDir, typename E, typename... TParams>
class Wary< graph::EdgeHashMap<TDir, E, TParams...> >
   : public Generic< Wary< graph::EdgeHashMap<TDir, E, TParams...> > > {
protected:
   Wary();
   ~Wary();
public:
   E& operator() (int n1, int n2)
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("EdgeHashMap::operator() - node id out of range or deleted");
      return this->top()(n1, n2);
   }
   typename graph::EdgeHashMap<TDir, E, TParams...>::const_iterator find(int n1, int n2) const
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("EdgeHashMap::find - node id out of range or deleted");
      return this->top().find(n1,n2);
   }
   void erase(int n1, int n2)
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("EdgeHashMap::erase - node id out of range or deleted");
      this->top().erase(n1, n2);
   }
};

/// create complete graph on given number of nodes
inline
graph::Graph<> complete_graph(int n_nodes)
{
   graph::Graph<> G(n_nodes);
   for (int n=0; n<n_nodes; ++n)
      G.adjacent_nodes(n)=sequence(0,n_nodes)-n;
   return G;
}

} // end namespace pm

namespace polymake {
   using pm::graph::Graph;
   using pm::graph::NodeMap;
   using pm::graph::EdgeMap;
   using pm::graph::NodeHashMap;
   using pm::graph::EdgeHashMap;
   using pm::complete_graph;
}

namespace std {
   template <typename TDir>
   void swap(pm::graph::Graph<TDir>& G1, pm::graph::Graph<TDir>& G2) { G1.swap(G2); }
}

#endif // POLYMAKE_GRAPH_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
