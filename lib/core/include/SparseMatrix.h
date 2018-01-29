/* Copyright (c) 1997-2018
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
#include "polymake/Array.h"

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
   : mlist_concat< typename sparse2d::line_params<TreeRef>::type,
                   OperationTag< skew_negator<> > > {
public:
   operations::identity<int> get_operation() const
   {
      return operations::identity<int>();
   }
};

template <typename TreeRef, typename TSymmetric>
class sparse_matrix_line_ops
   : public modified_tree< sparse_matrix_line<TreeRef, TSymmetric>,
                           typename sparse_matrix_line_params<TreeRef, TSymmetric>::type >,
     public GenericVector< sparse_matrix_line<TreeRef, TSymmetric>,
                           typename deref<TreeRef>::type::mapped_type> {};

template <typename TreeRef>
class sparse_matrix_line_ops<TreeRef, SkewSymmetric>
   : public modified_tree< sparse_matrix_line<TreeRef, SkewSymmetric>,
                           typename sparse_matrix_line_params<TreeRef, SkewSymmetric>::type >,
     public GenericVector< sparse_matrix_line<TreeRef, SkewSymmetric>,
                           typename deref<TreeRef>::type::mapped_type> {

   typedef modified_tree< sparse_matrix_line<TreeRef, SkewSymmetric>,
                          typename sparse_matrix_line_params<TreeRef, SkewSymmetric>::type > base_t;
public:
   typedef typename deref<TreeRef>::type::mapped_type value_type;

   skew_negator<> get_operation() const
   {
      return this->top().index();
   }

protected:
   template <typename TVector>
   void assign(const TVector& v)
   {
      assign_sparse(this->top().get_container(), attach_operation(ensure(v, (pure_sparse*)0), get_operation()).begin());
   }

   template <typename Operation>
   void assign_op(const Operation& op)
   {
      perform_assign(entire(this->top().get_container()), op);
   }

   template <typename TVector, typename Operation>
   typename std::enable_if<!operations::is_partially_defined_for<Operation, sparse_matrix_line_ops, TVector>::value, void>::type
   assign_op(const TVector& v, const Operation& op)
   {
      perform_assign(entire(this->top().get_container()), v.begin(), op);
   }

   template <typename TVector, typename Operation>
   typename std::enable_if<operations::is_partially_defined_for<Operation, sparse_matrix_line_ops, TVector>::value, void>::type
   assign_op(const TVector& v, const Operation& op)
   {
      perform_assign_sparse(this->top().get_container(), attach_operation(ensure(v, (pure_sparse*)0), get_operation()).begin(), op);
   }

   void fill_impl(typename function_argument<value_type>::type x, pure_sparse)
   {
      if (x)
         fill_sparse(this->top().get_container(), attach_operation(ensure(constant(x), (indexed*)0), get_operation()).begin());
      else
         this->clear();
   }

public:
   typename base_t::iterator insert(int i, const value_type& x)
   {
      operations::neg<const value_type&> op;
      return base_t::insert(i, i > this->top().index() ? op(x) : x);
   }

   typename base_t::iterator insert(const typename base_t::iterator& pos, int i)
   {
      return base_t::insert(pos, i);
   }

   typename base_t::iterator insert(const typename base_t::iterator& pos, int i, const value_type& x)
   {
      operations::neg<const value_type&> op;
      return base_t::insert(pos, i, i > this->top().index() ? op(x) : x);
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
   typedef sparse_matrix_line_base<TreeRef, symmetric> base_t;

   friend class GenericVector<sparse_matrix_line>;
   template <typename,typename> friend class SparseMatrix;
   template <typename,typename> friend class GenericMatrix;

public:
   sparse_matrix_line(typename base_t::first_arg_type arg1, typename base_t::second_arg_type arg2)
      : base_t(arg1, arg2) {}

   static const bool is_skew_symmetric=std::is_same<symmetric, SkewSymmetric>::value;
   typedef typename std::conditional<std::is_same<symmetric, NonSymmetric>::value, nothing, symmetric>::type operate_on_lower;

protected:
   using base_t::assign_op;

   template <typename TVector, typename Operation>
   typename std::enable_if<!operations::is_partially_defined_for<Operation, sparse_matrix_line, TVector>::value, void>::type
   assign_op(const TVector& v, const Operation& op, operate_on_lower)
   {
      perform_assign(entire(sparse2d::select_lower_triangle(this->get_container())), v.begin(), op);
   }

   template <typename TVector, typename Operation>
   typename std::enable_if<operations::is_partially_defined_for<Operation, sparse_matrix_line, TVector>::value, void>::type
   assign_op(const TVector& v, const Operation& op, operate_on_lower)
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
   typedef typename std::conditional<is_skew_symmetric, const value_type, const value_type&>::type const_reference;
   typedef typename std::conditional<attrib<TreeRef>::is_const,
                                     const_reference, sparse_elem_proxy<proxy_base, value_type, symmetric>>::type
      reference;
   typedef random_access_iterator_tag container_category;

   const_reference operator[] (int i) const
   {
      return deref_sparse_iterator(this->find(i));
   }

protected:
   reference random_impl(int i, std::false_type) { return proxy_base(this->get_container(),i); }
   reference random_impl(int i, std::true_type) const { return operator[](i); }
public:
   reference operator[] (int i)
   {
      return random_impl(i, bool_constant<attrib<TreeRef>::is_const>());
   }

   int dim() const { return this->get_container().dim(); }

protected:
   typedef typename std::conditional<std::is_same<symmetric, NonSymmetric>::value, maximal<int>, int>::type input_limit_type;

   maximal<int> _get_input_limit(type2type< maximal<int> >) const { return maximal<int>(); }

   int _get_input_limit(type2type<int>) const { return this->index(); }

   friend
   input_limit_type get_input_limit(sparse_matrix_line& me)
   {
      return me._get_input_limit(type2type<input_limit_type>());
   }
};

template <typename TreeRef, typename symmetric>
struct check_container_feature<sparse_matrix_line<TreeRef,symmetric>, pure_sparse> : std::true_type {};

template <typename TreeRef, typename symmetric>
struct spec_object_traits< sparse_matrix_line<TreeRef,symmetric> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=attrib<TreeRef>::is_reference,
                     is_always_const=attrib<TreeRef>::is_const;
   typedef typename std::conditional<is_temporary, void, typename deref<TreeRef>::type>::type masquerade_for;
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

   template <typename Iterator, typename TLines>
   static
   void copy_linewise(Iterator&& src, TLines& lines, std::true_type)
   {
      copy_range(std::forward<Iterator>(src), entire(lines));
   }

   template <typename Iterator, typename TLines>
   void copy_linewise(Iterator&& src, TLines& lines, std::false_type)
   {
      for (int i=0; !src.at_end(); ++src, ++i)
         append(lines, *src, i);
   }

   template <typename TLines, typename TVector>
   void append(TLines& lines, const TVector& vec, int i)
   {
      for (auto v=ensure(vec, (sparse_compatible*)0).begin(); !v.at_end(); ++v)
         lines[v.index()].push_back(i, *v);
   }

   typedef sparse_matrix_line<typename table_type::primary_tree_type, NonSymmetric> line_t;
public:
   typedef E value_type;
   typedef typename line_t::reference reference;
   typedef const E& const_reference;

   explicit RestrictedSparseMatrix(int n=0) : data(n) {}

   RestrictedSparseMatrix(int r, int c) : data(r,c) {}

   template <typename Iterator, typename Dir,
             typename=typename std::enable_if<is_among<Dir, sparse2d::rowwise, sparse2d::columnwise>::value &&
                                              assess_iterator_value<Iterator, can_initialize, Vector<E>>::value &&
                                              (Dir::value==restriction || assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value)>::type>
   RestrictedSparseMatrix(int n, Dir, Iterator&& src)
      : data(n)
   {
      copy_linewise(ensure_private_mutable(std::forward<Iterator>(src)), lines(*this, sparse2d::restriction_const<restriction>()),
                    bool_constant<Dir::value==restriction>());
   }

   template <typename Iterator, typename Dir,
             typename=typename std::enable_if<is_among<Dir, sparse2d::rowwise, sparse2d::columnwise>::value &&
                                              assess_iterator_value<Iterator, can_initialize, Vector<E>>::value &&
                                              (Dir::value==restriction || assess_iterator<Iterator, check_iterator_feature, end_sensitive>::value)>::type>
   RestrictedSparseMatrix(int r, int c, Dir, Iterator&& src)
      : data(r, c)
   {
      copy_linewise(ensure_private_mutable(std::forward<Iterator>(src)), lines(*this, sparse2d::restriction_const<restriction>()),
                    bool_constant<Dir::value==restriction>());
   }

   RestrictedSparseMatrix(RestrictedSparseMatrix&& M)
      : data(std::move(M.data)) {}

   template <typename Container, typename=typename std::enable_if<isomorphic_to_container_of<Container, Vector<E>, allow_conversion>::value &&
                                                                  restriction==sparse2d::only_rows>::type>
   RestrictedSparseMatrix(const Container& src)
      : data(src.size())
   {
      copy_linewise(src.begin(), pm::rows(*this), std::true_type());
   }

   void swap(RestrictedSparseMatrix& M)
   {
      data.swap(M.data);
   }

   void clear() { data.clear(); }

protected:
   reference random_impl(int i, int j, std::false_type)
   {
      return this->row(i)[j];
   }
   reference random_impl(int i, int j, std::true_type)
   {
      return this->col(j)[i];
   }
   const_reference random_impl(int i, int j, std::false_type) const
   {
      return this->row(i)[j];
   }
   const_reference random_impl(int i, int j, std::true_type) const
   {
      return this->col(j)[i];
   }
public:
   reference operator() (int i, int j)
   {
      return random_impl(i, j, bool_constant<restriction==sparse2d::only_cols>());
   }
   const_reference operator() (int i, int j) const
   {
      return random_impl(i, j, bool_constant<restriction==sparse2d::only_cols>());
   }

private:
   template <typename Iterator>
   void append_rows_impl(int n, Iterator src, std::true_type)
   {
      int oldrows=data.rows();
      data.resize_rows(oldrows+n);
      for (auto dst=pm::rows(*this).begin()+oldrows;  n>0;  ++src, ++dst, --n)
         *dst=*src;
   }

   template <typename Iterator>
   void append_rows_impl(int n, Iterator src, std::false_type)
   {
      for (int r=data.rows(); n>0; ++src, ++r, --n)
         append(pm::cols(*this), *src, r);
   }

   template <typename Iterator>
   void append_cols_impl(int n, Iterator src, std::true_type)
   {
      int oldcols=data.cols();
      data.resize_cols(oldcols+n);
      for (auto dst=pm::cols(*this).begin()+oldcols;  n>0;  ++src, ++dst, --n)
         *dst=*src;
   }

   template <typename Iterator>
   void append_cols_impl(int n, Iterator src, std::false_type)
   {
      for (int c=data.cols();  n>0;  ++src, ++c, --n)
         append(pm::rows(*this), *src, c);
   }

public:
   template <typename Matrix>
   RestrictedSparseMatrix& operator/= (const GenericMatrix<Matrix>& m)
   {
      append_rows_impl(m.rows(), pm::rows(m).begin(), bool_constant<restriction==sparse2d::only_rows>());
      return *this;
   }

   template <typename Vector>
   RestrictedSparseMatrix& operator/= (const GenericVector<Vector>& v)
   {
      append_rows_impl(1, &v.top(), bool_constant<restriction==sparse2d::only_rows>());
      return *this;
   }

   template <typename Matrix>
   RestrictedSparseMatrix& operator|= (const GenericMatrix<Matrix>& m)
   {
      append_cols_impl(m.cols(), pm::cols(m).begin(), bool_constant<restriction==sparse2d::only_cols>());
      return *this;
   }

   template <typename Vector>
   RestrictedSparseMatrix& operator|= (const GenericVector<Vector>& v)
   {
      append_cols_impl(1, &v.top(), bool_constant<restriction==sparse2d::only_cols>());
      return *this;
   }

   void squeeze() { data.squeeze(); }

   template <typename TPerm>
   typename std::enable_if<isomorphic_to_container_of<TPerm, int>::value>::type
   permute_rows(const TPerm& perm)
   {
      data.permute_rows(perm, std::false_type());
   }

   template <typename TPerm>
   typename std::enable_if<isomorphic_to_container_of<TPerm, int>::value>::type
   permute_cols(const TPerm& perm)
   {
      data.permute_cols(perm, std::false_type());
   }

   template <typename TInvPerm>
   typename std::enable_if<isomorphic_to_container_of<TInvPerm, int>::value>::type
   permute_inv_rows(const TInvPerm& inv_perm)
   {
      data.permute_rows(inv_perm, std::true_type());
   }

   template <typename TInvPerm>
   typename std::enable_if<isomorphic_to_container_of<TInvPerm, int>::value>::type
   permute_inv_cols(const TInvPerm& inv_perm)
   {
      data.permute_cols(inv_perm, std::true_type());
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

   typedef typename std::conditional<restriction==sparse2d::only_rows,
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
class Rows< RestrictedSparseMatrix<E, restriction> >
   : public sparse2d::Rows< RestrictedSparseMatrix<E, restriction>, E, false, restriction,
                            operations::masquerade2<sparse_matrix_line, NonSymmetric> > {
protected:
   ~Rows();
public:
   typedef typename std::conditional<restriction==sparse2d::only_rows, random_access_iterator_tag, output_iterator_tag>::type
      container_category;
};

template <typename E, sparse2d::restriction_kind restriction>
class Cols< RestrictedSparseMatrix<E, restriction> >
   : public sparse2d::Cols< RestrictedSparseMatrix<E, restriction>, E, false, restriction,
                            operations::masquerade2<sparse_matrix_line, NonSymmetric> > {
protected:
   ~Cols();
public:
   typedef typename std::conditional<restriction==sparse2d::only_cols, random_access_iterator_tag, output_iterator_tag>::type
      container_category;
};

template <typename E, typename symmetric>
class SparseMatrix_base {
protected:
   typedef sparse2d::Table<E, symmetric::value> table_type;
   shared_object<table_type, AliasHandlerTag<shared_alias_handler>> data;

   table_type& get_table() { return *data; }
   const table_type& get_table() const { return *data; }

   friend SparseMatrix_base& make_mutable_alias(SparseMatrix_base& alias, SparseMatrix_base& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }

   SparseMatrix_base() = default;

   SparseMatrix_base(int r, int c)
      : data(r, c) {}

   template <sparse2d::restriction_kind restriction>
   SparseMatrix_base(sparse2d::Table<E, symmetric::value, restriction>&& input_data)
      : data(std::move(input_data)) {}

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
   : public SparseMatrix_base<E, symmetric>
   , public GenericMatrix< SparseMatrix<E,symmetric>, E> {
protected:
   typedef SparseMatrix_base<E, symmetric> base_t;
   friend SparseMatrix& make_mutable_alias(SparseMatrix& alias, SparseMatrix& owner)
   {
      return static_cast<SparseMatrix&>(make_mutable_alias(static_cast<base_t&>(alias), static_cast<base_t&>(owner)));
   }

   // elementwise, non-symmetric
   template <typename Iterator>
   void init_impl(Iterator&& src_elem, std::true_type, std::false_type)
   {
      auto&& src=make_converting_iterator<E>(std::forward<Iterator>(src_elem));
      const int n=this->cols();
      for (auto r_i=entire(pm::rows(static_cast<base_t&>(*this))); !r_i.at_end(); ++r_i)
         for (int i=0; i<n; ++i, ++src)
            if (!is_zero(*src))
               r_i->push_back(i, *src);
   }

   // elementwise, symmetric
   template <typename Iterator>
   void init_impl(Iterator&& src_elem, std::true_type, std::true_type)
   {
      auto&& src=make_converting_iterator<E>(std::forward<Iterator>(src_elem));
      const int n=this->cols();
      int d=0;
      for (auto r_i=entire(pm::rows(static_cast<base_t&>(*this))); !r_i.at_end(); ++r_i) {
         for (int i=0; i<=d; ++i, ++src)
            if (!is_zero(*src))
               r_i->push_back(i, *src);
         ++d; std::advance(src, n-d);
      }
   }

   // rowwise, non-symmetric
   template <typename Iterator>
   void init_impl(Iterator&& src_rows, std::false_type, std::false_type)
   {
      for (auto r_i=entire(pm::rows(static_cast<base_t&>(*this))); !r_i.at_end(); ++r_i, ++src_rows)
         *r_i = convert_lazily<E>(*src_rows);
   }

   // rowwise, symmetric
   template <typename Iterator>
   void init_impl(Iterator&& src_rows, std::false_type, std::true_type)
   {
      int d=0;
      for (auto r_i=entire(pm::rows(static_cast<base_t&>(*this))); !r_i.at_end(); ++r_i, ++d, ++src_rows) {
         int i;
         for (auto src=make_converting_iterator<E>(ensure(*src_rows, (pure_sparse*)0).begin()); !src.at_end() && (i=src.index())<=d; ++src)
            r_i->push_back(i, *src);
      }
   }

   typedef sparse_matrix_line<typename base_t::table_type::primary_tree_type, symmetric> line_t;
public:
   typedef typename std::conditional<symmetric::value, void, RestrictedSparseMatrix<E>>::type unknown_columns_type;
   typedef E value_type;
   typedef typename line_t::reference reference;
   typedef typename line_t::const_reference const_reference;

   /// create as empty
   SparseMatrix() {}

   /// Create a matrix with r rows and c columns, (implicitly) initialize all elements to 0. 
   SparseMatrix(int r, int c)
      : base_t(r, c) {}

   /**
     Create a matrix with r rows and c columns, initialize the elements from a data sequence. 
     src should iterate either over r*c scalar values, corresponding to the elements in the row order 
     (the column index changes first,) or over r vectors of dimension c, corresponding to the matrix rows. 
     Zero input elements are filtered out. 
   */
   template <typename Iterator>
   SparseMatrix(int r, int c, Iterator&& src)
      : base_t(r, c)
   {
      init_impl(ensure_private_mutable(std::forward<Iterator>(src)),
                bool_constant<object_traits<typename iterator_traits<Iterator>::value_type>::total_dimension == object_traits<E>::total_dimension>(),
                symmetric());
   }

   /// Copy of a disguised Matrix object. 
   SparseMatrix(const GenericMatrix<SparseMatrix>& M)
      : base_t(M.top()) {}

   /// Copy of an abstract matrix of the same element type. 
   template <typename TMatrix2>
   SparseMatrix(const GenericMatrix<TMatrix2, E>& M,
                typename std::enable_if<SparseMatrix::template compatible_symmetry_types<TMatrix2>(), void**>::type=nullptr)
      : base_t(M.rows(), M.cols())
   {
      init_impl(pm::rows(M).begin(), std::false_type(), symmetric());
   }

   /// Copy of an abstract matrix with element conversion. 
   template <typename TMatrix2, typename E2>
   explicit SparseMatrix(const GenericMatrix<TMatrix2, E2>& M,
                         typename std::enable_if<(SparseMatrix::template compatible_symmetry_types<TMatrix2>() &&
                                                  can_initialize<E2, E>::value), void**>::type=nullptr)
      : base_t(M.rows(), M.cols())
   {
      init_impl(pm::rows(M).begin(), std::false_type(), symmetric());
   }

   template <sparse2d::restriction_kind restriction, typename enabled=typename std::enable_if<!symmetric::value && restriction!=sparse2d::full>::type>
   explicit SparseMatrix(RestrictedSparseMatrix<E, restriction>&& M)
      : base_t(std::move(M.data)) {}

   template <typename Container>
   SparseMatrix(const Container& src,
                typename std::enable_if<(isomorphic_to_container_of<Container, Vector<E>, allow_conversion>::value &&
                                         !symmetric::value), void**>::type=nullptr)
      : base_t(src.size(), src.empty() ? 0 : get_dim(src.front()))
   {
      init_impl(src.begin(), std::false_type(), symmetric());
   }

   /// Persistent matrix objects have after the assignment the same dimensions as the right hand side operand. 
   /// Alias objects, such as matrix minor or block matrix, cannot be resized, thus must have the same dimensions as on the right hand side.
   SparseMatrix& operator= (const SparseMatrix& other) { assign(other); return *this; }
   using SparseMatrix::generic_type::operator=;

   template <sparse2d::restriction_kind restriction, typename enabled=typename std::enable_if<!symmetric::value && restriction!=sparse2d::full>::type>
   SparseMatrix& operator= (RestrictedSparseMatrix<E, restriction>&& M)
   {
      this->data.replace(std::move(M.data));
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

   void clear(int r, int c) { this->data.apply(typename base_t::table_type::shared_clear(r,c)); }

   reference operator() (int i, int j)
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>this->rows() || j<0 || j>= this->cols())
            throw std::runtime_error("SparseMatrix::operator() - index out of range");
      }
      return pm::rows(static_cast<base_t&>(*this))[i][j];
   }

   const_reference operator() (int i, int j) const
   {
      if (POLYMAKE_DEBUG) {
         if (i<0 || i>this->rows() || j<0 || j>= this->cols())
            throw std::runtime_error("SparseMatrix::operator() - index out of range");
      }
      return pm::rows(static_cast<const base_t&>(*this))[i][j];
   }

   /// Physically remove all zero elements that might have creeped in by some previous operation. 
   void remove0s()
   {
      for (auto r=entire(pm::rows(static_cast<base_t&>(*this))); !r.at_end(); ++r)
         r->remove0s();
   }

   template <typename row_number_consumer, typename col_number_consumer>
   void squeeze(const row_number_consumer& rnc, const col_number_consumer& cnc) { this->data->squeeze(rnc,cnc); }

   template <typename row_number_consumer>
   void squeeze(const row_number_consumer& rnc) { this->data->squeeze(rnc); }

   /// Remove all empty (i.e., consisting entirely of implicit zeroes,) rows, renumber the rest, and reduce the dimensions.
   void squeeze() { this->data->squeeze(); }

   template <typename row_number_consumer>
   void squeeze_rows(const row_number_consumer& rnc) { this->data->squeeze_rows(rnc); }

   void squeeze_rows() { this->data->squeeze_rows(); }

   template <typename col_number_consumer>
   void squeeze_cols(const col_number_consumer& cnc) { this->data->squeeze_cols(cnc); }

   /// Remove all empty (i.e., consisting entirely of implicit zeroes,) columns, renumber the rest, and reduce the dimensions.
   void squeeze_cols() { this->data->squeeze_cols(); }

   /// Permute the rows of the matrix without copying the elements.
   /// These operations are nevertheless expensive, as they need to visit each element and adjust its indices.
   template <typename TPerm>
   typename std::enable_if<isomorphic_to_container_of<TPerm, int>::value>::type
   permute_rows(const TPerm& perm)
   {
      this->data->permute_rows(perm, std::false_type());
   }

   /// Permute the columns of the matrix without copying the elements.
   /// These operations are nevetherless expensive, as they need to visit each element and adjust its indices.
   template <typename TPerm>
   typename std::enable_if<isomorphic_to_container_of<TPerm, int>::value>::type
   permute_cols(const TPerm& perm)
   {
      this->data->permute_cols(perm, std::false_type());
   }

   template <typename TInvPerm>
   typename std::enable_if<isomorphic_to_container_of<TInvPerm, int>::value>::type
   permute_inv_rows(const TInvPerm& inv_perm)
   {
      this->data->permute_rows(inv_perm, std::true_type());
   }

   template <typename TInvPerm>
   typename std::enable_if<isomorphic_to_container_of<TInvPerm, int>::value>::type
   permute_inv_cols(const TInvPerm& inv_perm)
   {
      this->data->permute_cols(inv_perm, std::true_type());
   }

   template <typename Perm, typename InvPerm>
   SparseMatrix copy_permuted(const Perm& perm, const InvPerm& inv_perm,
                              typename std::enable_if<symmetric::value, mlist<Perm>*>::type=nullptr) const
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
      this->data.apply(typename base_t::table_type::shared_add_rows(m.rows()));
      copy_range(entire(pm::rows(m)), pm::rows(static_cast<base_t&>(*this)).begin()+old_rows);
   }

   template <typename Vector2>
   void append_row(const Vector2& v)
   {
      const int old_rows=this->rows();
      this->data.apply(typename base_t::table_type::shared_add_rows(1));
      this->row(old_rows)=v;
   }

   template <typename Matrix2>
   void append_cols(const Matrix2& m)
   {
      const int old_cols=this->cols();
      this->data.apply(typename base_t::table_type::shared_add_cols(m.cols()));
      copy_range(entire(pm::cols(m)), pm::cols(static_cast<base_t&>(*this)).begin()+old_cols);
   }

   template <typename Vector2>
   void append_col(const Vector2& v)
   {
      const int old_cols=this->cols();
      this->data.apply(typename base_t::table_type::shared_add_cols(1));
      this->col(old_cols)=v;
   }

   template <typename E2>
   void fill_impl(const E2& x, std::false_type)
   {
      if (this->data.is_shared())
         clear(this->rows(), this->cols());
      if (!is_zero(x))
         SparseMatrix::generic_type::fill_impl(x, std::false_type());
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
struct check_container_feature< SparseMatrix<E,symmetric>, pure_sparse > : std::true_type {};

template <typename E, typename symmetric>
struct check_container_feature< SparseMatrix<E,symmetric>, Symmetric > : std::is_same<symmetric, Symmetric> {};

template <typename E, typename symmetric>
struct check_container_feature< SparseMatrix<E,symmetric>, SkewSymmetric > : std::is_same<symmetric,SkewSymmetric> {};

template <bool rowwise, typename symmetric, typename BaseRef>
class sparse_matrix_line_factory {
public:
   typedef BaseRef first_argument_type;
   typedef int second_argument_type;
   typedef typename std::conditional<rowwise, typename deref<BaseRef>::type::table_type::row_tree_type,
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

template <typename E, typename TSymmetric>
class Rows< SparseMatrix<E, TSymmetric> >
   : public modified_container_pair_impl< Rows< SparseMatrix<E, TSymmetric> >,
                                          mlist< Container1Tag< constant_value_container< SparseMatrix_base<E, TSymmetric>& > >,
                                                 Container2Tag< sequence >,
                                                 OperationTag< pair< sparse_matrix_line_factory<true, TSymmetric>,
                                                                     BuildBinaryIt<operations::dereference2> > >,
                                                 MasqueradedTop > > {
protected:
   ~Rows();
public:
   constant_value_container< SparseMatrix_base<E, TSymmetric>& > get_container1()
   {
      return this->hidden();
   }
   const constant_value_container< const SparseMatrix_base<E, TSymmetric>& > get_container1() const
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

template <typename E, typename TSymmetric>
class Cols< SparseMatrix<E, TSymmetric> >
   : public modified_container_pair_impl< Cols< SparseMatrix<E, TSymmetric> >,
                                          mlist< Container1Tag< constant_value_container< SparseMatrix_base<E, TSymmetric>& > >,
                                                 Container2Tag< sequence >,
                                                 OperationTag< pair< sparse_matrix_line_factory<false, TSymmetric>,
                                                                     BuildBinaryIt<operations::dereference2> > >,
                                                 MasqueradedTop > > {
protected:
   ~Cols();
public:
   constant_value_container< SparseMatrix_base<E, TSymmetric>& > get_container1()
   {
      return this->hidden();
   }
   const constant_value_container< const SparseMatrix_base<E, TSymmetric>& > get_container1() const
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

template <typename TMatrix, typename E, typename Permutation> inline
typename std::enable_if<TMatrix::is_nonsymmetric && TMatrix::is_sparse, SparseMatrix<E>>::type
permuted_rows(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_rows - dimension mismatch");
   }
   return SparseMatrix<E>(RestrictedSparseMatrix<E, sparse2d::only_rows>(m.rows(), m.cols(), sparse2d::rowwise(), select(rows(m),perm).begin()));
}

template <typename TMatrix, typename E, typename Permutation> inline
typename std::enable_if<TMatrix::is_nonsymmetric && TMatrix::is_sparse, SparseMatrix<E>>::type
permuted_cols(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix>::value) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_cols - dimension mismatch");
   }
   return SparseMatrix<E>(RestrictedSparseMatrix<E, sparse2d::only_cols>(m.rows(), m.cols(), sparse2d::columnwise(), select(cols(m),perm).begin()));
}

template <typename TMatrix, typename E, typename Permutation> inline
typename std::enable_if<TMatrix::is_nonsymmetric && TMatrix::is_sparse, SparseMatrix<E>>::type
permuted_inv_rows(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   RestrictedSparseMatrix<E, sparse2d::only_rows> result(m.rows(), m.cols());
   copy_range(entire(rows(m)), select(rows(result),perm).begin());
   return SparseMatrix<E>(std::move(result));
}

template <typename TMatrix, typename E, typename Permutation> inline
typename std::enable_if<TMatrix::is_nonsymmetric && TMatrix::is_sparse, SparseMatrix<E>>::type
permuted_inv_cols(const GenericMatrix<TMatrix, E>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix>::value) {
      if (m.cols() != perm.size())
         throw std::runtime_error("permuted_inv_cols - dimension mismatch");
   }
   RestrictedSparseMatrix<E, sparse2d::only_cols> result(m.rows(), m.cols());
   copy_range(entire(cols(m)), select(cols(result),perm).begin());
   return SparseMatrix<E>(std::move(result));
}

