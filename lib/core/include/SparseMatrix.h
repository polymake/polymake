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

/** @file SparseMatrix.h
    @brief Implementation of pm::SparseMatrix class
 */

#ifndef POLYMAKE_SPARSE_MATRIX_H
#define POLYMAKE_SPARSE_MATRIX_H

#include "polymake/internal/sparse2d.h"
#include "polymake/Matrix.h"
#include "polymake/SparseVector.h"

namespace pm {

template <typename Iterator=void>
class skew_negator {
protected:
   int diag;
   operations::neg<typename iterator_traits<Iterator>::reference> op;
public:
   skew_negator(int diag_arg=-1) : diag(diag_arg) {}

   typedef Iterator argument_type;
   typedef typename iterator_traits<Iterator>::value_type result_type;

   result_type operator() (const argument_type& it) const
   {
      if (it.index() > diag) return op(*it);
      return *it;
   }
};

template <>
class skew_negator<void> : operations::incomplete {
protected:
   int diag;
public:
   skew_negator(int diag_arg=-1) : diag(diag_arg) {}
   operator int () const { return diag; }
};

template <typename Iterator, typename Reference>
struct unary_op_builder< skew_negator<void>, Iterator, Reference > {
   typedef skew_negator<Iterator> operation;
   static operation create(int diag_arg) { return operation(diag_arg); }
};

template <typename TreeRef, typename symmetric> class sparse_matrix_line;
template <bool rowwise, typename symmetric, typename BaseRef=void> class sparse_matrix_line_factory;
template <typename E, typename symmetric> class SparseMatrix_base;

template <typename TreeRef, typename symmetric>
struct sparse_matrix_line_params
   : sparse2d::line_params<TreeRef> {};

template <typename TreeRef>
struct sparse_matrix_line_params<TreeRef, SkewSymmetric>
   : concat_list< typename sparse2d::line_params<TreeRef>::type,
                  Operation< skew_negator<> > > {
public:
   operations::identity<int> get_operation() const
   {
      return operations::identity<int>();
   }
};

template <typename TreeRef, typename symmetric>
class sparse_matrix_line_ops
   : public modified_tree< sparse_matrix_line<TreeRef,symmetric>,
                           typename sparse_matrix_line_params<TreeRef,symmetric>::type >,
     public GenericVector< sparse_matrix_line<TreeRef,symmetric>,
                           typename deref<TreeRef>::type::mapped_type> {};

template <typename TreeRef>
class sparse_matrix_line_ops<TreeRef, SkewSymmetric>
   : public modified_tree< sparse_matrix_line<TreeRef,SkewSymmetric>,
                           typename sparse_matrix_line_params<TreeRef,SkewSymmetric>::type >,
     public GenericVector< sparse_matrix_line<TreeRef,SkewSymmetric>,
                           typename deref<TreeRef>::type::mapped_type> {

   typedef modified_tree< sparse_matrix_line<TreeRef,SkewSymmetric>,
                          typename sparse_matrix_line_params<TreeRef,SkewSymmetric>::type > super;
public:
   typedef typename deref<TreeRef>::type::mapped_type value_type;

   skew_negator<> get_operation() const
   {
      return this->top().index();
   }

protected:
   template <typename Vector>
   void assign(const Vector& v)
   {
      assign_sparse(this->top().get_container(), attach_operation(ensure(v, (pure_sparse*)0), get_operation()).begin());
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      perform_assign(entire(this->top().get_container()), op);
   }

   template <typename Vector, typename Operation>
   typename disable_if<void, operations::is_partially_defined_for<Operation,sparse_matrix_line_ops,Vector>::value>::type
   assign_op(const Vector& v, const Operation& op)
   {
      perform_assign(entire(this->top().get_container()), v.begin(), op);
   }

   template <typename Vector, typename Operation>
   typename enable_if<void, operations::is_partially_defined_for<Operation,sparse_matrix_line_ops,Vector>::value>::type
   assign_op(const Vector& v, const Operation& op)
   {
      perform_assign_sparse(this->top().get_container(), attach_operation(ensure(v, (pure_sparse*)0), get_operation()).begin(), op);
   }

   void _fill(typename function_argument<value_type>::type x, pure_sparse)
   {
      if (x)
         fill_sparse(this->top().get_container(), attach_operation(ensure(constant(x), (indexed*)0), get_operation()).begin());
      else
         this->clear();
   }

public:
   typename super::iterator insert(int i, const value_type& x)
   {
      operations::neg<const value_type&> op;
      return super::insert(i, i > this->top().index() ? op(x) : x);
   }

   typename super::iterator insert(const typename super::iterator& pos, int i)
   {
      return super::insert(pos, i);
   }

   typename super::iterator insert(const typename super::iterator& pos, int i, const value_type& x)
   {
      operations::neg<const value_type&> op;
      return super::insert(pos, i, i > this->top().index() ? op(x) : x);
   }
};

template <typename TreeRef, typename symmetric>
class sparse_matrix_line_base
   : public sparse_matrix_line_ops<TreeRef, symmetric> {
protected:
   typedef nothing first_arg_type;
   typedef nothing second_arg_type;
   ~sparse_matrix_line_base();
public:
   int index() const { return this->get_container().get_line_index(); }
};

template <typename Tree, typename symmetric>
class sparse_matrix_line_base<Tree&, symmetric>
   : public sparse_matrix_line_ops<Tree&, symmetric> {
protected:
   typedef typename deref<Tree>::type tree_type;
   typedef typename inherit_ref<SparseMatrix_base<typename sparse_matrix_line_base::element_type, symmetric>, Tree&>::type matrix_ref;
   typedef typename attrib<matrix_ref>::plus_const const_matrix_ref;

   alias<matrix_ref> matrix;
   int line_index;

   typedef typename alias<matrix_ref>::arg_type first_arg_type;
   typedef int second_arg_type;

   sparse_matrix_line_base(first_arg_type arg1, second_arg_type arg2)
      : matrix(arg1), line_index(arg2)  {}
public:
   typename inherit_const<typename sparse_matrix_line_base::container, Tree>::type& get_container()
   {
      return matrix->get_table().get_line(line_index, (tree_type*)0);
   }
   const typename sparse_matrix_line_base::container& get_container() const
   {
      return matrix->get_table().get_line(line_index, (tree_type*)0);
   }

   int index() const { return line_index; }
};

template <typename TreeRef, typename symmetric>
class sparse_matrix_line
   : public sparse_matrix_line_base<TreeRef, symmetric> {
   typedef sparse_matrix_line_base<TreeRef, symmetric> _super;

   friend class GenericVector<sparse_matrix_line>;
   template <typename,typename> friend class SparseMatrix;
   template <typename,typename> friend class GenericMatrix;

public:
   sparse_matrix_line(typename _super::first_arg_type arg1, typename _super::second_arg_type arg2)
      : _super(arg1, arg2) {}

   static const bool is_skew_symmetric=identical<symmetric, SkewSymmetric>::value;
   typedef typename if_else< identical<symmetric,NonSymmetric>::value, nothing, symmetric >::type operate_on_lower;

protected:
   using _super::assign_op;

   template <typename Vector, typename Operation>
   typename disable_if<void, operations::is_partially_defined_for<Operation,sparse_matrix_line,Vector>::value>::type
   assign_op(const Vector& v, const Operation& op, operate_on_lower)
   {
      perform_assign(entire(sparse2d::select_lower_triangle(this->get_container())), v.begin(), op);
   }

   template <typename Vector, typename Operation>
   typename enable_if<void, operations::is_partially_defined_for<Operation,sparse_matrix_line,Vector>::value>::type
   assign_op(const Vector& v, const Operation& op, operate_on_lower)
   {
      perform_assign_sparse(sparse2d::select_lower_triangle(this->get_container()),
                            attach_truncator(ensure(v, (pure_sparse*)0), index_truncator(this->index())).begin(), op);
   }

public:
   sparse_matrix_line& operator= (sparse_matrix_line& other)
   {
      return sparse_matrix_line::generic_type::operator=(other);
   }
   using sparse_matrix_line::generic_type::operator=;

   typedef typename deref<TreeRef>::type::mapped_type value_type;
protected:
   typedef sparse_proxy_base< sparse2d::line<typename deref<TreeRef>::type> > proxy_base;
public:
   typedef typename if_else<is_skew_symmetric, const value_type, const value_type&>::type const_reference;
   typedef typename if_else<attrib<TreeRef>::is_const,
                            const_reference, sparse_elem_proxy<proxy_base, value_type, symmetric> >::type
      reference;
   typedef random_access_iterator_tag container_category;

   const_reference operator[] (int i) const
   {
      return deref_sparse_iterator(this->find(i));
   }

protected:
   reference _random(int i, False) { return proxy_base(this->get_container(),i); }
   reference _random(int i, True) const { return operator[](i); }
public:
   reference operator[] (int i)
   {
      return _random(i, bool2type<attrib<TreeRef>::is_const>());
   }

   int dim() const { return this->get_container().dim(); }

protected:
   typedef typename if_else< identical<symmetric, NonSymmetric>::value, maximal<int>, int >::type input_limit_type;

   maximal<int> _get_input_limit(type2type< maximal<int> >) const { return maximal<int>(); }

   int _get_input_limit(type2type<int>) const { return this->index(); }

   friend
   input_limit_type get_input_limit(sparse_matrix_line& me)
   {
      return me._get_input_limit(type2type<input_limit_type>());
   }
};

template <typename TreeRef, typename symmetric>
struct check_container_feature<sparse_matrix_line<TreeRef,symmetric>, pure_sparse> : True {};

template <typename TreeRef, typename symmetric>
struct spec_object_traits< sparse_matrix_line<TreeRef,symmetric> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=attrib<TreeRef>::is_reference,
                     is_always_const=attrib<TreeRef>::is_const;
   typedef typename if_else<is_temporary, void, typename deref<TreeRef>::type>::type masquerade_for;
   static const int is_resizeable= deref<TreeRef>::type::fixed_dim ? 0 : -1;
};

