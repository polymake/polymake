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

/** @file IncidenceMatrix.h
    @brief Implementation of pm::IncidenceMatrix class
*/


#ifndef POLYMAKE_INCIDENCE_MATRIX_H
#define POLYMAKE_INCIDENCE_MATRIX_H

#include "polymake/internal/sparse2d.h"
#include "polymake/Set.h"
#include "polymake/GenericIncidenceMatrix.h"
#include "polymake/permutations.h"

namespace pm {

template <typename Set>
class incidence_proxy_base {
protected:
   Set* s;
   int j;

   bool get() const { return s->exists(j); }

   void insert() { s->insert(j); }

   void erase() { s->erase(j); }

   void toggle() { s->toggle(j); }
public:
   typedef bool value_type;

   incidence_proxy_base(Set& s_arg, int j_arg)
      : s(&s_arg), j(j_arg) {}
};

template <typename TreeRef> class incidence_line;
template <bool rowwise, typename BaseRef=void> class incidence_line_factory;
template <typename symmetric> class IncidenceMatrix_base;

template <typename TreeRef>
struct incidence_line_params
   : concat_list< typename sparse2d::line_params<TreeRef>::type,
                  Operation< BuildUnaryIt<operations::index2element> > > {};

template <typename TreeRef>
class incidence_line_base
   : public modified_tree< incidence_line<TreeRef>, typename incidence_line_params<TreeRef>::type > {
protected:
   typedef nothing first_arg_type;
   typedef nothing second_arg_type;
   ~incidence_line_base();
public:
   int index() const { return this->get_container().get_line_index(); }
};

template <typename Tree>
class incidence_line_base<Tree&>
   : public modified_tree< incidence_line<Tree&>, typename incidence_line_params<Tree&>::type > {
protected:
   typedef typename deref<Tree>::type tree_type;
   typedef typename if_else<Tree::symmetric, Symmetric, NonSymmetric>::type symmetric;
   typedef typename inherit_ref<IncidenceMatrix_base<symmetric>, Tree&>::type matrix_ref;
   typedef typename attrib<matrix_ref>::plus_const const_matrix_ref;

   alias<matrix_ref> matrix;
   int line_index;

   typedef typename alias<matrix_ref>::arg_type first_arg_type;
   typedef int second_arg_type;

   incidence_line_base(first_arg_type arg1, second_arg_type arg2)
      : matrix(arg1), line_index(arg2)  {}
public:
   typename incidence_line_base::container& get_container()
   {
      return matrix->get_table().get_line(line_index, (tree_type*)0);
   }
   const typename incidence_line_base::container& get_container() const
   {
      return matrix->get_table().get_line(line_index, (tree_type*)0);
   }

   int index() const { return line_index; }
};

template <typename TreeRef>
class incidence_line
   : public incidence_line_base<TreeRef>,
     public GenericMutableSet<incidence_line<TreeRef>, int, operations::cmp>
{
   typedef incidence_line_base<TreeRef> _super;
public:
   incidence_line(typename _super::first_arg_type arg1, typename _super::second_arg_type arg2)
      : _super(arg1,arg2) {}

   incidence_line& operator= (const incidence_line& other)
   {
      return incidence_line::generic_mutable_type::operator=(other);
   }

   template <typename Set>
   incidence_line& operator= (const GenericSet<Set, int, operations::cmp>& other)
   {
      return incidence_line::generic_mutable_type::operator=(other);
   }

   template <typename Set>
   incidence_line& operator= (const Complement<Set, int, operations::cmp>& other)
   {
      return incidence_line::generic_mutable_type::operator=(sequence(0, this->dim()) * other);
   }

   template <typename Container>
   typename enable_if<incidence_line, isomorphic_to_container_of<Container, int, is_set>::value>::type&
   operator= (const Container& src)
   {
      this->clear();
      for (typename Entire<Container>::const_iterator i=entire(src); !i.at_end(); ++i)
         this->insert(*i);
      return *this;
   }
};

template <typename TreeRef>
struct check_container_feature<incidence_line<TreeRef>, sparse_compatible> : True {};

template <typename TreeRef>
struct spec_object_traits< incidence_line<TreeRef> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=attrib<TreeRef>::is_reference,
                     is_always_const=attrib<TreeRef>::is_const;
   typedef typename if_else<is_temporary, void, typename deref<TreeRef>::type>::type masquerade_for;
   static const int is_resizeable=0;
};

