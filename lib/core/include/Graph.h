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
#include <cassert>

namespace pm {
namespace graph {

using pm::sparse2d::restriction_kind;
using pm::sparse2d::full;
using pm::sparse2d::dying;
using pm::sparse2d::relocate;
using pm::relocate;

template <typename dir> class Table;
template <typename dir, restriction_kind restriction=full> struct node_entry;
template <typename Traits> struct dir_permute_entries;
template <typename Traits> struct undir_permute_entries;

template <typename dir, typename E, typename Params=void> class NodeMap;
template <typename dir, typename E, typename Params=void> class EdgeMap;
template <typename dir, typename E, typename Params=void> class NodeHashMap;
template <typename dir, typename E, typename Params=void> class EdgeHashMap;

struct edge_agent_base {
   int n_edges, n_alloc;
   edge_agent_base() : n_edges(0), n_alloc(0) {}

   static const int bucket_shift=8, bucket_size=1<<bucket_shift, bucket_mask=bucket_size-1;
   static int min_buckets(int b) { return b>=10 ? b : 10; }

   template <typename MapList>
   bool extend_maps(MapList& maps);
};

template <typename dir>
struct edge_agent : edge_agent_base {
   Table<dir>* table;
   edge_agent() : table(NULL) {}

   typedef sparse2d::cell<int> Cell;

   template <bool for_copy>
   void init(Table<dir> *t, bool2type<for_copy>);

   template <typename NumberConsumer>
   void renumber(const NumberConsumer& nc);

   void reset() { n_alloc=0; table=NULL; }

   void added(Cell *c)
   {
      if (table != NULL)
         table->_edge_added(*this,c);
      else
         n_alloc=0;
      ++n_edges;
   }
   void removed(Cell *c)
   {
      --n_edges;
      if (table != NULL)
         table->_edge_removed(c);
      else
         n_alloc=0;
   }
};

template <typename dir, bool _out_edges>
class it_traits : public sparse2d::it_traits<int, _out_edges, dir::value> {
   typedef sparse2d::it_traits<int, _out_edges, dir::value> _super;
public:
   it_traits(int index_arg=0) : _super(index_arg) {}

   AVL::Ptr<typename _super::Node>& link(typename _super::Node *n, AVL::link_index X) const
   {
      return (dir::value && n->key<0) ? n->links[X-AVL::L] : _super::link(n,X);
   }

   const it_traits& get_it_traits() const { return *this; }
};

template <typename dir, bool _out_edges /* =false */, restriction_kind restriction>
class traits_base : public it_traits<dir, _out_edges> {
public:
   typedef it_traits<dir, _out_edges> traits_for_iterator;
   typedef typename traits_for_iterator::Node Node;
protected:
   mutable AVL::Ptr<Node> root_links[3];
public:
   typedef int mapped_type;

   static const bool
      symmetric=dir::value,
      row_oriented=_out_edges /* =false */,
      allow_multiple=dir::multigraph;

   typedef AVL::tree< sparse2d::traits<traits_base, symmetric, restriction> > own_tree;
   typedef AVL::tree< sparse2d::traits<traits_base<dir, (!symmetric && !row_oriented), restriction>, symmetric, restriction> >
      cross_tree;
protected:
   typedef node_entry<dir, restriction> entry;
   typedef sparse2d::ruler<entry, edge_agent<dir> > own_ruler;
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

   friend class Table<dir>;
   template <typename> friend struct sparse2d::sym_permute_entries;
   template <typename> friend struct dir_permute_entries;
   template <typename> friend struct undir_permute_entries;
};

template <typename dir, restriction_kind restriction>
class traits_base<dir, true, restriction> {
protected:
   typedef sparse2d::cell<int> Node;

   mutable AVL::Ptr<Node> root_links[3];
public:
   typedef int mapped_type;

   static const bool
      symmetric=false,
      row_oriented=true,
      allow_multiple=dir::multigraph;

   typedef it_traits<dir, true> traits_for_iterator;
   typedef traits_base<dir, false, restriction> cross_traits_base;

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
   typedef node_entry<dir, restriction> entry;
   typedef sparse2d::ruler<entry, edge_agent<dir> > own_ruler;
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

   friend class Table<dir>;
};

template <typename dir, restriction_kind restriction, bool _symmetric=dir::value /* =true */>
struct node_entry_trees {
   typedef AVL::tree< sparse2d::traits<traits_base<dir, false, restriction>, _symmetric, restriction> >
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

   static const node_entry<dir, restriction>*
   reverse_cast(const out_tree_type* t)
   {
      return static_cast<const node_entry<dir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_out));
   }
   static node_entry<dir, restriction>*
   reverse_cast(out_tree_type* t)
   {
      return static_cast<node_entry<dir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_out));
   }
};

template <typename dir, restriction_kind restriction>
struct node_entry_trees<dir, restriction, false> {
   typedef AVL::tree< sparse2d::traits<traits_base<dir, true, restriction>, false, restriction> >
      out_tree_type;
   typedef AVL::tree< sparse2d::traits<traits_base<dir, false, restriction>, false, restriction> >
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

   static const node_entry<dir, restriction>*
   reverse_cast(const out_tree_type* t)
   {
      return static_cast<const node_entry<dir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_out));
   }
   static node_entry<dir, restriction>*
   reverse_cast(out_tree_type* t)
   {
      return static_cast<node_entry<dir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_out));
   }
   static const node_entry<dir, restriction>*
   reverse_cast(const in_tree_type* t)
   {
      return static_cast<const node_entry<dir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_in));
   }
   static node_entry<dir, restriction>*
   reverse_cast(in_tree_type* t)
   {
      return static_cast<node_entry<dir, restriction>*>(pm::reverse_cast(t, &node_entry_trees::_in));
   }
};

template <typename _dir, restriction_kind restriction>
struct node_entry :
   public node_entry_trees<_dir, restriction> {
   typedef _dir dir;
   typedef sparse2d::ruler<node_entry, edge_agent<dir> > ruler;

   explicit node_entry(int index_arg) : node_entry_trees<_dir, restriction>(index_arg) {}

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
      if (_dir::value==Directed::value) relocate(&(from->in()), &(to->in()));
   }
};

template <typename Traits>
struct dir_permute_entries {
   typedef typename Traits::ruler ruler;
   typedef typename Traits::entry entry;
   typedef typename Traits::out_tree_type out_tree_type;
   typedef typename Traits::in_tree_type in_tree_type;
   typedef typename out_tree_type::Node Node;

   static void relocate(entry *from, entry *to)
   {
      relocate_tree(&from->_out, &to->_out, False());
      relocate_tree(&from->_in, &to->_in, False());
   }

   static void complete_in_trees(ruler* R)
   {
      int nfrom=0;
      for (typename Entire<ruler>::iterator ri=entire(*R); !ri.at_end();  ++ri, ++nfrom) {
         for (typename out_tree_type::iterator e=ri->_out.begin(); !e.at_end(); ++e) {
            Node *node=e.operator->();
            (*R)[node->key-nfrom]._in.push_back_node(node);
         }
      }
   }

   // FIXME: handle gaps properly!
   void operator()(ruler* Rold, ruler* R) const
   {
      std::vector<int> inv_perm(R->size());
      int nto=0;
      for (typename Entire<ruler>::iterator ri=entire(*R);  !ri.at_end();  ++ri, ++nto)
         inv_perm[ri->_in.line_index]=nto;

      nto=0;
      for (typename Entire<ruler>::iterator ri=entire(*R);  !ri.at_end();  ++ri, ++nto) {
         const int old_nto=ri->_in.line_index;
         ri->_in.line_index=nto;
         for (typename in_tree_type::iterator e=(*Rold)[old_nto]._in.begin(); !e.at_end(); ++e) {
            Node *node=e.operator->();
            const int old_nfrom=node->key-old_nto, nfrom=inv_perm[old_nfrom];
            node->key=nfrom+nto;
            (*R)[nfrom]._out.push_back_node(node);
         }
      }

      complete_in_trees(R);
   }

   template <typename Perm, typename InvPerm>
   static void copy(ruler* Rold, ruler* R, const Perm& perm, const InvPerm& inv_perm)
   {
      const int nn=R->size();
      typename Perm::const_iterator p=perm.begin();
      for (int nto=0; nto<nn; ++nto, ++p) {
         const int old_nto=*p;
         for (typename in_tree_type::const_iterator e=(*Rold)[old_nto]._in.begin(); !e.at_end(); ++e) {
            const Node *node=e.operator->();
            const int old_nfrom=node->key-old_nto, nfrom=inv_perm[old_nfrom];
            out_tree_type& t=(*R)[nfrom]._out;
            t.push_back_node(new(t.allocate_node()) Node(nfrom+nto));
         }
      }

      complete_in_trees(R);
   }
};

template <typename Traits>
struct undir_permute_entries : sparse2d::sym_permute_entries<Traits> {
   typedef typename Traits::entry entry;
   static void relocate(entry *from, entry *to)
   {
      relocate_tree(&from->_out, &to->_out, False());
   }
};

struct NodeMapBase {
   ptr_pair<NodeMapBase> ptrs;
   long refc;
   void *_table;

   NodeMapBase() : refc(1), _table(0) {}

   virtual ~NodeMapBase() {}
   virtual void init()=0;
   virtual void reset(int n=0)=0;
   virtual void resize(size_t n_alloc_new, int n, int nnew)=0;
   virtual void shrink(size_t n_alloc_new, int n)=0;
   virtual void move_entry(int n, int nnew)=0;
   virtual void revive_entry(int n)=0;
   virtual void delete_entry(int n)=0;
};

template <typename> class EdgeMapDataAccess;