template <typename E, sparse2d::restriction_kind restriction=sparse2d::only_rows>
class RestrictedSparseMatrix
   : public matrix_methods<RestrictedSparseMatrix<E,restriction>, E> {
protected:
   typedef sparse2d::Table<E, false, restriction> table_type;
   table_type data;

   table_type& get_table() { return data; }
   const table_type& get_table() const { return data; }

   template <typename Iterator>
   void _copy(Iterator src, True) { copy(src, entire(pm::rows(*this))); }

   template <typename Iterator>
   void _copy(Iterator src, False) { copy(src, entire(pm::cols(*this))); }

   typedef sparse_matrix_line<typename table_type::primary_tree_type, NonSymmetric> _line;
public:
   typedef E value_type;
   typedef typename _line::reference reference;
   typedef const E& const_reference;

   explicit RestrictedSparseMatrix(int n=0) : data(n) {}
   RestrictedSparseMatrix(int r, int c) : data(r,c) {}

   template <typename Iterator>
   RestrictedSparseMatrix(typename enable_if<int, isomorphic_types<typename iterator_traits<Iterator>::value_type, Vector<E> >::value>::type n, Iterator src)
      : data(n)
   {
      _copy(src, bool2type<restriction==sparse2d::only_rows>());
   }

   template <typename Iterator>
   RestrictedSparseMatrix(typename enable_if<int, isomorphic_types<typename iterator_traits<Iterator>::value_type, Vector<E> >::value>::type r, int c, Iterator src)
      : data(r,c)
   {
      _copy(src, bool2type<restriction==sparse2d::only_rows>());
   }

   void swap(RestrictedSparseMatrix& M)
   {
      data.swap(M.data);
   }

   void clear() { data.clear(); }

protected:
   reference _random(int i, int j, False)
   {
      return this->row(i)[j];
   }
   reference _random(int i, int j, True)
   {
      return this->col(j)[i];
   }
   const_reference _random(int i, int j, False) const
   {
      return this->row(i)[j];
   }
   const_reference _random(int i, int j, True) const
   {
      return this->col(j)[i];
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

private:
   template <typename RowCol, typename Vector>
   void _append(RowCol& row_col, const Vector& vec, int i)
   {
      for (typename Entire<Vector>::const_iterator v=entire(vec); !v.at_end(); ++v)
         row_col[v.index()].push_back(i,*v);
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
   RestrictedSparseMatrix& operator/= (const GenericMatrix<Matrix>& m)
   {
      _append_rows(m.rows(), entire(pm::rows(m)), bool2type<restriction==sparse2d::only_rows>());
      return *this;
   }

   template <typename Vector>
   RestrictedSparseMatrix& operator/= (const GenericVector<Vector>& v)
   {
      _append_rows(1, entire(item2container(v.top())), bool2type<restriction==sparse2d::only_rows>());
      return *this;
   }

   template <typename Matrix>
   RestrictedSparseMatrix& operator|= (const GenericMatrix<Matrix>& m)
   {
      _append_cols(m.cols(), entire(pm::cols(m)), bool2type<restriction==sparse2d::only_cols>());
      return *this;
   }

   template <typename Vector>
   RestrictedSparseMatrix& operator|= (const GenericVector<Vector>& v)
   {
      _append_cols(1, entire(item2container(v.top())), bool2type<restriction==sparse2d::only_cols>());
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
   friend class Rows<RestrictedSparseMatrix>;
   friend class Cols<RestrictedSparseMatrix>;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Rows;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Cols;
   template <typename,typename> friend class SparseMatrix;
};

template <typename E, sparse2d::restriction_kind restriction>
struct spec_object_traits< RestrictedSparseMatrix<E, restriction> >
   : spec_object_traits<is_container> {
   static const int dimension=2;

   typedef typename if_else<restriction==sparse2d::only_rows,
                            Rows< RestrictedSparseMatrix<E, restriction> >,
                            Cols< RestrictedSparseMatrix<E, restriction> > >::type serialized;

   static serialized& serialize(RestrictedSparseMatrix<E, restriction>& M)
   {
      return reinterpret_cast<serialized&>(M);
   }
   static const serialized& serialize(const RestrictedSparseMatrix<E, restriction>& M)
   {
      return reinterpret_cast<const serialized&>(M);
   }
};

template <typename E, sparse2d::restriction_kind restriction>
class Rows< RestrictedSparseMatrix<E,restriction> >
   : public sparse2d::Rows< RestrictedSparseMatrix<E,restriction>, E, false, restriction,
                            operations::masquerade2<sparse_matrix_line, NonSymmetric> > {
protected:
   ~Rows();
public:
   typedef typename if_else<restriction==sparse2d::only_rows, random_access_iterator_tag, output_iterator_tag>::type
      container_category;
};

template <typename E, sparse2d::restriction_kind restriction>
class Cols< RestrictedSparseMatrix<E,restriction> >
   : public sparse2d::Cols< RestrictedSparseMatrix<E,restriction>, E, false, restriction,
                            operations::masquerade2<sparse_matrix_line, NonSymmetric> > {
protected:
   ~Cols();
public:
   typedef typename if_else<restriction==sparse2d::only_cols, random_access_iterator_tag, output_iterator_tag>::type
      container_category;
};

template <typename E, typename symmetric>
class SparseMatrix_base {
protected:
   typedef sparse2d::Table<E, symmetric::value> table_type;
   shared_object<table_type, AliasHandler<shared_alias_handler> > data;

   table_type& get_table() { return *data; }
   const table_type& get_table() const { return *data; }

   friend SparseMatrix_base& make_mutable_alias(SparseMatrix_base& alias, SparseMatrix_base& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }

   SparseMatrix_base() {}

   SparseMatrix_base(int r, int c)
      : data(make_constructor(c ? r : 0, r ? c : 0, (table_type*)0)) {}

   template <typename Arg>
   SparseMatrix_base(Arg& arg, nothing)
      : data( constructor<table_type(Arg&)>(arg) ) {}

   template <typename> friend class Rows;
   template <typename> friend class Cols;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Rows;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Cols;
   template <bool, typename, typename> friend class sparse_matrix_line_factory;
   template <typename, typename> friend class sparse_matrix_line_base;
   template <typename, int> friend class alias;
};

template <typename E, typename symmetric>
class Rows< SparseMatrix_base<E,symmetric> >
   : public sparse2d::Rows< SparseMatrix_base<E,symmetric>, E, symmetric::value, sparse2d::full,
                            operations::masquerade2<sparse_matrix_line, symmetric> > {
protected:
   ~Rows();
};

template <typename E, typename symmetric>
class Cols< SparseMatrix_base<E,symmetric> >
   : public sparse2d::Cols< SparseMatrix_base<E,symmetric>, E, symmetric::value, sparse2d::full,
                            operations::masquerade2<sparse_matrix_line, symmetric> > {
protected:
   ~Cols();
};

/** @class SparseMatrix
    @brief A two-dimensional associative array with row and column indices as keys.  
	
     A two-dimensional associative array with row and column indices as keys; elements equal to the default value (ElementType(), 
     which is 0 for most numerical types) are not stored, but implicitly encoded by the gaps in the key set. 
     Each row and column is organized as a balanced binary search (%AVL) tree. 
*/
template <typename E, typename symmetric>
class SparseMatrix
   : public SparseMatrix_base<E, symmetric>,
     public GenericMatrix< SparseMatrix<E,symmetric>, E> {
protected:
   typedef SparseMatrix_base<E, symmetric> base;
   friend SparseMatrix& make_mutable_alias(SparseMatrix& alias, SparseMatrix& owner)
   {
      return static_cast<SparseMatrix&>(make_mutable_alias(static_cast<base&>(alias), static_cast<base&>(owner)));
   }

   template <typename Iterator>
   struct matching_converter :
      if_else< convertible_to<typename iterator_traits<Iterator>::value_type, E>::value,
               conv<E,E>,
               conv<typename iterator_traits<Iterator>::value_type, E> > {};

   // elementwise, non-symmetric
   template <typename Iterator>
   void _init(Iterator src, True, False)
   {
      typename matching_converter<Iterator>::type conv;
      const int n=this->cols();
      for (typename Entire< Rows<base> >::iterator r_i=entire(pm::rows(static_cast<base&>(*this))); !r_i.at_end(); ++r_i)
         for (int i=0; i<n; ++i, ++src)
            if (!is_zero(*src))
               r_i->push_back(i, conv(*src));
   }

   // elementwise, symmetric
   template <typename Iterator>
   void _init(Iterator src, True, True)
   {
      typename matching_converter<Iterator>::type conv;
      const int n=this->cols();
      int d=0;
      for (typename Entire< Rows<base> >::iterator r_i=entire(pm::rows(static_cast<base&>(*this))); !r_i.at_end(); ++r_i) {
         for (int i=0; i<=d; ++i, ++src)
            if (!is_zero(*src))
               r_i->push_back(i, conv(*src));
         ++d; std::advance(src, n-d);
      }
   }

   // rowwise, non-symmetric
   template <typename Iterator>
   void _init(Iterator src, False, False)
   {
      for (typename Entire< Rows<base> >::iterator r_i=entire(pm::rows(static_cast<base&>(*this))); !r_i.at_end(); ++r_i, ++src)
         *r_i = *src;
   }

   template <typename Row, typename Iterator>
   void _init_row(Row& r, Iterator src, int d)
   {
      typename matching_converter<Iterator>::type conv;
      for (int i; !src.at_end() && (i=src.index())<=d; ++src)
         r.push_back(i, conv(*src));
   }

   // rowwise, symmetric
   template <typename Iterator>
   void _init(Iterator src, False, True)
   {
      int d=0;
      for (typename Entire< Rows<base> >::iterator r_i=entire(pm::rows(static_cast<base&>(*this))); !r_i.at_end(); ++r_i, ++d, ++src)
         _init_row(*r_i, ensure(*src, (pure_sparse*)0).begin(), d);
   }

   typedef sparse_matrix_line<typename base::table_type::primary_tree_type, symmetric> _line;
public:
   typedef typename if_else<symmetric::value, void, RestrictedSparseMatrix<E> >::type unknown_columns_type;
   typedef E value_type;
   typedef typename _line::reference reference;
   typedef typename _line::const_reference const_reference;

   /// create as empty
   SparseMatrix() {}

   /// Create a matrix with r rows and c columns, (implicitly) initialize all elements to 0. 
   SparseMatrix(int r, int c)
      : base(r, c) {}

   /**
     Create a matrix with r rows and c columns, initialize the elements from a data sequence. 
     src should iterate either over r*c scalar values, corresponding to the elements in the row order 
     (the column index changes first,) or over r vectors of dimension c, corresponding to the matrix rows. 
     Zero input elements are filtered out. 
   */
   template <typename Iterator>
   SparseMatrix(int r, int c, Iterator src)
      : base(r, c)
   {
      _init(src,
            bool2type<(object_traits<typename iterator_traits<Iterator>::value_type>::total_dimension == object_traits<E>::total_dimension)>(),
            symmetric());
   }

   template <typename E2, size_t r, size_t c>
   explicit SparseMatrix(const E2 (&a)[r][c],
                         typename enable_if<void**, convertible_to<E2, E>::value>::type=0)
      : base(r, c)
   {
      _init(&a[0][0], True(), symmetric());
   }

   template <typename E2, size_t r, size_t c>
   explicit SparseMatrix(const E2 (&a)[r][c],
                         typename enable_if<void**, explicitly_convertible_to<E2, E>::value>::type=0)
      : base(r, c)
   {
      _init(make_converting_iterator<E>(&a[0][0]), True(), symmetric());
   }

   /// Copy of a disguised Matrix object. 
   SparseMatrix(const GenericMatrix<SparseMatrix>& M)
      : base(M.top()) {}

   /// Copy of an abstract matrix of the same element type. 
   template <typename Matrix2>
   SparseMatrix(const GenericMatrix<Matrix2, E>& M,
                typename enable_if<void**, !symmetric::value || identical<symmetric, typename matrix_symmetry_type<Matrix2>::type>::value>::type=0)
      : base(M.rows(), M.cols())
   {
      _init(pm::rows(M).begin(), False(), symmetric());
   }

   /// Copy of an abstract matrix with element conversion. 
   template <typename Matrix2, typename E2>
   explicit SparseMatrix(const GenericMatrix<Matrix2, E2>& M,
                         typename enable_if<void**, ((!symmetric::value || identical<symmetric, typename matrix_symmetry_type<Matrix2>::type>::value) &&
                                                     (convertible_to<E2,E>::value || explicitly_convertible_to<E2,E>::value))>::type=0)
      : base(M.rows(), M.cols())
   {
      _init(pm::rows(M).begin(), False(), symmetric());
   }

   template <sparse2d::restriction_kind restriction>
   explicit SparseMatrix(RestrictedSparseMatrix<E, restriction>& M,
                         typename disable_if<typename cons<void**, RestrictedSparseMatrix<E,restriction> >::head, symmetric::value>::type=0)
      : base(M.data, nothing()) {}

   template <typename Container>
   SparseMatrix(const Container& src,
                typename enable_if<void**, (isomorphic_to_container_of<Container, Vector<E>, allow_conversion>::value &&
                                            !symmetric::value &&
                                            (convertible_to<typename Container::value_type::value_type, E>::value ||
                                             explicitly_convertible_to<typename Container::value_type::value_type, E>::value))>::type=0)
      : base(src.size(), src.empty() ? 0 : get_dim(src.front()))
   {
      _init(src.begin(), False(), symmetric());
   }

   /// Persistent matrix objects have after the assignment the same dimensions as the right hand side operand. 
   /// Alias objects, such as matrix minor or block matrix, cannot be resized, thus must have the same dimensions as on the right hand side.
   SparseMatrix& operator= (const SparseMatrix& other) { assign(other); return *this; }
#ifdef __clang__
   template <typename Matrix2>
   typename SparseMatrix::generic_type::template enable_if_assignable_from<Matrix2>::type&
   operator= (const GenericMatrix<Matrix2>& other) { return SparseMatrix::generic_type::operator=(other); }
#else
   using SparseMatrix::generic_type::operator=;
#endif

   template <sparse2d::restriction_kind restriction>
   typename disable_if<typename cons<SparseMatrix, RestrictedSparseMatrix<E,restriction> >::head, symmetric::value>::type&
   operator= (RestrictedSparseMatrix<E, restriction>& M)
   {
      this->data=make_constructor(M.data, (typename base::table_type*)0);
      return *this;
   }

   /// Exchange the contents of two matrices in a most efficient way. 
   /// If at least one non-persistent object is involved, the operands must have equal dimensions. 
   void swap(SparseMatrix& M) { this->data.swap(M.data); }

   friend void relocate(SparseMatrix* from, SparseMatrix* to)
   {
      relocate(&from->data, &to->data);
   }

   /// Resize to new dimensions, added elements initialized with default constructor.
   void resize(int r, int c) { this->data->resize(r,c); }

   /// Truncate to 0x0 matrix. 
   void clear() { this->data.apply(shared_clear()); }

   void clear(int r, int c) { this->data.apply(typename base::table_type::shared_clear(r,c)); }

   reference operator() (int i, int j)
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>this->rows() || j<0 || j>= this->cols())
            throw std::runtime_error("SparseMatrix::operator() - index out of range");
      }
      return pm::rows(static_cast<base&>(*this))[i][j];
   }

   const_reference operator() (int i, int j) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>this->rows() || j<0 || j>= this->cols())
            throw std::runtime_error("SparseMatrix::operator() - index out of range");
      }
      return pm::rows(static_cast<const base&>(*this))[i][j];
   }

   /// Physically remove all zero elements that might have creeped in by some previous operation. 
   void remove0s()
   {
      for (typename Entire< Rows<SparseMatrix> >::iterator r=entire(pm::rows(static_cast<base&>(*this))); !r.at_end(); ++r)
         r->remove0s();
   }

   template <typename row_number_consumer, typename col_number_consumer>
   void squeeze(row_number_consumer rnc, col_number_consumer cnc) { this->data->squeeze(rnc,cnc); }

   template <typename row_number_consumer>
   void squeeze(row_number_consumer rnc) { this->data->squeeze(rnc); }

   /** Remove all empty (i.e., consisting entirely of implicit zeroes,) rows, renumber the rest, and reduce the dimensions. If you need to know the exact mapping between the old and new row indices, you can supply output iterators (e.g., back_inserter of a std::list.) They will get the old indices of non-empty rows assigned in the ascending order. */
   void squeeze() { this->data->squeeze(); }

   template <typename row_number_consumer>
   void squeeze_rows(row_number_consumer rnc) { this->data->squeeze_rows(rnc); }

   void squeeze_rows() { this->data->squeeze_rows(); }

   template <typename col_number_consumer>
   void squeeze_cols(col_number_consumer cnc) { this->data->squeeze_cols(cnc); }

   /** Remove all empty (i.e., consisting entirely of implicit zeroes,) columns, renumber the rest, and reduce the dimensions. If you need to know the exact mapping between the old and new column indices, you can supply output iterators (e.g., back_inserter of a std::list.) They will get the old indices of non-empty columns assigned in the ascending order. */
   void squeeze_cols() { this->data->squeeze_cols(); }

   /**
   Permute the rows of the matrix without copying the elements. These operations are nevertheless expensive, as they need to visit each element and adjust its indices. 
   */
   template <typename Iterator>
   void permute_rows(Iterator perm)
   {
      this->data->permute_rows(perm, False());
   }

   /**
   Permute the columns of the matrix without copying the elements. These operations are nevetherless expensive, as they need to visit each element and adjust its indices.
   */
   template <typename Iterator>
   void permute_cols(Iterator perm)
   {
      this->data->permute_cols(perm, False());
   }

   template <typename Iterator>
   void permute_inv_rows(Iterator inv_perm)
   {
      this->data->permute_rows(inv_perm, True());
   }

   template <typename Iterator>
   void permute_inv_cols(Iterator inv_perm)
   {
      this->data->permute_cols(inv_perm, True());
   }

   template <typename Perm, typename InvPerm>
   SparseMatrix copy_permuted(const Perm& perm, const InvPerm& inv_perm,
                              typename enable_if<typename cons<void**, Perm>::head, symmetric::value>::type=0) const
   {
      const int n=this->rows();
      SparseMatrix result(n,n);
      result.data.get()->copy_permuted(*this->data, perm, inv_perm);
      return result;
   }