template <sparse2d::restriction_kind restriction=sparse2d::only_rows>
class RestrictedIncidenceMatrix
   : public matrix_methods<RestrictedIncidenceMatrix<restriction>, bool> {
protected:
   typedef sparse2d::Table<nothing, false, restriction> table_type;
   table_type data;

   table_type& get_table() { return data; }
   const table_type& get_table() const { return data; }

   template <typename Iterator>
   void _copy(Iterator src, True)
   {
      copy(src, entire(pm::rows(*this)));
   }
   template <typename Iterator>
   void _copy(Iterator src, False)
   {
      copy(src, entire(pm::cols(*this)));
   }

   typedef incidence_proxy_base< incidence_line<typename table_type::primary_tree_type> > proxy_base;

public:
   typedef bool value_type;
   typedef sparse_elem_proxy<proxy_base> reference;
   typedef const bool const_reference;

   RestrictedIncidenceMatrix() : data(nothing()) {}
   explicit RestrictedIncidenceMatrix(int n) : data(n) {}
   RestrictedIncidenceMatrix(int r, int c) : data(r,c) {}

   template <typename Iterator>
   RestrictedIncidenceMatrix(typename enable_if<int, isomorphic_types<typename iterator_traits<Iterator>::value_type, Set<int> >::value>::type n, Iterator src)
      : data(n)
   {
      _copy(src, bool2type<restriction==sparse2d::only_rows>());
   }

   template <typename Iterator>
   RestrictedIncidenceMatrix(typename enable_if<int, isomorphic_types<typename iterator_traits<Iterator>::value_type, Set<int> >::value>::type r, int c, Iterator src)
      : data(r,c)
   {
      _copy(src, bool2type<restriction==sparse2d::only_rows>());
   }

   template <typename Container>
   RestrictedIncidenceMatrix(const Container& src,
                             typename enable_if<void**, (isomorphic_to_container_of<Container, Set<int>, allow_conversion>::value &&
                                                         restriction==sparse2d::only_rows)>::type=0)
      : data(src.size())
   {
      _copy(src.begin(), True());
   }

   void swap(RestrictedIncidenceMatrix& M) { data.swap(M.data); }

   void clear() { data.clear(); }

protected:
   proxy_base _random(int i, int j, False)
   {
      return proxy_base(this->row(i), j);
   }
   proxy_base _random(int i, int j, True)
   {
      return proxy_base(this->col(j), i);
   }
   bool _random(int i, int j, False) const
   {
      return this->row(i).exists(j);
   }
   bool _random(int i, int j, True) const
   {
      return this->col(j).exists(i);
   }
public:
   reference operator() (int i, int j)
   {
      return _random(i, j, bool2type<restriction==sparse2d::only_cols>());
   }
   const_reference operator() (int i, int j) const
   {
      return _random(i, j, bool2type<restriction==sparse2d::only_cols>());
   }

   bool exists(int i, int j) const
   {
      return _random(i, j, bool2type<restriction==sparse2d::only_cols>());
   }

private:
   template <typename RowCol, typename Set>
   void _append(RowCol& row_col, const Set& set, int i)
   {
      for (typename Entire<Set>::const_iterator s=entire(set); !s.at_end(); ++s)
         row_col[*s].push_back(i);
   }

   template <typename Iterator>
   void _append_rows(int n, Iterator src, True)
   {
      int oldrows=data.rows();
      data.resize_rows(oldrows+n);
      copy(src, pm::rows(*this).begin()+oldrows);
   }

   template <typename Iterator>
   void _append_rows(int, Iterator src, False)
   {
      for (int r=data.rows(); !src.at_end(); ++src, ++r)
         _append(pm::cols(*this), *src, r);
   }

   template <typename Iterator>
   void _append_cols(int n, Iterator src, True)
   {
      int oldcols=data.cols();
      data.resize_cols(oldcols+n);
      copy(src, pm::cols(*this).begin()+oldcols);
   }

   template <typename Iterator>
   void _append_cols(int, Iterator src, False)
   {
      for (int c=data.cols(); !src.at_end(); ++src, ++c)
         _append(pm::rows(*this), *src, c);
   }

public:
   template <typename Matrix>
   RestrictedIncidenceMatrix& operator/= (const GenericIncidenceMatrix<Matrix>& m)
   {
      _append_rows(m.rows(), entire(pm::rows(m)), bool2type<restriction==sparse2d::only_rows>());
      return *this;
   }

   template <typename Set>
   RestrictedIncidenceMatrix& operator/= (const GenericSet<Set, int, operations::cmp>& s)
   {
      _append_rows(1, entire(item2container(s.top())), bool2type<restriction==sparse2d::only_rows>());
      return *this;
   }

   template <typename Matrix>
   RestrictedIncidenceMatrix& operator|= (const GenericIncidenceMatrix<Matrix>& m)
   {
      _append_cols(m.cols(), entire(pm::cols(m)), bool2type<restriction==sparse2d::only_cols>());
      return *this;
   }

   template <typename Set>
   RestrictedIncidenceMatrix& operator|= (const GenericSet<Set, int, operations::cmp>& s)
   {
      _append_cols(1, entire(item2container(s.top())), bool2type<restriction==sparse2d::only_cols>());
      return *this;
   }

   void squeeze() { data.squeeze(); }

   template <typename Iterator>
   void permute_rows(Iterator perm)
   {
      data.permute_rows(perm, False());
   }

   template <typename Iterator>
   void permute_cols(Iterator perm)
   {
      data.permute_cols(perm, False());
   }

   template <typename Iterator>
   void permute_inv_rows(Iterator inv_perm)
   {
      data.permute_rows(inv_perm, True());
   }

   template <typename Iterator>
   void permute_inv_cols(Iterator inv_perm)
   {
      data.permute_cols(inv_perm, True());
   }

#if POLYMAKE_DEBUG
   void check() const { data.check(); }
#endif
   friend class Rows<RestrictedIncidenceMatrix>;
   friend class Cols<RestrictedIncidenceMatrix>;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Rows;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Cols;
   template <typename> friend class IncidenceMatrix;
};