template <typename TMatrix, typename Permutation> inline
typename std::enable_if<!TMatrix::is_nonsymmetric && TMatrix::is_sparse, typename TMatrix::persistent_type>::type
permuted_rows(const GenericMatrix<TMatrix>& m, const Permutation& perm)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix>::value) {
      if (m.rows() != perm.size())
         throw std::runtime_error("permuted_rows - dimension mismatch");
   }
   std::vector<int> inv_perm(m.rows());
   inverse_permutation(perm,inv_perm);
   return m.top().copy_permuted(perm,inv_perm);
}

template <typename TMatrix, typename Permutation> inline
typename std::enable_if<!TMatrix::is_nonsymmetric && TMatrix::is_sparse && container_traits<Permutation>::is_random,
                        typename TMatrix::persistent_type>::type
permuted_inv_rows(const GenericMatrix<TMatrix>& m, const Permutation& inv_perm)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix>::value) {
      if (m.rows() != inv_perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   std::vector<int> perm(m.rows());
   inverse_permutation(inv_perm,perm);
   return m.top().copy_permuted(perm,inv_perm);
}

template <typename TMatrix, typename Permutation> inline
typename std::enable_if<!TMatrix::is_nonsymmetric && TMatrix::is_sparse && !container_traits<Permutation>::is_random,
                        typename TMatrix::persistent_type>::type
permuted_inv_rows(const GenericMatrix<TMatrix>& m, const Permutation& inv_perm)
{
   if (POLYMAKE_DEBUG || !Unwary<TMatrix>::value) {
      if (m.rows() != inv_perm.size())
         throw std::runtime_error("permuted_inv_rows - dimension mismatch");
   }
   std::vector<int> inv_perm_copy(inv_perm.size());
   copy_range(entire(inv_perm), inv_perm_copy.begin());
   return permuted_inv_rows(m,inv_perm_copy);
}

template <typename TMatrix, typename Permutation> inline
typename std::enable_if<!TMatrix::is_nonsymmetric && TMatrix::is_sparse,
                        typename TMatrix::persistent_type>::type
permuted_cols(const GenericMatrix<TMatrix>& m, const Permutation& perm)
{
   return permuted_rows(m,perm);
}

template <typename TMatrix, typename Permutation> inline
typename std::enable_if<!TMatrix::is_nonsymmetric && TMatrix::is_sparse,
                        typename TMatrix::persistent_type>::type
permuted_inv_cols(const GenericMatrix<TMatrix>& m, const Permutation& inv_perm)
{
   return permuted_inv_rows(m,inv_perm);
}

/// sparse matrix statistics collection
template <typename E>
class SparseMatrixStatistics {
public:
   unsigned int maxnon0s, maxrowsize, maxcolsize;
   E maxabs;
   Array<unsigned int> row_support_sizes;

   SparseMatrixStatistics()
      : maxnon0s(0), maxrowsize(0), maxcolsize(0), maxabs(0) {}

   void gather(const SparseMatrix<E>& m)
   {
      unsigned int non0s=0;

      int row_ct(0);
      row_support_sizes = Array<unsigned int>(m.rows());
      for (typename Entire< Rows< SparseMatrix<E> > >::const_iterator r=entire(rows(m)); !r.at_end(); ++r, ++row_ct) {
         if (unsigned int s=r->size()) {
            for (typename Entire< typename SparseMatrix<E>::row_type >::const_iterator e=entire(*r); !e.at_end(); ++e) {
               maxabs=std::max(maxabs, abs(*e));
            }
            maxrowsize=std::max(maxrowsize, s);
            non0s+=s;
            row_support_sizes[row_ct] = s;
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
      // FIXME: also take component-wise max of the row_support_sizes
      return *this;
   }

   template <typename Traits> friend
   std::basic_ostream<char, Traits>&
   operator<< (std::basic_ostream<char, Traits>& os, const SparseMatrixStatistics& s)
   {
      wrap(os) << ">>> " << s.maxnon0s << " nonzeroes,  max abs(element)=" << s.maxabs
               << "\n>>> max row size=" << s.maxrowsize << ",  max col size=" << s.maxcolsize 
               << "\n>>> row support sizes=" << s.row_support_sizes
               << endl;
      return os;
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