#if POLYMAKE_DEBUG
   void check() const { this->data->check(); }
#endif

protected:
   void assign(const GenericMatrix<SparseMatrix>& m) { this->data=m.top().data; }

   template <typename Matrix2>
   void assign(const GenericMatrix<Matrix2>& m)
   {
      if (this->data.is_shared() || this->rows() != m.rows() || this->cols() != m.cols())
         *this=SparseMatrix(m);
      else
         SparseMatrix::generic_type::assign(m);
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      if (this->data.is_shared())
         *this=SparseMatrix(LazyMatrix1<const SparseMatrix&, Operation>(*this,op));
      else
         SparseMatrix::generic_type::assign_op(op);
   }

   template <typename Matrix2, typename Operation>
   void assign_op(const Matrix2& m, const Operation& op)
   {
      if (this->data.is_shared())
         *this=SparseMatrix(LazyMatrix2<const SparseMatrix&, const Matrix2&, Operation>(*this,m,op));
      else
         SparseMatrix::generic_type::assign_op(m,op);
   }

   template <typename Matrix2>
   void append_rows(const Matrix2& m)
   {
      const int old_rows=this->rows();
      this->data.apply(typename base::table_type::shared_add_rows(m.rows()));
      copy(entire(pm::rows(m)), pm::rows(static_cast<base&>(*this)).begin()+old_rows);
   }

   template <typename Vector2>
   void append_row(const Vector2& v)
   {
      const int old_rows=this->rows();
      this->data.apply(typename base::table_type::shared_add_rows(1));
      this->row(old_rows)=v;
   }

   template <typename Matrix2>
   void append_cols(const Matrix2& m)
   {
      const int old_cols=this->cols();
      this->data.apply(typename base::table_type::shared_add_cols(m.cols()));
      copy(entire(pm::cols(m)), pm::cols(static_cast<base&>(*this)).begin()+old_cols);
   }

   template <typename Vector2>
   void append_col(const Vector2& v)
   {
      const int old_cols=this->cols();
      this->data.apply(typename base::table_type::shared_add_cols(1));
      this->col(old_cols)=v;
   }

   template <typename E2>
   void _fill(const E2& x, False)
   {
      if (this->data.is_shared())
         clear(this->rows(), this->cols());
      if (x)
         SparseMatrix::generic_type::_fill(x, False());
   }

   void stretch_rows(int r)
   {
      this->data->resize_rows(r);
   }

   void stretch_cols(int c)
   {
      this->data->resize_cols(c);
   }

   template <typename, typename> friend class GenericMatrix;
   friend class Rows<SparseMatrix>;
   friend class Cols<SparseMatrix>;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Rows;
   template <typename, typename, bool, sparse2d::restriction_kind, typename> friend class sparse2d::Cols;
   template <typename, typename> friend class RowChain;
   template <typename, typename> friend class ColChain;
};