template <sparse2d::restriction_kind restriction>
class Rows< RestrictedIncidenceMatrix<restriction> >
   : public sparse2d::Rows< RestrictedIncidenceMatrix<restriction>, nothing, false, restriction,
                            operations::masquerade<incidence_line> > {
protected:
   ~Rows();
public:
   typedef typename if_else<restriction==sparse2d::only_rows, random_access_iterator_tag, output_iterator_tag>::type
      container_category;
};

template <sparse2d::restriction_kind restriction>
class Cols< RestrictedIncidenceMatrix<restriction> >
   : public sparse2d::Cols< RestrictedIncidenceMatrix<restriction>, nothing, false, restriction, 
                            operations::masquerade<incidence_line> > {
protected:
   ~Cols();
public:
   typedef typename if_else<restriction==sparse2d::only_cols, random_access_iterator_tag, output_iterator_tag>::type
      container_category;
};

template <sparse2d::restriction_kind restriction>
struct spec_object_traits< RestrictedIncidenceMatrix<restriction> >
   : spec_object_traits<is_container> {
   static const int dimension=2;

   typedef typename if_else<restriction==sparse2d::only_rows,
                            Rows< RestrictedIncidenceMatrix<restriction> >,
                            Cols< RestrictedIncidenceMatrix<restriction> > >::type serialized;

   static serialized& serialize(RestrictedIncidenceMatrix<restriction>& M)
   {
      return reinterpret_cast<serialized&>(M);
   }
   static const serialized& serialize(const RestrictedIncidenceMatrix<restriction>& M)
   {
      return reinterpret_cast<const serialized&>(M);
   }
};

template <typename symmetric>
class IncidenceMatrix_base {
protected:
   typedef sparse2d::Table<nothing, symmetric::value> table_type;
   shared_object<table_type, AliasHandler<shared_alias_handler> > data;