struct EdgeMapBase {
   ptr_pair<EdgeMapBase> ptrs;
   long refc;
   void *_table;

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

   EdgeMapDenseBase() : buckets(0) {}
   void alloc(size_t n)
   {
      n_alloc=n;
      buckets=new void*[n];
      std::memset(buckets, 0, n*sizeof(void*));
   }
   void realloc(size_t new_n_alloc)
   {
      if (new_n_alloc > n_alloc) {
         void **old_buckets=buckets;
         buckets=new void*[new_n_alloc];
         std::memcpy(buckets, old_buckets, n_alloc*sizeof(void*));
         std::memset(buckets+n_alloc, 0, (new_n_alloc-n_alloc)*sizeof(void*));
         delete[] old_buckets;
         n_alloc=new_n_alloc;
      }
   }

   void destroy() { delete[] buckets; buckets=NULL; n_alloc=0; }
};

template <typename MapList>
bool edge_agent_base::extend_maps(MapList& maps)
{
   if (n_edges&bucket_mask) return false;

   const int new_bucket=n_edges>>bucket_shift;
   if (new_bucket>=n_alloc) {
      n_alloc+=min_buckets(n_alloc/5);
      for (typename Entire<MapList>::iterator m=entire(maps); !m.at_end(); ++m) {
         m->realloc(n_alloc);
         m->add_bucket(new_bucket);
      }
   } else {
      for (typename Entire<MapList>::iterator m=entire(maps); !m.at_end(); ++m)
         m->add_bucket(new_bucket);
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

template <typename _dir>
class Table {
public:
   typedef _dir dir;
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
   template <typename Set>
   static int _get_dim(const Set& s, typename enable_if<int, check_container_feature<Set, sparse_compatible>::value>::type=0)
   {
      return s.dim();
   }
   template <typename Set>
   static int _get_dim(const Set& s, typename disable_if<int, check_container_feature<Set, sparse_compatible>::value>::type=0)
   {
      return s.empty() ? 0 : s.back()+1;
   }
public:
   template <typename Set>
   explicit Table(const GenericSet<Set>& s)
      : R(ruler::construct(_get_dim(s.top()))), n_nodes(R->size()), free_node_id(std::numeric_limits<int>::min())
   {
      if (!identical<Set,sequence>::value || s.top().size()!=n_nodes)
         init_delete_nodes(sequence(0,n_nodes)-s);
   }

   template <typename Set>
   Table(const GenericSet<Set>& s, int dim)
      : R(ruler::construct(dim)), n_nodes(dim), free_node_id(std::numeric_limits<int>::min())
   {
      if (!identical<Set,sequence>::value || s.top().size()!=n_nodes)
         init_delete_nodes(sequence(0,n_nodes)-s);
   }

protected:
   void detach_node_maps()
   {
      for (Entire<node_map_list>::iterator it=entire(node_maps); !it.at_end(); ) {
         NodeMapBase *m=it.operator->();  ++it;
         m->reset(); m->_table=NULL;
         detach(*m);
      }
   }
   void detach_edge_maps()
   {
      for (Entire<edge_map_list>::iterator it=entire(edge_maps); !it.at_end(); ) {
         EdgeMapBase *m=it.operator->();  ++it;
         m->reset(); m->_table=NULL;
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
      std::swap(node_maps, t.node_maps);
      std::swap(edge_maps, t.edge_maps);
      std::swap(n_nodes, t.n_nodes);
      std::swap(free_node_id, t.free_node_id);
      std::swap(free_edge_ids, t.free_edge_ids);
      for (Entire<node_map_list>::iterator it=entire(node_maps); !it.at_end(); ++it)
         it->_table=this;
      for (Entire<node_map_list>::iterator it=entire(t.node_maps); !it.at_end(); ++it)
         it->_table=&t;
      for (Entire<edge_map_list>::iterator it=entire(edge_maps); !it.at_end(); ++it)
         it->_table=this;
      for (Entire<edge_map_list>::iterator it=entire(t.edge_maps); !it.at_end(); ++it)
         it->_table=&t;
   }

   void clear(int n=0)
   {
      for (Entire<node_map_list>::iterator it=entire(node_maps); !it.at_end(); ++it) it->reset(n);
      for (Entire<edge_map_list>::iterator it=entire(edge_maps); !it.at_end(); ++it) it->reset();
      R->prefix().table=NULL;
      R=ruler::resize_and_clear(R,n);
      edge_agent<dir>& h=R->prefix();
      if (!edge_maps.empty()) h.table=this;
      h.n_alloc=0;
      h.n_edges=0;
      n_nodes=n;
      if (n) for (Entire<node_map_list>::iterator it=entire(node_maps); !it.at_end(); ++it) it->init();
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
      for (Entire<node_map_list>::iterator it=entire(node_maps); !it.at_end(); ++it) it->revive_entry(n);
      ++n_nodes;
      return n;
   }

   // is only called when the maps do not contain any gaps
   void _resize(int nnew)
   {
      R=ruler::resize(R,nnew);
      for (Entire<node_map_list>::iterator it=entire(node_maps); !it.at_end(); ++it) it->resize(R->max_size(),n_nodes,nnew);
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
      for (Entire<edge_map_list>::iterator it=entire(edge_maps); !it.at_end(); ++it) it->revive_entry(id);
   }

   void _edge_removed(Cell *c)
   {
      const int id=c->data;
      for (Entire<edge_map_list>::iterator it=entire(edge_maps); !it.at_end(); ++it) it->delete_entry(id);
      free_edge_ids.push_back(id);
   }
public:
   int add_node()
   {
      if (free_node_id!=std::numeric_limits<int>::min())
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
      for (Entire<node_map_list>::iterator it=entire(node_maps); !it.at_end(); ++it) it->delete_entry(n);
      --n_nodes;
   }

protected:
   template <typename List>
   void init_delete_nodes(const List& l)
   {
      for (typename Entire<List>::const_iterator it=entire(l); !it.at_end(); ++it) {
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
      for (typename Entire<Tree>::iterator e=entire(t); !e.at_end(); ++e)
         e->key -= diff;
   }
   void renumber_nodes_in_edges(in_tree_type& t, int nnew, int diff, Undirected)
   {
      const int diag=2*t.line_index;
      for (typename Entire<in_tree_type>::iterator e=entire(t); !e.at_end(); ) {
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
      for (entry *t=R->begin(), *end=R->end(); t!=end; ++t, ++n) {
         const int what=to_delete(*t);
         if (what==0) {
            if (int diff=n-nnew) {
               if (is_directed) t->in().line_index=nnew;
               renumber_nodes_in_edges(t->out(), nnew, diff, dir());
               if (is_directed) renumber_nodes_in_edges(t->in(), nnew, diff, dir());
               relocate(t, t-diff);
               for (Entire<node_map_list>::iterator it=entire(node_maps); !it.at_end(); ++it) it->move_entry(n,nnew);
            }
            nc(n, nnew);  ++nnew;
         } else {
            if (what>0) {
               for (Entire<node_map_list>::iterator it=entire(node_maps); !it.at_end(); ++it) it->delete_entry(n);
               --n_nodes;
            }
            std::_Destroy(t);
         }
      }
      if (nnew < n) {
         R=ruler::resize(R,nnew,false);
         for (Entire<node_map_list>::iterator it=entire(node_maps); !it.at_end(); ++it) it->shrink(R->max_size(),nnew);
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
      for (Entire<edge_map_list>::iterator it=entire(edge_maps); !it.at_end(); ++it) {
         if (!it->is_detachable())
            throw std::runtime_error("can't renumber edge IDs - non-trivial data attached");
      }
      R->prefix().renumber(nc);
      detach_edge_maps();
   }

   void resize(int n)
   {
      if (n > n_nodes) {
         while (free_node_id!=std::numeric_limits<int>::min()) {
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
      typedef out_tree_type tree_type;
      typedef typename Table::entry entry;
      static tree_type& tree(entry& e) { return e.out(); }
      static const tree_type& tree(const entry& e) { return e.out(); }
   };

   static
   dir_permute_entries<Table> permute_entries(Directed)
   {
      return dir_permute_entries<Table>();
   }
   static
   undir_permute_entries<undir_perm_traits> permute_entries(Undirected)
   {
      return undir_permute_entries<undir_perm_traits>();
   }

public:
   template <typename Iterator, typename _inverse>
   void permute_nodes(Iterator perm, _inverse)
   {
      // FIXME: permute node maps!
      R=ruler::permute(R, perm, permute_entries(dir()), _inverse());
   }

   template <typename Perm, typename InvPerm>
   void copy_permuted(const Table& src, const Perm& perm, const InvPerm& inv_perm)
   {
      permute_entries(dir()).copy(src.R, R, perm, inv_perm);
      if (src.free_node_id != std::numeric_limits<int>::min()) {
         int *index_p=&free_node_id, ns=src.free_node_id;
         do {
            const int n=inv_perm[~ns];
            *index_p=~n;
            index_p=&(*R)[n].in().line_index;
            ns=(*src.R)[~ns].in().line_index;
         } while (ns != std::numeric_limits<int>::min());
         *index_p=ns;
         n_nodes=src.n_nodes;
      }
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
   const edge_agent<_dir>& get_edge_agent(bool2type<for_copy> C) const
   {
      edge_agent<_dir>& h=R->prefix();
      if (!h.table) h.init(const_cast<Table*>(this), C);
      return h;
   }
};

struct edge_accessor :
   public sparse2d::cell_accessor< sparse2d::cell<int> > {

   template <typename Iterator>
   class mix_in : public Iterator {
   public:
      mix_in() {}
      mix_in(const Iterator& cur_arg) : Iterator(cur_arg) {}
      mix_in(typename alt_constructor<Iterator>::arg_type& cur_arg) : Iterator(cur_arg) {}

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
class incident_edge_list :
   public modified_tree< incident_edge_list<Tree>,
                         list( Operation< pair< edge_accessor, BuildUnaryIt<sparse2d::cell_index_accessor> > >,
                               Hidden< Tree > ) > {
   typedef modified_tree<incident_edge_list> _super;
   template <typename> friend class Graph;
protected:
   ~incident_edge_list();

   template <typename Iterator>
   void copy(Iterator src)
   {
      typename _super::iterator dst=this->begin();
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
   bool init_from_set(Iterator src, False)
   {
      typename _super::iterator dst=this->end();
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
   void init_from_set(Iterator src, True)
   {
      typename _super::iterator dst=this->begin();
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

   template <typename Iterator, typename _need_merge>
   void init_from_edge_list(Iterator src, _need_merge, False)
   {
      init_from_set(make_unary_transform_iterator(src, BuildUnaryIt<operations::index2element>()), _need_merge());
   }

   template <typename Iterator, typename _need_merge>
   void init_from_edge_list(Iterator src, _need_merge, True)
   {
      init_from_set(make_equal_range_contractor(make_unary_transform_iterator(src, BuildUnaryIt<operations::index2element>())), _need_merge());
   }

   template <typename Input>
   void init_multi_from_sparse(Input& src)
   {
      if (!Input::template get_option< TrustedValue<True> >::value &&
          src.lookup_dim(false) != dim())
         throw std::runtime_error("multigraph input - dimension mismatch");

      typename _super::iterator dst=this->end();
      const int diag= Tree::symmetric ? this->hidden().get_line_index() : 0;

      while (!src.at_end()) {
         const int index=src.index();
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
      if (!Input::template get_option<TrustedValue<True> >::value &&
          src.size() != dim())
         throw std::runtime_error("multigraph input - dimension mismatch");

      typename _super::iterator dst=this->end();
      const int diag= Tree::symmetric ? this->hidden().get_line_index() : 0;

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

   template <typename Input>
   void read(Input& in, False)
   {
      typedef typename Input::template list_cursor< std::list<int> >::type cursor;
      cursor src=in.begin_list((std::list<int>*)0);
      if (init_from_set(list_reader<int, cursor&>(src), False())) src.skip_rest();
      src.finish();
   }

   template <typename Input>
   void read(Input& in, True)
   {
      typedef typename Input::template list_cursor< SparseVector<int> >::type cursor;
      cursor src=in.begin_list((SparseVector<int>*)0);
      if (src.sparse_representation())
         init_multi_from_sparse(src.set_option(SparseRepresentation<True>()));
      else
         init_multi_from_dense(src.set_option(SparseRepresentation<False>()));
      src.finish();
   }

public:
   static const bool multigraph=Tree::allow_multiple;

   int dim() const { return this->max_size(); }

   incident_edge_list& operator= (const incident_edge_list& l)
   {
      copy(entire(l));
      return *this;
   }

   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& in, incident_edge_list& me)
   {
      me.read(in.top(), bool2type<multigraph>());
      return in.top();
   }

   typedef typename if_else<multigraph,
                            input_truncator<typename _super::iterator, truncate_after_index>,
                            single_position_iterator<typename _super::iterator> >::type
      parallel_edge_iterator;

   typedef typename if_else<multigraph,
                            input_truncator<typename _super::const_iterator, truncate_after_index>,
                            single_position_iterator<typename _super::const_iterator> >::type
      parallel_edge_const_iterator;

private:
   parallel_edge_const_iterator all_edges_to(int n2, False) const
   {
      return this->find(n2);
   }

   parallel_edge_iterator all_edges_to(int n2, False)
   {
      return this->find(n2);
   }

   parallel_edge_const_iterator all_edges_to(int n2, True) const
   {
      return parallel_edge_const_iterator(this->find_nearest(n2, first_of_equal()), truncate_after_index(n2));
   }

   parallel_edge_iterator all_edges_to(int n2, True)
   {
      return parallel_edge_iterator(this->find_nearest(n2, first_of_equal()), truncate_after_index(n2));
   }

   void delete_all_edges_to(int n2, False)
   {
      this->erase(n2);
   }

   void delete_all_edges_to(int n2, True)
   {
      for (parallel_edge_iterator e=all_edges_to(n2, True()); !e.at_end(); )
         this->erase(e++);
   }
public:
   parallel_edge_const_iterator all_edges_to(int n2) const
   {
      return all_edges_to(n2, bool2type<multigraph>());
   }

   parallel_edge_iterator all_edges_to(int n2)
   {
      return all_edges_to(n2, bool2type<multigraph>());
   }

   void delete_all_edges_to(int n2)
   {
      delete_all_edges_to(n2, bool2type<multigraph>());
   }
};

template <typename Tree>
class lower_incident_edge_list
   : public modified_container_impl< lower_incident_edge_list<Tree>,
                                     list( Hidden< incident_edge_list<Tree> >,
                                           IteratorConstructor< input_truncator_constructor >,
                                           Operation< BuildUnaryIt<uniq_edge_predicate> > ) > {
public:
   int dim() const { return this->hidden().dim(); }
};


template <typename Tree>
class multi_adjacency_line :
   public modified_container_impl< multi_adjacency_line<Tree>,
                                   list( Hidden< incident_edge_list<Tree> >,
                                         IteratorConstructor< range_folder_constructor >,
                                         Operation< equal_index_folder > ) >,
   public GenericVector<multi_adjacency_line<Tree>, int> {
protected:
   ~multi_adjacency_line();
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

   typedef typename entry_type::out_tree_type out_tree_type;
   typedef typename entry_type::in_tree_type in_tree_type;
   typedef incident_edge_list<out_tree_type> out_edge_list;
   typedef incident_edge_list<in_tree_type> in_edge_list;
   typedef typename if_else<out_edge_list::multigraph, multi_adjacency_line<out_tree_type>, incidence_line<out_tree_type> >::type out_adjacent_node_list;
   typedef typename if_else<out_edge_list::multigraph, multi_adjacency_line<in_tree_type>,  incidence_line<in_tree_type>  >::type in_adjacent_node_list;
   typedef out_adjacent_node_list adjacent_node_list;
   typedef typename inherit_ref<out_edge_list, EntryRef>::type out_edge_list_ref;
   typedef typename inherit_ref<in_edge_list, EntryRef>::type in_edge_list_ref;
   typedef typename inherit_ref<out_adjacent_node_list, EntryRef>::type out_adjacent_node_list_ref;
   typedef typename inherit_ref<in_adjacent_node_list, EntryRef>::type in_adjacent_node_list_ref;
   typedef out_adjacent_node_list_ref adjacent_node_list_ref;

   out_edge_list_ref out_edges(EntryRef t) const
   {
      return reinterpret_cast<out_edge_list_ref>(t.out());
   }
   in_edge_list_ref in_edges(EntryRef t) const
   {
      return reinterpret_cast<in_edge_list_ref>(t.in());
   }

   typename out_tree_type::const_iterator _out_edge(EntryRef t, int n2, True) const
   {
      typename out_tree_type::const_iterator e=t.out().find(n2);
      if (e.at_end()) throw no_match("non-existing edge");
      return e;
   }
   typename in_tree_type::const_iterator _in_edge(EntryRef t, int n2, True) const
   {
      typename in_tree_type::const_iterator e=t.in().find(n2);
      if (e.at_end()) throw no_match("non-existing edge");
      return e;
   }

   typename out_tree_type::iterator _out_edge(EntryRef t, int n2, False) const
   {
      return t.out().insert(n2);
   }
   typename in_tree_type::iterator _in_edge(EntryRef t, int n2, False) const
   {
      return t.in().insert(n2);
   }
   int out_edge(EntryRef t, int n2) const
   {
      return _out_edge(t, n2, bool2type<attrib<EntryRef>::is_const>())->data;
   }
   int in_edge(EntryRef t, int n2) const
   {
      return _in_edge(t, n2, bool2type<attrib<EntryRef>::is_const>())->data;
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

template <bool _out_edges, template <typename> class MasqueradeLine, typename NodeIterator=void>
class line_factory;

template <typename Iterator, typename Accessor>
struct valid_node_iterator : public unary_predicate_selector<Iterator, Accessor> {
   typedef unary_predicate_selector<Iterator, Accessor> _super;
   template <bool, template <typename> class, typename> friend class line_factory;
public:
   typedef valid_node_iterator<typename iterator_traits<Iterator>::iterator, Accessor> iterator;
   typedef valid_node_iterator<typename iterator_traits<Iterator>::const_iterator, Accessor> const_iterator;
   typedef random_access_iterator_tag iterator_category;

   valid_node_iterator() {}

   template <typename Accessor2>
   valid_node_iterator(const valid_node_iterator<typename iterator_traits<Iterator>::iterator, Accessor2>& it)
      : _super(it) {}

   valid_node_iterator(const Iterator& cur_arg, const Accessor& acc_arg=Accessor())
      : _super(cur_arg,acc_arg) {}

   valid_node_iterator& operator++ () { _super::operator++(); return *this; }
   const valid_node_iterator operator++ (int) { valid_node_iterator copy(*this); _super::operator++(); return copy; }

   valid_node_iterator& operator-- () { _super::operator--(); return *this; }
   const valid_node_iterator operator-- (int) { valid_node_iterator copy(*this); _super::operator--(); return copy; }

   // random access is based on the absolute node index, not on the number of valid nodes in between!
   valid_node_iterator& operator+= (int i)
   {
      static_cast<Iterator&>(*this)+=i;
      return *this;
   }
   const valid_node_iterator operator+ (int i) const { valid_node_iterator copy(*this); return copy+i; }
   friend
   const valid_node_iterator operator+ (int i, const valid_node_iterator& me) { return me+i; }

   valid_node_iterator& operator-= (int i)
   {
      static_cast<Iterator&>(*this)-=i;
      return *this;
   }
   const valid_node_iterator operator- (int i) { valid_node_iterator copy(*this); return copy-i; }
   ptrdiff_t operator- (const valid_node_iterator& it) const { return static_cast<const Iterator&>(*this)-it; }

   int index() const { return (**this).get_line_index(); }

   typedef typename _super::helper::operation::out_edge_list_ref out_edge_list_ref;
   typedef typename _super::helper::operation::in_edge_list_ref in_edge_list_ref;
   typedef typename _super::helper::operation::out_adjacent_node_list_ref out_adjacent_node_list_ref;
   typedef typename _super::helper::operation::in_adjacent_node_list_ref in_adjacent_node_list_ref;
   typedef typename _super::helper::operation::adjacent_node_list_ref adjacent_node_list_ref;

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

template <typename dir>
class valid_node_container
   : public modified_container_impl< valid_node_container<dir>,
                                     list( Container< typename Table<dir>::ruler >,
                                           Operation< BuildUnary<valid_node_selector> >,
                                           IteratorConstructor< valid_node_access_constructor >,
                                           Hidden< Table<dir> > ) > {
   typedef modified_container_impl<valid_node_container> _super;
protected:
   ~valid_node_container();
public:
   typedef random_access_iterator_tag container_category;

   typename _super::container& get_container() { return this->hidden().get_ruler(); }
   const typename _super::container& get_container() const { return this->hidden().get_ruler(); }

   typename _super::reference operator[] (int n) { return get_container()[n]; }
   typename _super::const_reference operator[] (int n) const { return get_container()[n]; }
};

template <typename dir>
class node_container
   : public modified_container_impl< node_container<dir>,
                                     list( Hidden< valid_node_container<dir> >,
                                           Operation< BuildUnaryIt<operations::index2element> >) >,
     public GenericSet<node_container<dir>, int, operations::cmp> {
protected:
   ~node_container();
};

} // end namespace graph

template <typename Iterator, typename Accessor, typename Feature>
struct check_iterator_feature<graph::valid_node_iterator<Iterator, Accessor>, Feature>
   : check_iterator_feature<unary_predicate_selector<Iterator, Accessor>, Feature> {};

template <typename Iterator, typename Accessor>
struct check_iterator_feature<graph::valid_node_iterator<Iterator, Accessor>, indexed> : True {};

namespace graph {

template <bool _out_edges, template <typename> class MasqueradeLine, typename EntryRef>
class line_factory {
public:
   typedef EntryRef argument_type;
   typedef typename if_else<_out_edges, typename deref<EntryRef>::type::out_tree_type,
                                        typename deref<EntryRef>::type::in_tree_type >::type
      tree_type;
   typedef typename inherit_ref<MasqueradeLine<tree_type>, EntryRef>::type result_type;

   result_type operator() (argument_type e) const
   {
      return _do(e, bool2type<_out_edges>());
   }
private:
   result_type _do(argument_type e, True) const
   {
      return reinterpret_cast<result_type>(e.out());
   }
   result_type _do(argument_type e, False) const
   {
      return reinterpret_cast<result_type>(e.in());
   }
};

template <bool _out_edges, template <typename> class MasqueradeLine>
class line_factory<_out_edges, MasqueradeLine, void> : operations::incomplete {};

} // end namespace graph

template <bool _out_edges, template <typename> class MasqueradeLine, typename Iterator, typename EntryRef>
struct unary_op_builder<graph::line_factory<_out_edges,MasqueradeLine,void>, Iterator, EntryRef>
   : empty_op_builder< graph::line_factory<_out_edges,MasqueradeLine,EntryRef> > {};

namespace graph {

template <typename dir, bool _out_edges, template <typename> class MasqueradeLine>
class line_container
   : public modified_container_impl< line_container<dir, _out_edges, MasqueradeLine>,
                                     list( Hidden< valid_node_container<dir> >,
                                           Operation< line_factory<_out_edges, MasqueradeLine> > ) > {
protected:
   ~line_container();
public:
   Table<dir>&       get_table()       { return this->hidden().hidden(); }
   const Table<dir>& get_table() const { return this->hidden().hidden(); }
   void resize(int n) { get_table().clear(n); }
};

template <typename dir, bool undirected=dir::value>
struct edge_container_helper {
   typedef line_container<dir, true, incident_edge_list> type;
};
template <typename dir>
struct edge_container_helper<dir, true> {
   typedef line_container<dir, true, lower_incident_edge_list> type;
};

template <typename dir>
class edge_container
   : public cascade_impl< edge_container<dir>,
                          list( Hidden< typename edge_container_helper<dir>::type >,
                                CascadeDepth< int2type<2> > ) > {
protected:
   ~edge_container();
public:
   int size() const { return this->hidden().get_table().edges(); }
   int max_size() const { return size(); }
};

template <typename dir>
template <bool for_copy> inline
void edge_agent<dir>::init(Table<dir>* t, bool2type<for_copy>)
{
   table=t;
   n_alloc=min_buckets(n_edges+bucket_mask>>bucket_shift);
   if (!for_copy) {
      // if the table was cloned, edge ids were copied too
      int id=0;
      for (typename edge_container<dir>::iterator e=reinterpret_cast<edge_container<dir>*>(t)->begin(); !e.at_end(); ++e, ++id)
         e.edge_id()=id;
   }
}

template <typename dir>
template <typename NumberConsumer> inline
void edge_agent<dir>::renumber(const NumberConsumer& nc)
{
   int id=0;
   for (typename edge_container<dir>::iterator e=reinterpret_cast<edge_container<dir>*>(table)->begin(); !e.at_end(); ++e, ++id) {
      nc(e.edge_id(), id);
      e.edge_id()=id;
   }
   assert(id == n_edges);
}

} // end namespace graph

template <typename Tree>
struct check_container_feature< graph::incident_edge_list<Tree>, sparse_compatible > : True {};

template <typename Tree>
struct check_container_feature< graph::lower_incident_edge_list<Tree>, sparse_compatible > : True {};

template <typename Tree>
struct check_container_feature< graph::multi_adjacency_line<Tree>, pure_sparse > : True {};

template <typename Tree>
struct spec_object_traits< graph::multi_adjacency_line<Tree> > :
   spec_object_traits<is_container> {
   static const bool is_always_const=true;
   typedef Tree masquerade_for;
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
template <typename _dir>
class Graph
   : public GenericGraph< Graph<_dir>, _dir > {
protected:
   typedef Table<_dir> table_type;
public:
   /// Create an empty Graph with 0 nodes.
   Graph() {}

   /// Create a Graph with @a n isolated nodes (without edges).
   explicit Graph(int n) :
      data( make_constructor(n, (table_type*)0) ) {}

   Graph(const GenericGraph<Graph>& G2) :
      data(G2.top().data) {}

   template <typename Graph2>
   Graph(const GenericGraph<Graph2, _dir>& G2) :
      data( make_constructor(G2.top().dim(), (table_type*)0) )
   {
      _copy(pm::nodes(G2).begin(), False(), False(), G2.top().has_gaps());
   }

   template <typename Graph2, typename dir2>
   explicit Graph(const GenericGraph<Graph2, dir2>& G2) :
      data( make_constructor(G2.top().dim(), (table_type*)0) )
   {
      const bool need_merge= Graph2::is_directed && !Graph::is_directed,
           need_contraction= Graph2::is_multigraph && !Graph::is_multigraph;
      _copy(pm::nodes(G2).begin(), bool2type<need_merge>(), bool2type<need_contraction>(), G2.top().has_gaps());
   }

   template <typename Matrix>
   explicit Graph(const GenericIncidenceMatrix<Matrix>& m,
                  typename enable_if<type2type<Matrix>**, !_dir::multigraph>::type=0) :
      data( make_constructor(m.rows(), (table_type*)0) )
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (!Matrix::is_symmetric && m.rows() != m.cols())
            throw std::runtime_error("Graph - non-quadratic source adjacency matrix");
      }
      const bool need_merge= !Matrix::is_symmetric && !Graph::is_directed;
      _copy(rows(m).begin(), bool2type<need_merge>());
   }

   // construct graph with gaps
   template <typename Set>
   explicit Graph(const GenericSet<Set,int>& s) :
      data( make_constructor(s.top(), (table_type*)0) ) {}

   template <typename Set>
   Graph(const GenericSet<Set,int>& s, int dim) :
      data( make_constructor(s.top(), dim, (table_type*)0) ) {}

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
   template <typename Graph2, typename dir2>
   Graph& operator= (const GenericGraph<Graph2, dir2>& G2)
   {
      clear(G2.top().dim());
      const bool need_merge= Graph2::is_directed && !Graph::is_directed,
           need_contraction= Graph2::is_multigraph && !Graph::is_multigraph;
      _copy(pm::nodes(G2).begin(), bool2type<need_merge>(), bool2type<need_contraction>(), G2.top().has_gaps());
      return *this;
   }

   /// number of nodes
   int nodes() const { return data->nodes(); }

   /// resize the Graph to given number of nodes
   void resize(int n) { data->resize(n); }

   /// clear all edges and resize (to zero nodes by default)
   void clear(int n=0) { data.apply(typename table_type::shared_clear(n)); }

   /// true of nodes are not (known to be) consecutively ordered
   bool has_gaps() const { return data->free_node_id!=std::numeric_limits<int>::min(); }

   /// renumber the nodes
   friend Graph renumber_nodes(const Graph& me)
   {
      if (!me.has_gaps()) return me;
      Graph G(me.nodes());
      std::vector<int> renumber(me.dim());
      int i=0;
      for (typename Entire< Nodes<Graph> >::const_iterator n=entire(pm::nodes(me)); !n.at_end(); ++n, ++i)
         renumber[n.index()]=i;
      for (typename Entire< Edges<Graph> >::const_iterator e=entire(pm::edges(me)); !e.at_end(); ++e)
         G.edge(renumber[e.from_node()], renumber[e.to_node()]);
      return G;
   }

   /// "output dimension"; relevant for proper output in the polymake shell
   int dim() const { return data->dim(); }

   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& in, Graph& me)
   {
      me.read(in.top(), in.top().begin_list(&rows(pm::adjacency_matrix(me))));
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

   /// permute the nodes as specified by an iterator
   template <typename Iterator>
   void permute_nodes(Iterator perm)
   {
      data->permute_nodes(perm, False());
   }

   /// inverse permutation of nodes
   template <typename Iterator>
   void permute_inv_nodes(Iterator perm)
   {
      data->permute_nodes(perm, True());
   }

   /// permuted copy
   template <typename Perm, typename InvPerm>
   Graph copy_permuted(const Perm& perm, const InvPerm& inv_perm) const
   {
      Graph result(dim());
      result.data->copy_permuted(*data, perm, inv_perm);
      return result;
   }

   /// node type
   typedef graph::node_container<_dir> node_container;
   /// node reference type
   typedef node_container& node_container_ref;
   /// constant node reference type
   typedef const node_container& const_node_container_ref;

   template <template <typename> class MasqueradeLine>
   struct edge_access {
      typedef line_container<_dir, true, MasqueradeLine> out;
      typedef line_container<_dir, false, MasqueradeLine> in;
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

   typedef typename if_else<_dir::multigraph, typename edge_access<multi_adjacency_line>::out, typename edge_access<incidence_line>::out>::type adjacency_rows_container;
   typedef typename assign_const<adjacency_rows_container, _dir::multigraph>::type& adjacency_rows_container_ref;
   typedef const adjacency_rows_container& const_adjacency_rows_container_ref;

   typedef typename if_else<_dir::multigraph, typename edge_access<multi_adjacency_line>::in, typename edge_access<incidence_line>::in>::type adjacency_cols_container;
   typedef typename assign_const<adjacency_cols_container, _dir::multigraph>::type& adjacency_cols_container_ref;
   typedef const adjacency_cols_container& const_adjacency_cols_container_ref;

   typedef typename adjacency_rows_container::value_type out_adjacent_node_list;
   typedef typename adjacency_cols_container::value_type in_adjacent_node_list;
   typedef typename if_else<Graph::is_directed, nothing, out_adjacent_node_list>::type adjacent_node_list;
   typedef typename assign_const<out_adjacent_node_list, _dir::multigraph>::type& out_adjacent_node_list_ref;
   typedef const out_adjacent_node_list& const_out_adjacent_node_list_ref;
   typedef typename assign_const<in_adjacent_node_list, _dir::multigraph>::type& in_adjacent_node_list_ref;
   typedef const in_adjacent_node_list& const_in_adjacent_node_list_ref;
   typedef typename if_else<Graph::is_directed, nothing, out_adjacent_node_list_ref>::type adjacent_node_list_ref;
   typedef typename if_else<Graph::is_directed, nothing, const_out_adjacent_node_list_ref>::type const_adjacent_node_list_ref;

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
   int _add_edge(int n1, int n2, False) { return edge(n1, n2); }
   int _add_edge(int n1, int n2, True) { return (*data)[n1].out().insert_new(n2)->data; }
public:
   /// Create a new edge between two given nodes and return its number.
   /// In a multigraph, a new edge is always created; the exact position of the new edge among its parallel twins can't be predicted.
   /// In a normal graph, a new edge is only created if the nodes were not adjacent before.
   int add_edge(int n1, int n2)
   {
      return _add_edge(n1, n2, bool2type<Graph::is_multigraph>());
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
      for (typename Tree::iterator it=tree_from.begin(); !it.at_end(); ) {
         typename Tree::Node* c=it.operator->();  ++it;
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
            c->key += node_to - node_from;
            if (tree_to.insert_node(c)) {
               (*data)[c->key - node_to].cross_tree(&tree_from).update_node(c);
            } else {
               c->key -= node_to - node_from;
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
      return out_adjacent_nodes(n, bool2type<_dir::multigraph>());
   }
   /// constant reference to list of nodes which are adjacent via out-arcs
   const_out_adjacent_node_list_ref out_adjacent_nodes(int n) const
   {
      return out_adjacent_nodes(n, bool2type<_dir::multigraph>());
   }
   /// reference to list of nodes which are adjacent via in-arcs
   in_adjacent_node_list_ref in_adjacent_nodes(int n)
   {
      return in_adjacent_nodes(n, bool2type<_dir::multigraph>());
   }
   /// constant reference to list of nodes which are adjacent via in-arcs
   const_in_adjacent_node_list_ref in_adjacent_nodes(int n) const
   {
      return in_adjacent_nodes(n, bool2type<_dir::multigraph>());
   }
   /// reference to list of all adjacent nodes
   adjacent_node_list_ref adjacent_nodes(int n)
   {
      typedef typename disable_if<adjacent_node_list, Graph::is_directed>::type error_if_undefined __attribute__((unused));
      return out_adjacent_nodes(n, bool2type<_dir::multigraph>());
   }
   /// constant reference to list of all adjacent nodes
   const_adjacent_node_list_ref adjacent_nodes(int n) const
   {
      typedef typename disable_if<adjacent_node_list, Graph::is_directed>::type error_if_undefined __attribute__((unused));
      return out_adjacent_nodes(n, bool2type<_dir::multigraph>());
   }

private:
   out_adjacent_node_list_ref out_adjacent_nodes(int n, False)
   {
      return this->template out_edges<incidence_line>(n);
   }
   const_out_adjacent_node_list_ref out_adjacent_nodes(int n, False) const
   {
      return this->template out_edges<incidence_line>(n);
   }
   in_adjacent_node_list_ref in_adjacent_nodes(int n, False)
   {
      return this->template in_edges<incidence_line>(n);
   }
   const_in_adjacent_node_list_ref in_adjacent_nodes(int n, False) const
   {
      return this->template in_edges<incidence_line>(n);
   }

   const_out_adjacent_node_list_ref out_adjacent_nodes(int n, True) const
   {
      return this->template out_edges<multi_adjacency_line>(n);
   }
   const_in_adjacent_node_list_ref in_adjacent_nodes(int n, True) const
   {
      return this->template in_edges<multi_adjacency_line>(n);
   }

public:
   template <typename E, typename Params=void>
   struct NodeMapData : public NodeMapBase {
      typedef typename extract_type_param<Params, DefaultValue, operations::clear<E> >::type default_value_supplier;

      E *data;
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
         for (typename Entire<node_container>::const_iterator it=entire(get_index_container()); !it.at_end(); ++it)
            new(data+*it) E(dflt());
      }

      void init(typename function_argument<E>::type val)
      {
         for (typename Entire<node_container>::const_iterator it=entire(get_index_container()); !it.at_end(); ++it)
            new(data+*it) E(val);
      }

      template <typename Iterator>
      void init(Iterator src)
      {
         for (typename Entire<node_container>::const_iterator it=entire(get_index_container()); !it.at_end(); ++it, ++src)
            new(data+*it) E(*src);
      }

      void copy(const NodeMapData& m)
      {
         dflt=m.dflt;
         typename node_container::const_iterator src=m.get_index_container().begin();
         for (typename Entire<node_container>::const_iterator it=entire(get_index_container()); !it.at_end(); ++it, ++src)
            new(data+*it) E(m.data[*src]);
      }

      void reset(int n=0)
      {
         if (!has_trivial_destructor<E>::value)
            for (typename Entire<node_container>::const_iterator it=entire(get_index_container()); !it.at_end(); ++it)
               std::_Destroy(data+*it);
         if (n) {
            if (size_t(n) != n_alloc) {
               _allocator.deallocate(data, n_alloc);
               alloc(n);
            }
         } else {
            _allocator.deallocate(data, n_alloc);
            data=NULL;  n_alloc=0;
         }
      }

      void clear(int n=0)
      {
         for (typename Entire<node_container>::const_iterator it=entire(get_index_container()); !it.at_end(); ++it)
            dflt.assign(data[*it]);
      }

      void resize(size_t new_n_alloc, int n, int nnew)
      {
         if (n_alloc < new_n_alloc) {
            E *new_data=_allocator.allocate(new_n_alloc), *src=data, *dst=new_data;
            for (E *end=new_data+std::min(n,nnew); dst<end; ++src, ++dst)
               relocate(src, dst);
            if (nnew>n)
               for (E *end=new_data+nnew; dst<end; ++dst)
                  new(dst) E(dflt());
            else
               std::_Destroy(src, data+n);

            if (data) _allocator.deallocate(data,n_alloc);
            data=new_data;  n_alloc=new_n_alloc;

         } else if (nnew>n)
            for (E *d=data+n, *end=data+nnew; d<end; ++d)
               new(d) E(dflt());
         else
            std::_Destroy(data+nnew, data+n);
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

      void move_entry(int n, int nnew) { relocate(data+n, data+nnew); }
      void revive_entry(int n) { new(data+n) E(dflt()); }
      void delete_entry(int n) { if (!has_trivial_destructor<E>::value) std::_Destroy(data+n); }

      typedef E value_type;

      NodeMapData() {}
      explicit NodeMapData(const default_value_supplier& dflt_arg) : data(0), n_alloc(0), dflt(dflt_arg) {}
      ~NodeMapData() { if (_table) { reset(); table().detach(*this); } }

      typedef const_node_container_ref index_container_ref;
      index_container_ref get_index_container() const { return reinterpret_cast<index_container_ref>(ctable()); }
   };

   template <typename E, typename Params=void>
   struct EdgeMapData : public EdgeMapDenseBase {
      typedef typename extract_type_param<Params, DefaultValue, operations::clear<E> >::type default_value_supplier;

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
         for (typename Entire< edge_container<_dir> >::const_iterator it=entire(get_index_container()); !it.at_end(); ++it)
            new(index2addr(*it)) E(dflt());
      }

      void init(typename function_argument<E>::type val)
      {
         for (typename Entire< edge_container<_dir> >::const_iterator it=entire(get_index_container()); !it.at_end(); ++it)
            new(index2addr(*it)) E(val);
      }

      template <typename Iterator>
      void init(Iterator src)
      {
         for (typename Entire< edge_container<_dir> >::const_iterator it=entire(get_index_container()); !it.at_end(); ++it, ++src)
            new(index2addr(*it)) E(*src);
      }

      void copy(const EdgeMapData& m)
      {
         dflt=m.dflt;
         typename edge_container<_dir>::const_iterator src=m.get_index_container().begin();
         for (typename Entire< edge_container<_dir> >::const_iterator it=entire(get_index_container()); !it.at_end(); ++it, ++src)
            new(index2addr(*it)) E(*m.index2addr(*src));
      }

      void reset()
      {
         if (!has_trivial_destructor<E>::value)
            for (typename Entire< edge_container<_dir> >::const_iterator it=entire(get_index_container()); !it.at_end(); ++it)
               std::_Destroy(index2addr(*it));
         for (E **b=reinterpret_cast<E**>(buckets), **b_end=b+n_alloc; b<b_end; ++b)
            if (*b) _allocator.deallocate(*b,edge_agent_base::bucket_size);
         EdgeMapDenseBase::destroy();
      }

      bool is_detachable() const
      {
         return has_trivial_destructor<E>::value;
      }

      void clear()
      {
         if (!is_pod<E>::value) {
            operations::clear<E> clr;
            for (typename Entire< edge_container<_dir> >::const_iterator it=entire(get_index_container()); !it.at_end(); ++it)
               clr(*index2addr(*it));
         }
      }

      void revive_entry(int e) { new(index2addr(e)) E(dflt()); }
      void delete_entry(int e) { if (!has_trivial_destructor<E>::value) std::_Destroy(index2addr(e)); }

      void add_bucket(int n)
      {
         E *d=_allocator.allocate(edge_agent_base::bucket_size);
         new(d) E(dflt());
         buckets[n]=d;
      }

      typedef E value_type;

      EdgeMapData() {}
      explicit EdgeMapData(const default_value_supplier& dflt_arg) : dflt(dflt_arg) {}
      ~EdgeMapData() { if (_table) { reset(); table().detach(*this); } }

      typedef const edge_container<_dir>& index_container_ref;
      index_container_ref get_index_container() const { return reinterpret_cast<index_container_ref>(ctable()); }
   };

   template <typename E, typename Params>
   struct NodeHashMapData : public NodeMapBase {
      typedef typename extract_type_param<Params, DefaultValue, operations::clear<E> >::type default_value_supplier;

      hash_map<int, E, Params> data;

      void init() {}
      void reset(int=0) { data.clear(); }
      void clear() { data.clear(); }
      void shrink(size_t,int) {}
      void revive_entry(int) {}

      void resize(size_t, int n, int nnew)
      {
         while (n>nnew) data.erase(--n);
      }
      void move_entry(int n, int nnew)
      {
         typename hash_map<int,E>::iterator it=data.find(n);
         if (it != data.end()) {
            data.insert(nnew,it->second);
            data.erase(it);
         }
      }
      void delete_entry(int n) { data.erase(n); }

      void copy(const NodeHashMapData& m) { data=m.data; }

      table_type& table() { return *reinterpret_cast<table_type*>(_table); }
      const table_type& ctable() const { return *reinterpret_cast<const table_type*>(_table); }

      NodeHashMapData() {}
      NodeHashMapData(const default_value_supplier& dflt_arg) : data(dflt_arg) {}
      ~NodeHashMapData() { if (_table) table().detach(*this); }
   };

   template <typename E, typename Params>
   struct EdgeHashMapData : public EdgeMapBase {
      typedef typename extract_type_param<Params, DefaultValue, operations::clear<E> >::type default_value_supplier;

      hash_map<int, E, Params> data;

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
#if !(defined(__clang__) && ((!defined(__APPLE__) && __clang_major__ == 3 && __clang_minor__ >= 8) || ( defined(__APPLE__) && __clang_major__ == 7 && __clang_minor__ >= 3)))
   template <typename BaseMap> class SharedMap;
#endif

protected:
   template <typename E, typename Params, bool for_copy> static
   void prepare_attach(const table_type& t, NodeMapData<E,Params>& m, bool2type<for_copy>)
   {
      m.alloc(t.get_ruler().max_size());
      t.attach(m);
   }

   template <typename E, typename Params, bool for_copy> static
   void prepare_attach(const table_type& t, EdgeMapData<E,Params>& m, bool2type<for_copy> C)
   {
      m.alloc(t.get_edge_agent(C));
      t.attach(m);
   }

   template <typename E, typename Params, bool for_copy> static
   void prepare_attach(const table_type& t, NodeHashMapData<E,Params>& m, bool2type<for_copy>)
   {
      t.attach(m);
   }

   template <typename E, typename Params, bool for_copy> static
   void prepare_attach(const table_type& t, EdgeHashMapData<E,Params>& m, bool2type<for_copy> C)
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
      friend class Graph<_dir>;
   };

   class divorce_maps : public shared_alias_handler {
   public:
      template <typename Rep>
      Rep* operator() (Rep *body, True) const
      {
         if (al_set.n_aliases)
            for (AliasSet::iterator it=al_set.begin(); !it.at_end(); ++it)
               static_cast<map2graph_connector*>((*it)->to_handler())->divorce(body->obj);
         return body;
      }

      template <typename Rep>
      Rep* operator() (Rep *body, False)
      {
         if (al_set.n_aliases) al_set.forget();
         return body;
      }

      friend class Graph<_dir>;
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
         prepare_attach(t,*cp,True());
         cp->copy(*map);
         return cp;
      }

      void divorce();
      void divorce(const table_type& t);
      void leave() { if (--map->refc == 0) delete map; }
      void mutable_access() { if (__builtin_expect(map->refc>1, 0)) divorce(); }

      template <bool may_need_detach>
      void attach_to(const Graph& G, bool2type<may_need_detach>)
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
         prepare_attach(*G.data,*map,False());
         this->al_set.enter(G.data.get_divorce_handler().al_set);
      }

      SharedMap() : map(0) {}

      explicit SharedMap(const default_value_supplier& dflt_arg) : map(0), dflt(dflt_arg) {}

      explicit SharedMap(const Graph& G) { attach_to(G,False()); }

      SharedMap(const Graph& G, const default_value_supplier& dflt_arg) : dflt(dflt_arg) { attach_to(G,False()); }

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

      friend class Graph<_dir>;

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
      m.attach_to(*this,True());
      m.map->init();
   }

   template <typename Map>
   void attach(SharedMap<Map>& m, typename function_argument<typename Map::value_type>::type val) const
   {
      m.attach_to(*this,True());
      m.map->init(val);
   }

   template <typename Map, typename Iterator>
   void attach(SharedMap<Map>& m, Iterator src) const
   {
      m.attach_to(*this,True());
      m.map->init(src);
   }

protected:
   template <typename Container>
   static void prepare_attach_static(Container& c, int, io_test::as_set)
   {
      unwary(c).clear();
   }
   template <typename Container, bool _allow_sparse>
   static void prepare_attach_static(Container& c, int n, io_test::as_array<1,_allow_sparse>)
   {
      unwary(c).resize(n);
   }
   template <typename Container, bool _allow_sparse>
   static void prepare_attach_static(Container& c, int n, io_test::as_array<0,_allow_sparse>)
   {
      if (POLYMAKE_DEBUG || !Unwary<Container>::value) {
         if (get_dim(unwary(c)) != n)
            throw std::runtime_error("Graph::init_map - dimension mismatch");
      }
   }
public:
   template <typename Container>
   void init_node_map(Container& c) const
   {
      prepare_attach_static(c, this->nodes(), typename io_test::input_mode<typename Unwary<Container>::type>::type());
   }

   template <typename Container>
   void init_edge_map(Container& c) const
   {
      prepare_attach_static(c, data->get_edge_agent(False()).n_edges, typename io_test::input_mode<typename Unwary<Container>::type>::type());
   }

   /// Provide each edge with a unique integer id.
   /// The ids are assigned in the order of visiting them by Edges<Graph>::iterator,
   /// but you shouldn't rely on any special order later, after having added or deleted
   /// some edges, because the enumeration is effectively performed only once in the life
   /// of a Graph object.  Later calls to this function are no-ops.
   /// Moreover, the edges are also enumerated when an edge attribute map is attached
   /// to the Graph object for the first time.
   void enumerate_edges() const { data->get_edge_agent(False()); }

#if POLYMAKE_DEBUG
   void check(const char *prefix="") const { data->check(prefix); }
   void dump() const { cerr << adjacency_matrix(*this) << std::flush; }
   void dump_edges() const;
   void dumps() const { dump(); dump_edges(); }
   ~Graph() { POLYMAKE_DEBUG_METHOD(Graph,dumps); }
#endif

protected:
   typedef shared_object<table_type, cons< AliasHandler<shared_alias_handler>, DivorceHandler<divorce_maps> > > shared_type;
   shared_type data;

   friend Graph& make_mutable_alias(Graph& alias, Graph& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }

   template <typename Input, typename Cursor>
   void read(Input& in, Cursor src)
   {
      if (src.sparse_representation()) {
         int n=0, end=src.lookup_dim(false);
         clear(end);
         for (typename Entire<out_edge_list_container>::iterator l=entire(out_edge_lists(*this));  !src.at_end();  ++l, ++n) {
            const int i=src.index();
            while (n<i) ++l, data.get()->delete_node(n++);
            src >> *l;
         }
         while (n<end) data.get()->delete_node(n++);
      } else {
         clear(src.size());
         for (typename Entire<out_edge_list_container>::iterator l=entire(out_edge_lists(*this));  !src.at_end();  ++l)
            src >> *l;
      }
   }

   template <typename NodeIterator, typename _need_merge, typename _need_contraction>
   void _copy(NodeIterator src, _need_merge, _need_contraction, bool has_gaps)
   {
      if (has_gaps) {
         int n=0, end=dim();
         for (typename Entire<out_edge_list_container>::iterator l=entire(out_edge_lists(*this));  !src.at_end();  ++l, ++src, ++n) {
            const int i=src.index();
            while (n<i) ++l, data.get()->delete_node(n++);
            l->init_from_edge_list(src.out_edges().begin(), _need_merge(), _need_contraction());
         }
         while (n<end) data.get()->delete_node(n++);
      } else {
         for (typename Entire<out_edge_list_container>::iterator l=entire(out_edge_lists(*this));  !l.at_end();  ++l, ++src)
            l->init_from_edge_list(src.out_edges().begin(), _need_merge(), _need_contraction());
      }
   }

   template <typename RowIterator, typename _need_merge>
   void _copy(RowIterator src, _need_merge)
   {
      for (typename Entire<out_edge_list_container>::iterator l=entire(out_edge_lists(*this));  !l.at_end();  ++l, ++src)
         l->init_from_set(src->begin(), _need_merge());
   }

   template <typename,typename,typename> friend class NodeMap;
   template <typename,typename,typename> friend class EdgeMap;
   template <typename,typename,typename> friend class NodeHashMap;
   template <typename,typename,typename> friend class EdgeHashMap;

   const Graph& get_graph() const { return *this; }     // for HashMaps wanting to attach
};

#if POLYMAKE_DEBUG
template <typename dir>
void Graph<dir>::dump_edges() const
{
   for (typename Entire< edge_container<dir> >::const_iterator e=entire(pretend<const edge_container<dir>&>()); !e.at_end(); ++e)
      cerr << *e << ':' << e.from_node() << '-' << e.to_node() << '\n';
   cerr << std::flush;
}
#endif


/// data structure to store data at the nodes of a Graph
template <typename dir, typename E, typename Params>
class NodeMap : public Graph<dir>::template SharedMap<typename Graph<dir>::template NodeMapData<E,Params> >,
                public modified_container_impl< NodeMap<dir,E,Params>,
                                                list( Container< typename Graph<dir>::const_node_container_ref >,
                                                      Operation< operations::random_access<E*> > ) > {
   typedef modified_container_impl<NodeMap> _super;
   typedef typename Graph<dir>::template SharedMap<typename Graph<dir>::template NodeMapData<E,Params> > shared_base;
public:
   typedef typename shared_base::default_value_supplier default_value_supplier;
   typedef Graph<dir> graph_type;
   typedef random_access_iterator_tag container_category;

   NodeMap() {}

   explicit NodeMap(const default_value_supplier& dflt_arg) : shared_base(dflt_arg) {}

   explicit NodeMap(const graph_type& G) : shared_base(G)
   {
      this->map->init();
   }

   NodeMap(const graph_type& G, const default_value_supplier& dflt_arg) : shared_base(G, dflt_arg)
   {
      this->map->init();
   }

   NodeMap(const graph_type& G, typename function_argument<E>::type val) : shared_base(G)
   {
      this->map->init(val);
   }

   NodeMap(const graph_type& G, typename function_argument<E>::type val,
           const default_value_supplier& dflt_arg) : shared_base(G, dflt_arg)
   {
      this->map->init(val);
   }

   template <typename Iterator>
   NodeMap(const graph_type& G, Iterator src) : shared_base(G)
   {
      this->map->init(src);
   }

   template <typename Iterator>
   NodeMap(const graph_type& G, Iterator src,
           const default_value_supplier& dflt_arg) : shared_base(G, dflt_arg)
   {
      this->map->init(src);
   }

   typename shared_base::map_type::index_container_ref get_container()
   {
      this->mutable_access();
      return this->map->get_index_container();
   }
   typename shared_base::map_type::index_container_ref get_container() const
   {
      return this->map->get_index_container();
   }

   typename _super::operation get_operation() { return this->map->data; }
   typename _super::const_operation get_operation() const { return this->map->data; }

   friend int index_within_range(const NodeMap& me, int n)
   {
      if (n<0) n+=me.map->ctable().dim();
      if (me.invalid_node(n))
         throw std::runtime_error("NodeMap::operator[] - node id out of range or deleted");
      return n;
   }

   E& operator[] (int n)
   {
      this->mutable_access();
      return this->map->data[n];
   }

   const E& operator[] (int n) const
   {
      return this->map->data[n];
   }

   friend class Graph<dir>;

   friend void relocate(NodeMap* from, NodeMap* to)
   {
      relocate(static_cast<shared_base*>(from), static_cast<shared_base*>(to));
   }

#if POLYMAKE_DEBUG
   void dump(False) const { cerr << "elements are not printable" << std::flush; }
   void dump(True) const { cerr << *this << std::flush; }
   void dump() const { dump(bool2type<is_printable<E>::value>()); }
   ~NodeMap() { POLYMAKE_DEBUG_METHOD(NodeMap,dump); }
#endif
};

/// data structure to store data at the edges of a Graph
template <typename dir, typename E, typename Params>
class EdgeMap : public Graph<dir>::template SharedMap<typename Graph<dir>::template EdgeMapData<E,Params> >,
                public modified_container_impl< EdgeMap<dir,E,Params>,
                                                list( Container< const edge_container<dir>& >,
                                                      Operation< EdgeMapDataAccess<E> > ) > {
   typedef modified_container_impl<EdgeMap> _super;
   typedef typename Graph<dir>::template SharedMap<typename Graph<dir>::template EdgeMapData<E,Params> > shared_base;
public:
   typedef typename shared_base::default_value_supplier default_value_supplier;
   typedef Graph<dir> graph_type;
   typedef random_access_iterator_tag container_category;

   EdgeMap() {}

   explicit EdgeMap(const default_value_supplier& dflt_arg) : shared_base(dflt_arg) {}

   explicit EdgeMap(const graph_type& G) : shared_base(G)
   {
      this->map->init();
   }

   EdgeMap(const graph_type& G, const default_value_supplier& dflt_arg) : shared_base(G, dflt_arg)
   {
      this->map->init();
   }

   EdgeMap(const graph_type& G, typename function_argument<E>::type val) : shared_base(G)
   {
      this->map->init(val);
   }

   EdgeMap(const graph_type& G, typename function_argument<E>::type val,
           const default_value_supplier& dflt_arg) : shared_base(G, dflt_arg)
   {
      this->map->init(val);
   }

   template <typename Iterator>
   EdgeMap(const graph_type& G, Iterator src) : shared_base(G)
   {
      this->map->init(src);
   }

   template <typename Iterator>
   EdgeMap(const graph_type& G, Iterator src,
           const default_value_supplier& dflt_arg) : shared_base(G, dflt_arg)
   {
      this->map->init(src);
   }

   typename shared_base::map_type::index_container_ref get_container()
   {
      this->mutable_access();
      return this->map->get_index_container();
   }
   typename shared_base::map_type::index_container_ref get_container() const
   {
      return this->map->get_index_container();
   }

   EdgeMapDataAccess<E> get_operation() { return this->map->buckets; }
   EdgeMapDataAccess<const E> get_operation() const { return this->map->buckets; }

   E& operator[] (int e) { this->mutable_access(); return *this->map->index2addr(e); }
   const E& operator[] (int e) const { return *this->map->index2addr(e); }

   E& operator() (int n1, int n2)
   {
      this->mutable_access();
      typename graph_type::node_acc n;
      return *this->map->index2addr(n.out_edge(this->map->table()[n1], n2));
   }

   const E& operator() (int n1, int n2) const
   {
      typename graph_type::const_node_acc n;
      return *this->map->index2addr(n.out_edge(this->map->table()[n1], n2));
   }

   friend class Graph<dir>;

   friend void relocate(EdgeMap* from, EdgeMap* to)
   {
      relocate(static_cast<shared_base*>(from), static_cast<shared_base*>(to));
   }

#if POLYMAKE_DEBUG
   void dump(False) const { cerr << "elements are not printable" << std::flush; }
   void dump(True) const { cerr << *this << std::flush; }
   void dump() const { dump(bool2type<is_printable<E>::value>()); }
   ~EdgeMap() { POLYMAKE_DEBUG_METHOD(EdgeMap,dump); }
#endif
};

template <typename dir, typename E, typename Params>
class NodeHashMap : public Graph<dir>::template SharedMap<typename Graph<dir>::template NodeHashMapData<E,Params> >,
                    public redirected_container< NodeHashMap<dir,E,Params>, Container< hash_map<int,E,Params> > > {
   typedef redirected_container<NodeHashMap> _super;
   typedef typename Graph<dir>::template SharedMap<typename Graph<dir>::template NodeHashMapData<E,Params> > shared_base;
public:
   typedef typename shared_base::default_value_supplier default_value_supplier;
   typedef int key_type;
   typedef E mapped_type;

   NodeHashMap() {}

   explicit NodeHashMap(const default_value_supplier& dflt_arg) : shared_base(dflt_arg) {}

   template <typename Graph2>
   explicit NodeHashMap(const GenericGraph<Graph2,dir>& G) : shared_base(G.top().get_graph()) {}

   template <typename Graph2>
   NodeHashMap(const GenericGraph<Graph2,dir>& G, const default_value_supplier& dflt_arg) : shared_base(G.top().get_graph(), dflt_arg) {}

   hash_map<int,E>& get_container()
   {
      this->mutable_access();
      return this->map->data;
   }
   const hash_map<int,E>& get_container() const
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
      typename _super::const_iterator it=find(n);
      if (it==get_container().end()) throw no_match();
      return it->second;
   }

   typename _super::iterator insert(int n, typename function_argument<E>::type v)
   {
      if (POLYMAKE_DEBUG) {
         if (this->invalid_node(n))
            throw std::runtime_error("NodeHashMap::insert - node index out of range or deleted");
      }
      return get_container().insert(n,v);
   }

   pair<typename _super::iterator, bool> insert(const typename _super::value_type& v)
   {
      if (POLYMAKE_DEBUG) {
         if (this->invalid_node(v.first))
            throw std::runtime_error("NodeHashMap::insert - node index out of range or deleted");
      }
      return get_container().insert(v);
   }

   typename _super::const_iterator find(int n) const { return get_container().find(n); }
   void erase(int n) { return get_container().erase(n); }
   typename _super::iterator erase(typename _super::iterator where) { return get_container().erase(where); }

   friend class Graph<dir>;

   friend void relocate(NodeHashMap* from, NodeHashMap* to)
   {
      relocate(static_cast<shared_base*>(from), static_cast<shared_base*>(to));
   }

#if POLYMAKE_DEBUG
   void dump(False) const { cerr << "elements are not printable" << std::flush; }
   void dump(True) const { cerr << *this << std::flush; }
   void dump() const { dump(bool2type<is_printable<E>::value>()); }
   ~NodeHashMap() { POLYMAKE_DEBUG_METHOD(NodeHashMap,dump); }
#endif
};

template <typename dir, typename E, typename Params>
class EdgeHashMap : public Graph<dir>::template SharedMap<typename Graph<dir>::template EdgeHashMapData<E,Params> >,
                    public redirected_container< EdgeHashMap<dir,E,Params>, Container< hash_map<int,E,Params> > > {
   typedef redirected_container<EdgeHashMap> _super;
   typedef typename Graph<dir>::template SharedMap<typename Graph<dir>::template EdgeHashMapData<E,Params> > shared_base;
public:
   typedef typename shared_base::default_value_supplier default_value_supplier;
   typedef int key_type;
   typedef E mapped_type;

   EdgeHashMap() {}

   explicit EdgeHashMap(const default_value_supplier& dflt_arg) : shared_base(dflt_arg) {}

   template <typename Graph2>
   explicit EdgeHashMap(const GenericGraph<Graph2,dir>& G) : shared_base(G.top().get_graph()) {}

   template <typename Graph2>
   EdgeHashMap(const GenericGraph<Graph2,dir>& G, const default_value_supplier& dflt_arg) : shared_base(G.top().get_graph(), dflt_arg) {}

   hash_map<int,E>& get_container()
   {
      this->mutable_access();
      return this->map->data;
   }
   const hash_map<int,E>& get_container() const
   {
      return this->map->data;
   }

   E& operator[] (int e) { return get_container()[e]; }

   const E& operator[] (int e) const
   {
      typename _super::const_iterator it=find(e);
      if (it==get_container().end()) throw no_match();
      return it->second;
   }

   E& operator() (int n1, int n2)
   {
      typename Graph<dir>::node_acc n;
      return get_container()[n.out_edge(this->map->table()[n1], n2)];
   }

   typename _super::iterator insert(int e, typename function_argument<E>::type v)
   {
      return get_container().insert(e,v);
   }

   pair<typename _super::iterator, bool> insert(const typename _super::value_type& v)
   {
      return get_container().insert(v);
   }

   typename _super::const_iterator find(int e) const { return get_container().find(e); }

   typename _super::const_iterator find(int n1, int n2) const
   {
      typename Graph<dir>::out_edge_list::const_iterator e=this->map->table()[n1].out().find(n2);
      return e.at_end() ? get_container().end() : find(*e);
   }

   void erase(int e) { return get_container().erase(e); }

   typename _super::iterator erase(typename _super::iterator where) { return get_container().erase(where); }

   void erase(int n1, int n2)
   {
      this->mutable_access();   // an iterator into the tree becomes invalid after divorce()
      typename Graph<dir>::node_acc n;
      typename Graph<dir>::out_edge_list::iterator e=n.out_edges(this->map->table()[n1]).find(n2);
      if (!e.at_end()) this->map->data.erase(*e);
   }

   friend class Graph<dir>;

   friend void relocate(EdgeHashMap* from, EdgeHashMap* to)
   {
      relocate(static_cast<shared_base*>(from), static_cast<shared_base*>(to));
   }

#if POLYMAKE_DEBUG
   void dump(False) const { cerr << "elements are not printable" << std::flush; }
   void dump(True) const { cerr << *this << std::flush; }
   void dump() const { dump(bool2type<is_printable<E>::value>()); }
   ~EdgeHashMap() { POLYMAKE_DEBUG_METHOD(EdgeHashMap,dump); }
#endif
};

template <typename dir>
template <typename Map>
void Graph<dir>::SharedMap<Map>::divorce()
{
   map->refc--;
   map=copy(map->ctable());
}

template <typename dir>
template <typename Map>
void Graph<dir>::SharedMap<Map>::clear()
{
   if (__builtin_expect(map->refc>1,0)) {
      map->refc--;
      const table_type& t=map->ctable();
      map=new map_type(dflt);
      prepare_attach(t,*map,False());
   } else {
      map->clear();
   }
}

template <typename dir>
template <typename Map>
void Graph<dir>::SharedMap<Map>::divorce(const typename Graph<dir>::table_type& t)
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

template <typename dir, typename E, typename Params>
struct spec_object_traits< graph::NodeMap<dir,E,Params> > : spec_object_traits<is_container> {
   static const int is_resizeable=0;
};

template <typename dir, typename E, typename Params>
struct spec_object_traits< graph::EdgeMap<dir,E,Params> > : spec_object_traits<is_container> {
   static const int is_resizeable=0;
};

template <typename dir>
struct spec_object_traits< graph::Graph<dir> > : spec_object_traits<is_opaque> {
   static const int is_resizeable=1;
};

template <typename Graph, typename Permutation> inline
typename Graph::persistent_type
permuted_nodes(const GenericGraph<Graph>& g, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Graph>::value) {
      if (g.top().dim() != perm.size())
         throw std::runtime_error("permuted_nodes - dimension mismatch");
   }
   std::vector<int> inv_perm(g.nodes());
   inverse_permutation(perm,inv_perm);
   return g.top().copy_permuted(perm,inv_perm);
}

template <typename Graph, typename Permutation> inline
typename enable_if<typename Graph::persistent_type, container_traits<Permutation>::is_random>::type
permuted_inv_nodes(const GenericGraph<Graph>& g, const Permutation& inv_perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Graph>::value) {
      if (g.top().dim() != inv_perm.size())
         throw std::runtime_error("permuted_inv_nodes - dimension mismatch");
   }
   std::vector<int> perm(g.nodes());
   inverse_permutation(inv_perm,perm);
   return g.top().copy_permuted(perm,inv_perm);
}

template <typename Graph, typename Permutation> inline
typename disable_if<typename Graph::persistent_type, container_traits<Permutation>::is_random>::type
permuted_inv_nodes(const GenericGraph<Graph>& g, const Permutation& inv_perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Graph>::value) {
      if (g.top().dim() != inv_perm.size())
         throw std::runtime_error("permuted_inv_nodes - dimension mismatch");
   }
   std::vector<int> inv_perm_copy(inv_perm.size());
   copy(entire(inv_perm), inv_perm_copy.begin());
   return permuted_inv_rows(g,inv_perm_copy);
}

template <typename dir>
class Wary< graph::Graph<dir> > : public WaryGraph< graph::Graph<dir> > {
protected:
   Wary();
   ~Wary();
};

template <typename dir, typename E, typename Params>
class Wary< graph::EdgeMap<dir,E,Params> > : public Generic< Wary< graph::EdgeMap<dir,E,Params> > > {
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

template <typename dir, typename E, typename Params>
class Wary< graph::EdgeHashMap<dir,E,Params> > : public Generic< Wary< graph::EdgeHashMap<dir,E,Params> > > {
protected:
   Wary();
   ~Wary();
public:
   E& operator() (int n1, int n2)
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("EdgeHashMap::operator() - node id out of range or deleted");
      return this->top()(n1,n2);
   }
   typename graph::EdgeHashMap<dir,E,Params>::const_iterator find(int n1, int n2) const
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("EdgeHashMap::find - node id out of range or deleted");
      return this->top().find(n1,n2);
   }
   void erase(int n1, int n2)
   {
      if (this->top().invalid_node(n1) || this->top().invalid_node(n2))
         throw std::runtime_error("EdgeHashMap::erase - node id out of range or deleted");
      this->top().erase(n1,n2);
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
   template <typename dir> inline
   void swap(pm::graph::Graph<dir>& G1, pm::graph::Graph<dir>& G2) { G1.swap(G2); }
}

#endif // POLYMAKE_GRAPH_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