template <typename E, typename symmetric>
struct check_container_feature< SparseMatrix<E,symmetric>, pure_sparse > : True {};

template <typename E, typename symmetric>
struct check_container_feature< SparseMatrix<E,symmetric>, Symmetric > : identical<symmetric,Symmetric> {};

template <typename E, typename symmetric>
struct check_container_feature< SparseMatrix<E,symmetric>, SkewSymmetric > : identical<symmetric,SkewSymmetric> {};

template <bool rowwise, typename symmetric, typename BaseRef>
class sparse_matrix_line_factory {
public:
   typedef BaseRef first_argument_type;
   typedef int second_argument_type;
   typedef typename if_else<rowwise, typename deref<BaseRef>::type::table_type::row_tree_type,
                                     typename deref<BaseRef>::type::table_type::col_tree_type>::type
      tree_type;
   typedef sparse_matrix_line<typename inherit_ref<tree_type, BaseRef>::type, symmetric> result_type;

   result_type operator() (BaseRef matrix, int index) const
   {
      return result_type(matrix,index);
   }
};

template <bool rowwise, typename symmetric>
class sparse_matrix_line_factory<rowwise, symmetric, void> : public operations::incomplete {};

template <bool rowwise, typename symmetric, typename BaseRef>
struct operation_cross_const_helper< sparse_matrix_line_factory<rowwise, symmetric, BaseRef> > {
   typedef sparse_matrix_line_factory<rowwise, symmetric, typename attrib<BaseRef>::minus_const> operation;
   typedef sparse_matrix_line_factory<rowwise, symmetric, typename attrib<BaseRef>::plus_const> const_operation;
};