   table_type& get_table() { return *data; }
   const table_type& get_table() const { return *data; }

   friend IncidenceMatrix_base& make_mutable_alias(IncidenceMatrix_base& alias, IncidenceMatrix_base& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }

   IncidenceMatrix_base() {}

   IncidenceMatrix_base(int r, int c)
      : data(make_constructor(r, c, (table_type*)0)) {}

   template <typename Arg>
   IncidenceMatrix_base(Arg& arg, nothing)
      : data( constructor<table_type(Arg&)>(arg) ) {}

   template <typename> friend class Rows;
   template <typename> friend class Cols;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Rows;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Cols;
   template <bool, typename> friend class incidence_line_factory;
   template <typename> friend class incidence_line_base;
   template <typename, int> friend class alias;
};

template <typename symmetric>
class Rows< IncidenceMatrix_base<symmetric> >
   : public sparse2d::Rows< IncidenceMatrix_base<symmetric>, nothing, symmetric::value, sparse2d::full,
                            operations::masquerade<incidence_line> > {
protected:
   ~Rows();
};

template <typename symmetric>
class Cols< IncidenceMatrix_base<symmetric> >
   : public sparse2d::Cols< IncidenceMatrix_base<symmetric>, nothing, symmetric::value, sparse2d::full,
                            operations::masquerade<incidence_line> > {
protected:
   ~Cols();
};


/** @class IncidenceMatrix
    @brief 0/1 incidence matrix.

   The only @ref persistent class from the incidence matrix family.
   The implementation is based on a two-dimensional grid of <a href="AVL.html">balanced binary search (AVL) trees</a>,
   the same as for @see SparseMatrix. The whole internal data structure
   is attached to a smart pointer with @see {reference counting}.

   A symmetric incidence matrix is a square matrix whose elements `(i,j)` and `(j,i)`
   are always equal. Internally it is stored in a triangular form, avoiding redundant elements, but appears as a full square.

*/