template <bool rowwise, typename symmetric, typename Iterator1, typename Iterator2, typename Reference1, typename Reference2>
struct binary_op_builder< sparse_matrix_line_factory<rowwise,symmetric>, Iterator1, Iterator2, Reference1, Reference2>
   : empty_op_builder< sparse_matrix_line_factory<rowwise,symmetric,Reference1> > {};

template <typename E, typename symmetric>
class Rows< SparseMatrix<E,symmetric> >
   : public modified_container_pair_impl< Rows< SparseMatrix<E,symmetric> >,
                                          list( Container1< constant_value_container< SparseMatrix_base<E,symmetric>& > >,
                                                Container2< sequence >,
                                                Operation< pair< sparse_matrix_line_factory<true,symmetric>,
                                                                 BuildBinaryIt<operations::dereference2> > >,
                                                MasqueradedTop ) > {
protected:
   ~Rows();
public:
   constant_value_container< SparseMatrix_base<E,symmetric>& > get_container1()
   {
      return this->hidden();
   }
   const constant_value_container< const SparseMatrix_base<E,symmetric>& > get_container1() const
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

template <typename E, typename symmetric>
class Cols< SparseMatrix<E,symmetric> >
   : public modified_container_pair_impl< Cols< SparseMatrix<E,symmetric> >,
                                          list( Container1< constant_value_container< SparseMatrix_base<E,symmetric>& > >,
                                                Container2< sequence >,
                                                Operation< pair< sparse_matrix_line_factory<false,symmetric>,
                                                                 BuildBinaryIt<operations::dereference2> > >,
                                                MasqueradedTop ) > {
protected:
   ~Cols();
public:
   constant_value_container< SparseMatrix_base<E,symmetric>& > get_container1()
   {
      return this->hidden();
   }
   const constant_value_container< const SparseMatrix_base<E,symmetric>& > get_container1() const
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

template <typename Matrix, typename E, typename Permutation> inline
typename enable_if<SparseMatrix<E>,
                   (Matrix::is_nonsymmetric && Matrix::is_sparse)>::type
permuted_rows(const GenericMatrix<Matrix,E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_rows - dimension mismatch");
   }
   RestrictedSparseMatrix<E, sparse2d::only_rows> result(m.rows(), m.cols(), select(rows(m),perm).begin());
   return SparseMatrix<E>(result);
}

template <typename Matrix, typename E, typename Permutation> inline
typename enable_if<SparseMatrix<E>,
                   (Matrix::is_nonsymmetric && Matrix::is_sparse)>::type
permuted_cols(const GenericMatrix<Matrix,E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_cols - dimension mismatch");
   }
   RestrictedSparseMatrix<E, sparse2d::only_cols> result(m.rows(), m.cols(), select(cols(m),perm).begin());
   return SparseMatrix<E>(result);
}