template <typename symmetric>
class IncidenceMatrix
   : public IncidenceMatrix_base<symmetric>,
     public GenericIncidenceMatrix< IncidenceMatrix<symmetric> > {
protected:
   typedef IncidenceMatrix_base<symmetric> base;
   friend IncidenceMatrix& make_mutable_alias(IncidenceMatrix& alias, IncidenceMatrix& owner)
   {
      return static_cast<IncidenceMatrix&>(make_mutable_alias(static_cast<base&>(alias), static_cast<base&>(owner)));
   }

   /// initialize from a dense boolean sequence in row order
   template <typename Iterator>
   void _init(Iterator src, True)
   {
      const int n=this->cols();
      for (typename Entire< Rows<base> >::iterator r_i=entire(pm::rows(static_cast<base&>(*this))); !r_i.at_end(); ++r_i) {
         int i=0;
         if (symmetric::value) {
            i=r_i.index();
            std::advance(src,i);
         }
         for (; i<n; ++i, ++src)
            if (*src) r_i->push_back(i);
      }
   }

   /// initialize rowwise from a sequence of sets
   template <typename Iterator>
   void _init(Iterator src, False)
   {
      copy(src, entire(pm::rows(static_cast<base&>(*this))));
   }

   typedef incidence_proxy_base< incidence_line<typename base::table_type::primary_tree_type> > proxy_base;
public:
   typedef typename if_else<symmetric::value, void, RestrictedIncidenceMatrix<> >::type unknown_columns_type;
   typedef bool value_type;
   typedef sparse_elem_proxy<proxy_base> reference;
   typedef const bool const_reference;

   /// Create an empty IncidenceMatrix.
   IncidenceMatrix() {}

   /// Create an empty IncidenceMatrix with @a r rows and @a c columns initialized with zeroes.
   IncidenceMatrix(int r, int c)
      : base(r,c) {}

   /** @brief Create an IncidenceMatrix IncidenceMatrix with @a r rows and @a c columns and initialize it from a data sequence.

       @a src should iterate either over @a r&times;@a c boolean values, corresponding to the
       elements in the row order (the column index changes first,) or over @a r sets with
       integer elements (or convertible to integer), which are assigned to the matrix rows.

       In the symmetric case the redundant elements must be present in the input sequence; their values are ignored.

       @param r the number of rows
       @param c the number of columns
       @param src an iterator
   */
   template <typename Iterator>
   IncidenceMatrix(int r, int c, Iterator src)
      : base(r,c)
   {
      _init(src, bool2type<(object_traits<typename iterator_traits<Iterator>::value_type>::total_dimension==0)>());
   }

   IncidenceMatrix(const GenericIncidenceMatrix<IncidenceMatrix>& M)
      : base(M.top()) {}

   template <typename Matrix2>
   IncidenceMatrix(const GenericIncidenceMatrix<Matrix2>& M,
                   typename enable_if<void**, (!symmetric::value || Matrix2::is_symmetric)>::type=0)
      : base(M.rows(), M.cols())
   {
      _init(pm::rows(M).begin(), False());
   }

   template <sparse2d::restriction_kind restriction>
   explicit IncidenceMatrix(RestrictedIncidenceMatrix<restriction>& M,
                            typename disable_if<typename cons<void**, RestrictedIncidenceMatrix<restriction> >::head, symmetric::value>::type=0)
      : base(M.data, nothing()) {}

   /// number of columns not known in advance
   template <typename Container>
   IncidenceMatrix(const Container& src,
                   typename enable_if<void**, (isomorphic_to_container_of<Container, Set<int>, allow_conversion>::value &&
                                               !symmetric::value)>::type=0)
   {
      RestrictedIncidenceMatrix<> Mt(src);
      *this=Mt;
   }

   /// number of columns given explicitly
   template <typename Container>
   IncidenceMatrix(const Container& src,
                   typename enable_if<int, isomorphic_to_container_of<Container, Set<int>, allow_conversion>::value &&
                                           !symmetric::value>::type c)
      : base(src.size(), c)
   {
      _init(src.begin(), False());
   }

   IncidenceMatrix& operator= (const IncidenceMatrix& other) { assign(other); return *this; }
#ifdef __clang__
   template <typename Matrix2>
   IncidenceMatrix& operator=(const GenericIncidenceMatrix<Matrix2>& other) { return IncidenceMatrix::generic_type::operator=(other); }
#else
   using IncidenceMatrix::generic_type::operator=;
#endif

   template <sparse2d::restriction_kind restriction>
   typename disable_if<typename cons<IncidenceMatrix, RestrictedIncidenceMatrix<restriction> >::head, symmetric::value>::type&
   operator= (RestrictedIncidenceMatrix<restriction>& M)
   {
      this->data=make_constructor(M.data,(typename base::table_type*)0);
      return *this;
   }

   /// Swap the contents with that of another matrix in an efficient way.
   void swap(IncidenceMatrix& M) { this->data.swap(M.data); }

   friend void relocate(IncidenceMatrix* from, IncidenceMatrix* to)
   {
      relocate(&from->data, &to->data);
   }

/**  @brief Extend or truncate to new dimensions (@a m rows, @a n columns).
   Surviving elements keep their values, new elements are implicitly @c false.

   @c IncidenceMatrix deploys an adaptive reallocation strategy similar to @c std::vector,
   reserving additional stock memory by every reallocation. If you repeatedly increase the matrix dimensions by one,
   the amortized reallocation costs will be proportional to the logarithm of the final dimension.

   A special case, looking at the first glance like a "no operation": @c{ M.resize(M.rows(), M.cols()) },
   gets rid of this extra allocated storage.
*/
   void resize(int m, int n) { this->data->resize(m,n); }

   /// Clear contents.
   void clear() { this->data.apply(shared_clear()); }

   /// Clear contents.
   void clear(int r, int c) { this->data.apply(typename base::table_type::shared_clear(r,c)); }

   /// Entry at row i column j.
   reference operator() (int i, int j)
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=this->rows() || j<0 || j>=this->cols())
            throw std::runtime_error("IncidenceMatrix::operator() - index out of range");
      }
      return proxy_base(pm::rows(static_cast<base&>(*this))[i],j);
   }

   /// Entry at row i column j (const).
   const_reference operator() (int i, int j) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>=this->rows() || j<0 || j>=this->cols())
            throw std::runtime_error("IncidenceMatrix::operator() - index out of range");
      }
      return pm::rows(static_cast<const base&>(*this))[i].exists(j);
   }

   /// Returns the entry at position (i,j).
   bool exists(int i, int j) const { return operator()(i,j); }

   template <typename row_number_consumer, typename col_number_consumer>
   void squeeze(row_number_consumer rnc, col_number_consumer cnc) { this->data->squeeze(rnc,cnc); }

   template <typename row_number_consumer>
   void squeeze(row_number_consumer rnc) { this->data->squeeze(rnc); }

   /// Delete empty rows and columns, renumber the rest and reduce the dimensions.
   void squeeze() { this->data->squeeze(); }

   template <typename row_number_consumer>
   void squeeze_rows(row_number_consumer rnc) { this->data->squeeze_rows(rnc); }

   /// Delete empty rows, renumber the rest and reduce the dimensions.
   void squeeze_rows() { this->data->squeeze_rows(); }

   template <typename col_number_consumer>
   void squeeze_cols(col_number_consumer cnc) { this->data->squeeze_cols(cnc); }

   /// Delete empty columns, renumber the rest and reduce the dimensions.
   void squeeze_cols() { this->data->squeeze_cols(); }

   /// Permute the rows according to the given permutation.
   template <typename Iterator>
   void permute_rows(Iterator perm)
   {
      this->data->permute_rows(perm, False());
   }

   /// Permute the columns according to the given permutation.
   template <typename Iterator>
   void permute_cols(Iterator perm)
   {
      this->data->permute_cols(perm, False());
   }
   
   /// Permute the rows according to the inverse of the given permutation.
   template <typename Iterator>
   void permute_inv_rows(Iterator inv_perm)
   {
      this->data->permute_rows(inv_perm, True());
   }

   /// Permute the columns according to the inverse of the given permutation.
   template <typename Iterator>
   void permute_inv_cols(Iterator inv_perm)
   {
      this->data->permute_cols(inv_perm, True());
   }

   template <typename Perm, typename InvPerm>
   IncidenceMatrix copy_permuted(const Perm& perm, const InvPerm& inv_perm,
                                 typename enable_if<typename cons<void**, Perm>::head, symmetric::value>::type=0) const
   {
      const int n=this->rows();
      IncidenceMatrix result(n,n);
      result.data.get()->copy_permuted(*this->data, perm, inv_perm);
      return result;
   }