template <typename Matrix, typename E, typename Permutation> inline
typename enable_if<SparseMatrix<E>,
                   (Matrix::is_nonsymmetric && Matrix::is_sparse)>::type
permuted_inv_rows(const GenericMatrix<Matrix,E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   RestrictedSparseMatrix<E, sparse2d::only_rows> result(m.rows(), m.cols());
   copy(entire(rows(m)), select(rows(result),perm).begin());
   return SparseMatrix<E>(result);
}

template <typename Matrix, typename E, typename Permutation> inline
typename enable_if<SparseMatrix<E>,
                   (Matrix::is_nonsymmetric && Matrix::is_sparse)>::type
permuted_inv_cols(const GenericMatrix<Matrix,E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_inv_cols - dimension mismatch");
   }
   RestrictedSparseMatrix<E, sparse2d::only_cols> result(m.rows(), m.cols());
   copy(entire(cols(m)), select(cols(result),perm).begin());
   return SparseMatrix<E>(result);
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type,
                   (!Matrix::is_nonsymmetric && Matrix::is_sparse)>::type
permuted_rows(const GenericMatrix<Matrix>& m, const Permutation& perm)
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
typename enable_if<typename Matrix::persistent_type,
                   (!Matrix::is_nonsymmetric && Matrix::is_sparse && container_traits<Permutation>::is_random)>::type