#if POLYMAKE_DEBUG
   void check() const { this->data->check(); }
#endif
protected:
   void assign(const GenericIncidenceMatrix<IncidenceMatrix>& M) { this->data=M.top().data; }

   template <typename Matrix>
   void assign(const GenericIncidenceMatrix<Matrix>& M)
   {
      if (this->data.is_shared() || this->rows() != M.rows() || this->cols() != M.cols())
         // circumvent the symmetry checks, they are already done in GenericIncidenceMatrix methods
         assign(IncidenceMatrix(M.rows(), M.cols(), pm::rows(M).begin()));
      else
         GenericIncidenceMatrix<IncidenceMatrix>::assign(M);
   }

   template <typename Matrix2>
   void append_rows(const Matrix2& m)
   {
      const int old_rows=this->rows();
      this->data.apply(typename base::table_type::shared_add_rows(m.rows()));
      copy(entire(pm::rows(m)), pm::rows(static_cast<base&>(*this)).begin()+old_rows);
   }

   template <typename Set2>
   void append_row(const Set2& s)
   {
      const int old_rows=this->rows();
      this->data.apply(typename base::table_type::shared_add_rows(1));
      this->row(old_rows)=s;
   }

   template <typename Matrix2>
   void append_cols(const Matrix2& m)
   {
      const int old_cols=this->cols();
      this->data.apply(typename base::table_type::shared_add_cols(m.cols()));
      copy(entire(pm::cols(m)), pm::cols(static_cast<base&>(*this)).begin()+old_cols);
   }

   template <typename Set2>
   void append_col(const Set2& s)
   {
      const int old_cols=this->cols();
      this->data.apply(typename base::table_type::shared_add_cols(1));
      this->col(old_cols)=s;
   }

   void stretch_rows(int r)
   {
      this->data->resize_rows(r);
   }

   void stretch_cols(int c)
   {
      this->data->resize_cols(c);
   }

   template <typename> friend class GenericIncidenceMatrix;
   friend class Rows<IncidenceMatrix>;
   friend class Cols<IncidenceMatrix>;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Rows;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Cols;
   template <typename, typename> friend class RowChain;
   template <typename, typename> friend class ColChain;
};