permuted_inv_rows(const GenericMatrix<Matrix>& m, const Permutation& inv_perm)
{
   if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
      if (m.rows() != inv_perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   std::vector<int> perm(m.rows());
   inverse_permutation(inv_perm,perm);
   return m.top().copy_permuted(perm,inv_perm);
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type,
                   (!Matrix::is_nonsymmetric && Matrix::is_sparse && !container_traits<Permutation>::is_random)>::type
permuted_inv_rows(const GenericMatrix<Matrix>& m, const Permutation& inv_perm)
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
typename enable_if<typename Matrix::persistent_type,
                   (!Matrix::is_nonsymmetric && Matrix::is_sparse)>::type
permuted_cols(const GenericMatrix<Matrix>& m, const Permutation& perm)
{
   return permuted_rows(m,perm);
}

template <typename Matrix, typename Permutation> inline
typename enable_if<typename Matrix::persistent_type,
                   (!Matrix::is_nonsymmetric && Matrix::is_sparse)>::type
permuted_inv_cols(const GenericMatrix<Matrix>& m, const Permutation& inv_perm)
{
   return permuted_inv_rows(m,inv_perm);
}

/// sparse matrix statistics collection
template <typename E>
class SparseMatrixStatistics {
public:
   unsigned int maxnon0s, maxrowsize, maxcolsize;
   E maxabs;

   SparseMatrixStatistics()
      : maxnon0s(0), maxrowsize(0), maxcolsize(0), maxabs(0) {}

   void gather(const SparseMatrix<E>& m)
   {
      unsigned int non0s=0;
      for (typename Entire< Rows< SparseMatrix<E> > >::const_iterator r=entire(rows(m)); !r.at_end(); ++r) {
         if (unsigned int s=r->size()) {
            for (typename Entire< typename SparseMatrix<E>::row_type >::const_iterator e=entire(*r); !e.at_end(); ++e)
               maxabs=std::max(maxabs, abs(*e));
            maxrowsize=std::max(maxrowsize, s);
            non0s+=s;
         }
      }
      maxnon0s=std::max(maxnon0s, non0s);
      for (typename Entire< Cols< SparseMatrix<E> > >::const_iterator c=entire(cols(m)); !c.at_end(); ++c) {
         if (unsigned int s=c->size()) {
            maxcolsize=std::max(maxcolsize, s);
         }
      }
   }

   void gather(const Transposed< SparseMatrix<E> >& m)
   {
      gather(m.hidden());
   }

   // statistics at two various moments can be glued together
   SparseMatrixStatistics& operator+= (const SparseMatrixStatistics& s)
   {
      maxnon0s=std::max(maxnon0s,s.maxnon0s);
      maxabs=std::max(maxabs,s.maxabs);
      maxrowsize=std::max(maxrowsize,s.maxrowsize);
      maxcolsize=std::max(maxcolsize,s.maxcolsize);
      return *this;
   }

   template <typename Traits> friend
   std::basic_ostream<char, Traits>&
   operator<< (std::basic_ostream<char, Traits>& os, const SparseMatrixStatistics& s)
   {
      return os << ">>> " << s.maxnon0s << " nonzeroes,  max abs(element)=" << s.maxabs
                << "\n>>> max row size=" << s.maxrowsize << ",  max col size=" << s.maxcolsize << endl;
   }
};

} // end namespace pm

namespace polymake {
   using pm::SparseMatrix;
   using pm::RestrictedSparseMatrix;
}

namespace std {
   template <typename E, typename symmetric> inline
   void swap(pm::SparseMatrix<E,symmetric>& M1, pm::SparseMatrix<E,symmetric>& M2)
   {
      M1.swap(M2);
   }

   template <typename E, pm::sparse2d::restriction_kind restriction> inline
   void swap(pm::RestrictedSparseMatrix<E,restriction>& M1,
             pm::RestrictedSparseMatrix<E,restriction>& M2)
   {
      M1.swap(M2);
   }

   template <typename Tree, typename symmetric> inline
   void swap(pm::sparse_matrix_line<Tree&,symmetric> l1, pm::sparse_matrix_line<Tree&,symmetric> l2)
   {
      l1.swap(l2);
   }
}

#endif // POLYMAKE_SPARSE_MATRIX_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