template <typename symmetric>
struct check_container_feature< IncidenceMatrix<symmetric>, Symmetric > : symmetric {};

template <bool rowwise, typename BaseRef>
class incidence_line_factory {
public:
   typedef BaseRef first_argument_type;
   typedef int second_argument_type;
   typedef typename if_else<rowwise, typename deref<BaseRef>::type::table_type::row_tree_type,
                                     typename deref<BaseRef>::type::table_type::col_tree_type>::type
      tree_type;
   typedef incidence_line<typename inherit_ref<tree_type, BaseRef>::type> result_type;

   result_type operator() (BaseRef matrix, int index) const
   {
      return result_type(matrix,index);
   }
};

template <bool rowwise>
class incidence_line_factory<rowwise, void> : public operations::incomplete {};

template <bool rowwise, typename BaseRef>
struct operation_cross_const_helper< incidence_line_factory<rowwise, BaseRef> > {
   typedef incidence_line_factory<rowwise, typename attrib<BaseRef>::minus_const> operation;
   typedef incidence_line_factory<rowwise, typename attrib<BaseRef>::plus_const> const_operation;
};

template <bool rowwise, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< incidence_line_factory<rowwise>, Iterator1, Iterator2, Reference1, Reference2>
   : empty_op_builder< incidence_line_factory<rowwise,Reference1> > {};

template <typename symmetric>
class Rows< IncidenceMatrix<symmetric> >
   : public modified_container_pair_impl< Rows< IncidenceMatrix<symmetric> >,
                                          list( Container1< constant_value_container< IncidenceMatrix_base<symmetric>& > >,
                                                Container2< sequence >,
                                                Operation< pair< incidence_line_factory<true>,
                                                                 BuildBinaryIt<operations::dereference2> > >,
                                                MasqueradedTop ) > {
protected:
   ~Rows();
public:
   constant_value_container< IncidenceMatrix_base<symmetric>& > get_container1()
   {
      return this->hidden();
   }
   const constant_value_container< const IncidenceMatrix_base<symmetric>& > get_container1() const
   {
      return this->hidden();
   }
   sequence get_container2() const
   {
      return sequence(0, this->hidden().get_table().rows());
   }
   void resize(int n)
   {
      this->hidden().get_table().resize_rows(n);
   }
};

template <typename symmetric>
class Cols< IncidenceMatrix<symmetric> >
   : public modified_container_pair_impl< Cols< IncidenceMatrix<symmetric> >,
                                          list( Container1< constant_value_container< IncidenceMatrix_base<symmetric>& > >,
                                                Container2< sequence >,
                                                Operation< pair< incidence_line_factory<false>,
                                                                 BuildBinaryIt<operations::dereference2> > >,
                                                MasqueradedTop ) > {
protected:
   ~Cols();
public:
   constant_value_container< IncidenceMatrix_base<symmetric>& > get_container1()
   {
      return this->hidden();
   }
   const constant_value_container< const IncidenceMatrix_base<symmetric>& > get_container1() const
   {
      return this->hidden();
   }
   sequence get_container2() const
   {
      return sequence(0, this->hidden().get_table().cols());
   }
   void resize(int n)
   {
      this->hidden().get_table().resize_cols(n);
   }
};

/// Convolution of two incidence relations.
template <typename Matrix1, typename Matrix2> inline
IncidenceMatrix<>
convolute(const GenericIncidenceMatrix<Matrix1>& m1, const GenericIncidenceMatrix<Matrix2>& m2)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix1>::value || !Unwary<Matrix2>::value) {
      if (m1.cols() != m2.rows())
         throw std::runtime_error("convolute - dimension mismatch");
   }
   IncidenceMatrix<> result(m1.rows(), m2.cols());
   typename Rows<Matrix1>::const_iterator r1=rows(m1).begin();
   for (typename Entire< Rows< IncidenceMatrix<> > >::iterator dst=entire(rows(result));
        !dst.at_end();  ++dst, ++r1)
      accumulate_in(entire(rows(m2.minor(*r1,All))), BuildBinary<operations::add>(), *dst);

   return result;
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type, !Matrix::is_symmetric>::type
permuted_rows(const GenericIncidenceMatrix<Matrix>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_rows - dimension mismatch");
   }
   RestrictedIncidenceMatrix<sparse2d::only_rows> result(m.rows(), m.cols(), select(rows(m),perm).begin());
   return IncidenceMatrix<>(result);
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type, !Matrix::is_symmetric>::type
permuted_cols(const GenericIncidenceMatrix<Matrix>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_cols - dimension mismatch");
   }
   RestrictedIncidenceMatrix<sparse2d::only_cols> result(m.rows(), m.cols(), select(cols(m),perm).begin());
   return IncidenceMatrix<>(result);
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type, !Matrix::is_symmetric>::type
permuted_inv_rows(const GenericIncidenceMatrix<Matrix>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   RestrictedIncidenceMatrix<sparse2d::only_rows> result(m.rows(), m.cols());
   copy(entire(rows(m)), select(rows(result),perm).begin());
   return IncidenceMatrix<>(result);
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type, !Matrix::is_symmetric>::type
permuted_inv_cols(const GenericIncidenceMatrix<Matrix>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_inv_cols - dimension mismatch");
   }
   RestrictedIncidenceMatrix<sparse2d::only_cols> result(m.rows(), m.cols());
   copy(entire(cols(m)), select(cols(result),perm).begin());
   return IncidenceMatrix<>(result);
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type, Matrix::is_symmetric>::type
permuted_rows(const GenericIncidenceMatrix<Matrix>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_rows - dimension mismatch");
   }
   std::vector<int> inv_perm(m.rows());
   inverse_permutation(perm,inv_perm);
   return m.top().copy_permuted(perm,inv_perm);
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type, (Matrix::is_symmetric && container_traits<Permutation>::is_random)>::type
permuted_inv_rows(const GenericIncidenceMatrix<Matrix>& m, const Permutation& inv_perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.rows() != inv_perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   std::vector<int> perm(m.rows());
   inverse_permutation(inv_perm,perm);
   return m.copy_permuted(perm,inv_perm);
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type, (Matrix::is_symmetric && !container_traits<Permutation>::is_random)>::type
permuted_inv_rows(const GenericIncidenceMatrix<Matrix>& m, const Permutation& inv_perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.rows() != inv_perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   std::vector<int> inv_perm_copy(inv_perm.size());
   copy(entire(inv_perm), inv_perm_copy.begin());
   return permuted_inv_rows(m,inv_perm_copy);
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type, Matrix::is_symmetric>::type
permuted_cols(const GenericIncidenceMatrix<Matrix>& m, const Permutation& perm)
{
   return permuted_rows(m,perm);
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type, Matrix::is_symmetric>::type
permuted_inv_cols(const GenericIncidenceMatrix<Matrix>& m, const Permutation& inv_perm)
{
   return permuted_inv_rows(m,inv_perm);
}

} // end namespace pm

namespace polymake {
   using pm::IncidenceMatrix;
   using pm::RestrictedIncidenceMatrix;
}

namespace std {
   template <typename symmetric> inline
   void swap(pm::IncidenceMatrix<symmetric>& M1, pm::IncidenceMatrix<symmetric>& M2) { M1.swap(M2); }

   template <pm::sparse2d::restriction_kind restriction> inline
   void swap(pm::RestrictedIncidenceMatrix<restriction>& M1,
             pm::RestrictedIncidenceMatrix<restriction>& M2)
   {
      M1.swap(M2);
   }

   template <typename TreeRef> inline
   void swap(pm::incidence_line<TreeRef>& l1, pm::incidence_line<TreeRef>& l2)
   {
      l1.swap(l2);
   }
}

#endif // POLYMAKE_INCIDENCE_MATRIX_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
