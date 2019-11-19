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

#ifndef POLYMAKE_INTERNAL_MATRIX_METHODS_H
#define POLYMAKE_INTERNAL_MATRIX_METHODS_H

#include "polymake/IndexedSubset.h"
#include "polymake/ContainerChain.h"
#include "polymake/internal/matrix_rows_cols.h"
#include <stdexcept>

// OSF/1 defines this name to something odd that we don't need at all
#ifdef minor
#undef minor
#endif

namespace pm {

struct Symmetric : std::true_type {};
struct NonSymmetric : std::false_type {};
struct SkewSymmetric : Symmetric {};
struct FlatStorage {};

template <typename TMatrix>
struct default_check_container_feature<TMatrix, Symmetric> : std::false_type {};

template <typename TMatrix>
struct default_check_container_feature<TMatrix, SkewSymmetric> : std::false_type {};

template <typename TMatrix>
struct default_check_container_feature<TMatrix, FlatStorage> : std::false_type {};

template <typename TMatrix>
struct default_check_container_feature<TMatrix, NonSymmetric>
   : bool_not< mlist_or< check_container_feature<TMatrix, Symmetric>,
                         check_container_feature<TMatrix, SkewSymmetric> > > {};

template <typename TMatrix>
struct matrix_symmetry_type
   : mselect< std::enable_if<check_container_feature<TMatrix, Symmetric>::value, Symmetric>,
              std::enable_if<check_container_feature<TMatrix, SkewSymmetric>::value, SkewSymmetric>,
              NonSymmetric > {};


template <typename TMatrix>
int empty_rows(const TMatrix& m)
{
   int cnt=0;
   for (auto r=entire(rows(m)); !r.at_end(); ++r)
      if (!r->size()) ++cnt;
   return cnt;
}

template <typename TMatrix>
int empty_cols(const TMatrix& m)
{
   int cnt=0;
   for (auto c=entire(cols(m)); !c.at_end(); ++c)
      if (!c->size()) ++cnt;
   return cnt;
}

template <typename E>
int count_columns(const std::initializer_list<std::initializer_list<E>>& l)
{
   if (l.size()==0) return 0;
#if POLYMAKE_DEBUG
   auto r=l.begin(), e=l.end();
   const size_t c=r->size();
   while (++r != e) {
      if (r->size() != c)
         throw std::runtime_error("Matrix initializer list does not have a rectangular shape");
   }
   return c;
#else
   return l.begin()->size();
#endif
}

/* -------------------------------
 *  Matrix masquerade: Transposed
 * ------------------------------- */

template <typename TMatrix>
class Transposed
   : public inherit_generic<Transposed<TMatrix>, TMatrix>::type {
protected:
   ~Transposed();
public:
   typedef typename TMatrix::value_type value_type;
   typedef typename TMatrix::reference reference;
   typedef typename TMatrix::const_reference const_reference;

   TMatrix& hidden() { return reinterpret_cast<TMatrix&>(*this); }
   const TMatrix& hidden() const { return reinterpret_cast<const TMatrix&>(*this); }

   void clear() { hidden().clear(); }
   void clear(int r, int c) { hidden().clear(c,r); }
};

template <typename TMatrix>
struct spec_object_traits< Transposed<TMatrix> >
   : spec_object_traits<is_container> {
   typedef TMatrix masquerade_for;
   static const bool is_lazy         = object_traits<TMatrix>::is_lazy,
                     is_always_const = object_traits<TMatrix>::is_always_const;
   static const int is_resizeable= object_traits<TMatrix>::is_resizeable;
};

template <typename TMatrix>
struct check_container_feature<Transposed<TMatrix>, sparse>
   : check_container_feature<TMatrix, sparse> {};

template <typename TMatrix>
struct check_container_feature<Transposed<TMatrix>, pure_sparse>
   : check_container_feature<TMatrix, pure_sparse> {};

template <typename TMatrix>
decltype(auto) T(TMatrix&& m)
{
   return reinterpret_cast<typename inherit_ref_norv<Transposed<unwary_t<pure_type_t<TMatrix>>>, TMatrix&>::type>(unwary(m));
}

template <typename TMatrix>
class Rows< Transposed<TMatrix> > : public Cols<TMatrix> {};

template <typename TMatrix>
class Cols< Transposed<TMatrix> > : public Rows<TMatrix> {};

/* ------------------------------------------------
 *  Methods defined for all matrices,
 *  depending on Rows::category and Cols::category
 * ------------------------------------------------ */
template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef> class MatrixMinor;

template <typename TMatrix, typename RowCategory=typename container_traits< Rows<TMatrix> >::category>
class matrix_row_methods {
public:
   typedef typename deref<typename container_traits< Rows<TMatrix> >::reference>::type row_type;
   typedef typename deref<typename container_traits< Rows<TMatrix> >::const_reference>::minus_ref const_row_type;

   int rows() const
   {
      return pm::rows(static_cast<const TMatrix&>(*this)).size();
   }

   // stub for BlockMatrix
   void stretch_rows(int) const
   {
      throw std::runtime_error("row dimension mismatch");
   }
};

template <typename TMatrix>
class matrix_row_methods<TMatrix, output_iterator_tag> {};

template <typename TMatrix>
class matrix_row_methods<TMatrix, random_access_iterator_tag>
   : public matrix_row_methods<TMatrix, forward_iterator_tag> {
public:
   decltype(auto) row(int i)
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (i<0 || i>=this->rows())
            throw std::runtime_error("matrix row index out of range");
      }
      return pm::rows(*static_cast<TMatrix*>(this))[i];
   }

   decltype(auto) operator[] (int i)
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (i<0 || i>=this->rows())
            throw std::runtime_error("matrix row index out of range");
      }
      return pm::rows(*static_cast<TMatrix*>(this))[i];
   }

   decltype(auto) row(int i) const
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (i<0 || i>=this->rows())
            throw std::runtime_error("matrix row index out of range");
      }
      return pm::rows(*static_cast<const TMatrix*>(this))[i];
   }

   decltype(auto) operator[] (int i) const
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (i<0 || i>=this->rows())
            throw std::runtime_error("matrix row index out of range");
      }
      return pm::rows(*static_cast<const TMatrix*>(this))[i];
   }
};

template <typename TMatrix, typename ColCategory=typename container_traits< Cols<TMatrix> >::category>
class matrix_col_methods {
public:
   typedef typename deref<typename container_traits< Cols<TMatrix> >::reference>::type col_type;
   typedef typename deref<typename container_traits< Cols<TMatrix> >::const_reference>::minus_ref const_col_type;

   int cols() const
   {
      return pm::cols(*static_cast<const TMatrix*>(this)).size();
   }

   // stub for BlockMatrix
   void stretch_cols(int) const
   {
      throw std::runtime_error("col dimension mismatch");
   }
};

template <typename TMatrix>
class matrix_col_methods<TMatrix, output_iterator_tag> {};

template <typename TMatrix>
class matrix_col_methods<TMatrix, random_access_iterator_tag>
   : public matrix_col_methods<TMatrix, forward_iterator_tag> {
public:
   decltype(auto) col(int i)
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (i<0 || i>=this->cols())
            throw std::runtime_error("matrix column index out of range");
      }
      return pm::cols(*static_cast<TMatrix*>(this))[i];
   }

   decltype(auto) col(int i) const
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (i<0 || i>=this->cols())
            throw std::runtime_error("matrix column index out of range");
      }
      return pm::cols(*static_cast<const TMatrix*>(this))[i];
   }
};

template <typename IndexSetRef>
struct final_index_set<IndexSetRef, std::enable_if_t<std::is_same<pure_type_t<IndexSetRef>, all_selector>::value>> {
   using type = const all_selector&;
};

template <typename TMatrix, typename E=typename TMatrix::element_type,
          typename RowCategory=typename container_traits< Rows<TMatrix> >::category,
          typename ColCategory=typename container_traits< Cols<TMatrix> >::category>
class matrix_methods
   : public matrix_row_methods<TMatrix>
   , public matrix_col_methods<TMatrix> {
public:
   using element_type = E;

   using container_category = typename mprefer1st<typename least_derived_class<typename container_traits< Rows<TMatrix> >::category,
                                                                               typename container_traits< Cols<TMatrix> >::category>::type,
                                                  output_iterator_tag>::type;

protected:
   template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
   static auto make_minor(MatrixRef&& matrix, RowIndexSetRef&& row_indices, ColIndexSetRef&& col_indices)
   {
      if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
         if (!set_within_range(row_indices, matrix.rows()))
            throw std::runtime_error("matrix minor - row indices out of range");

         if (!set_within_range(col_indices, matrix.cols()))
            throw std::runtime_error("matrix minor - column indices out of range");
      }
      using result_type = MatrixMinor<MatrixRef, typename final_index_set<RowIndexSetRef>::type, typename final_index_set<ColIndexSetRef>::type>;
      return result_type(std::forward<MatrixRef>(matrix),
                         prepare_index_set(std::forward<RowIndexSetRef>(row_indices), [&](){ return matrix.rows(); }),
                         prepare_index_set(std::forward<ColIndexSetRef>(col_indices), [&](){ return matrix.cols(); }));
   }
public:
   template <typename RowIndexSetRef, typename ColIndexSetRef>
   // gcc 5 can't digest auto here
   MatrixMinor<const typename Unwary<TMatrix>::type&, typename final_index_set<RowIndexSetRef>::type, typename final_index_set<ColIndexSetRef>::type>
   minor(RowIndexSetRef&& row_indices, ColIndexSetRef&& col_indices) const &
   {
      return make_minor(unwary(static_cast<const TMatrix&>(*this)),
                        std::forward<RowIndexSetRef>(row_indices),
                        std::forward<ColIndexSetRef>(col_indices));
   }

   template <typename RowIndexSetRef, typename ColIndexSetRef>
   // gcc 5 can't digest auto here
   MatrixMinor<typename Unwary<TMatrix>::type&, typename final_index_set<RowIndexSetRef>::type, typename final_index_set<ColIndexSetRef>::type>
   minor(RowIndexSetRef&& row_indices, ColIndexSetRef&& col_indices) &
   {
      return make_minor(unwary(static_cast<TMatrix&>(*this)),
                        std::forward<RowIndexSetRef>(row_indices),
                        std::forward<ColIndexSetRef>(col_indices));
   }

   template <typename RowIndexSetRef, typename ColIndexSetRef>
   // gcc 5 can't digest auto here
   MatrixMinor<typename Unwary<TMatrix>::type, typename final_index_set<RowIndexSetRef>::type, typename final_index_set<ColIndexSetRef>::type>
   minor(RowIndexSetRef&& row_indices, ColIndexSetRef&& col_indices) &&
   {
      return make_minor(unwary(static_cast<TMatrix&&>(*this)),
                        std::forward<RowIndexSetRef>(row_indices),
                        std::forward<ColIndexSetRef>(col_indices));
   }
};

template <typename TMatrix> class matrix_random_access_methods {};

template <typename TMatrix, typename E>
class matrix_methods<TMatrix, E, random_access_iterator_tag, random_access_iterator_tag>
   : public matrix_methods<TMatrix, E, forward_iterator_tag, forward_iterator_tag>,
     public matrix_random_access_methods<TMatrix> {};

template <typename TMatrix, typename E>
class matrix_methods<Wary<TMatrix>, E, random_access_iterator_tag, random_access_iterator_tag>
   : public matrix_methods<Wary<TMatrix>, E, forward_iterator_tag, forward_iterator_tag>
{
public:
   decltype(auto) operator() (int i, int j)
   {
      if (i<0 || i>=this->rows() || j<0 || j>=this->cols())
         throw std::runtime_error("matrix element access - index out of range");
      return unwary(static_cast<Wary<TMatrix>&>(*this))(i,j);
   }
   decltype(auto) operator() (int i, int j) const
   {
      if (i<0 || i>=this->rows() || j<0 || j>=this->cols())
         throw std::runtime_error("matrix element access - index out of range");
      return unwary(static_cast<const Wary<TMatrix>&>(*this))(i,j);
   }
};

template <typename TMatrix>
class matrix_random_access_methods< Transposed<TMatrix> > {
public:
   decltype(auto) operator() (int i, int j)
   {
      return static_cast<Transposed<TMatrix>*>(this)->hidden()(j, i);
   }
   decltype(auto) operator() (int i, int j) const
   {
      return static_cast<const Transposed<TMatrix>*>(this)->hidden()(j, i);
   }
};

template <typename TMatrix>
class Transposed< Transposed<TMatrix> > : public TMatrix {
protected:
   Transposed();
   ~Transposed();
};

/* -------------
 *  MatrixMinor
 * ------------- */

template <>
class alias<const all_selector&, alias_kind::ref> {
public:
   typedef const all_selector& arg_type;
   typedef const alias& reference;
   typedef reference const_reference;

   alias(arg_type) {}

   reference operator* () const { return *this; }
   int operator[] (int i) const { return i; }
};

constexpr bool set_within_range(const all_selector&, int) { return true; }

template <typename GetDim>
const all_selector& prepare_index_set(all_selector&& s, const GetDim&)
{
   return s;
}

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
class minor_base {
public:
   using matrix_type = typename deref<MatrixRef>::type;
   using row_set_type =typename deref<RowIndexSetRef>::type;
   using col_set_type = typename deref<ColIndexSetRef>::type;

protected:
   using matrix_alias_t = alias<MatrixRef>;
   using rset_alias_t = alias<RowIndexSetRef>;
   using cset_alias_t = alias<ColIndexSetRef>;
   matrix_alias_t matrix;
   rset_alias_t rset;
   cset_alias_t cset;

public:
   template <typename Arg1, typename Arg2, typename Arg3,
             typename=std::enable_if_t<std::is_constructible<matrix_alias_t, Arg1>::value &&
                                       std::is_constructible<rset_alias_t, Arg2>::value &&
                                       std::is_constructible<cset_alias_t, Arg3>::value>>
   minor_base(Arg1&& matrix_arg, Arg2&& rset_arg, Arg3&& cset_arg)
      : matrix(std::forward<Arg1>(matrix_arg))
      , rset(std::forward<Arg2>(rset_arg))
      , cset(std::forward<Arg3>(cset_arg)) {}

   decltype(auto) get_matrix() { return *matrix; }
   decltype(auto) get_matrix() const { return *matrix; }

   const rset_alias_t& get_subset_alias(int_constant<1>) const { return rset; }
   const cset_alias_t& get_subset_alias(int_constant<2>) const { return cset; }
   decltype(auto) get_subset(int_constant<1>) const { return *rset; }
   decltype(auto) get_subset(int_constant<2>) const { return *cset; }

   int random_row(int i, std::false_type) const { return get_subset(int_constant<1>())[i]; }
   int random_col(int i, std::false_type) const { return get_subset(int_constant<2>())[i]; }
   int random_row(int i, std::true_type) const { return i; }
   int random_col(int i, std::true_type) const { return i; }

   int random_row(int i) const { return random_row(i, std::is_same<pure_type_t<RowIndexSetRef>, all_selector>()); }
   int random_col(int i) const { return random_col(i, std::is_same<pure_type_t<ColIndexSetRef>, all_selector>()); }
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
class MatrixMinor
   : public minor_base<MatrixRef, RowIndexSetRef, ColIndexSetRef>
   , public inherit_generic<MatrixMinor<MatrixRef,RowIndexSetRef,ColIndexSetRef>, typename deref<MatrixRef>::type>::type {
   using base_t = minor_base<MatrixRef, RowIndexSetRef, ColIndexSetRef>;
public:
   using matrix_type = typename base_t::matrix_type;
   using value_type = typename container_traits<MatrixRef>::value_type;
   using reference = typename container_traits<MatrixRef>::reference;
   using const_reference = typename container_traits<MatrixRef>::const_reference;

   using minor_base<MatrixRef, RowIndexSetRef, ColIndexSetRef>::minor_base;

   /// Assignment operator should copy elements instead of alias pointers
   MatrixMinor& operator= (const MatrixMinor& other) { return MatrixMinor::generic_type::operator=(other); }
   using MatrixMinor::generic_type::operator=;

protected:
   void clear_impl(std::true_type)
   {
      for (auto c=entire(pm::cols(*this)); !c.at_end(); ++c)
         c->fill(0);
   }
   void clear_impl(std::false_type)
   {
      for (auto r=entire(pm::rows(*this)); !r.at_end(); ++r)
         r->fill(0);
   }
public:
   /// fill with zeroes (if dense), delete elements (if sparse)
   void clear()
   {
      clear_impl(std::is_same<typename base_t::row_set_type, all_selector>());
   }
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
struct spec_object_traits< MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef> >
   : spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_lazy = object_traits<typename deref<MatrixRef>::type>::is_lazy,
      is_always_const = is_effectively_const<MatrixRef>::value;
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
struct check_container_feature<MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef>, sparse>
   : check_container_ref_feature<MatrixRef, sparse> {};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
struct check_container_feature<MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef>, pure_sparse>
   : check_container_ref_feature<MatrixRef, pure_sparse> {};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
struct check_container_feature<MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef>, FlatStorage>
   : mlist_and< check_container_ref_feature<MatrixRef, FlatStorage>,
                same_pure_type<ColIndexSetRef, all_selector> > {};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
class matrix_random_access_methods< MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef> > {
   typedef MatrixMinor<MatrixRef,RowIndexSetRef,ColIndexSetRef> master;
public:
   typename inherit_const<typename deref<MatrixRef>::type::reference, MatrixRef>::type
   operator() (int i, int j)
   {
      master& me=static_cast<master&>(*this);
      return me.get_matrix()(me.random_row(i), me.random_col(j));
   }

   typename deref<MatrixRef>::type::const_reference
   operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      return me.get_matrix()(me.random_row(i), me.random_col(j));
   }
};

template <typename TMinor, int dir>
struct RowCol_helper;

template <typename TMinor>
struct RowCol_helper<TMinor, 1> : masquerade<Rows, typename mget_template_parameter<TMinor, 0>::type> {};

template <typename TMinor>
struct RowCol_helper<TMinor, 2> : masquerade<Cols, typename mget_template_parameter<TMinor, 0>::type> {};

template <typename TMinor, typename TRenumber, int TDir, typename TSelector=typename mget_template_parameter<TMinor, TDir>::type>
class RowColSubset
   : public indexed_subset_impl< RowColSubset<TMinor, TRenumber, TDir, TSelector>,
                                 mlist< Container1RefTag< typename RowCol_helper<TMinor, TDir>::type >,
                                        Container2RefTag< TSelector >,
                                        RenumberTag< TRenumber >,
                                        HiddenTag< TMinor > > > {
   typedef indexed_subset_impl<RowColSubset> base_t;
public:
   typename base_t::container1& get_container1()
   {
      return reinterpret_cast<typename base_t::container1&>(this->hidden().get_matrix());
   }
   const typename base_t::container1& get_container1() const
   {
      return reinterpret_cast<const typename base_t::container1&>(this->hidden().get_matrix());
   }
   const typename base_t::container2& get_container2() const
   {
      return this->hidden().get_subset(int_constant<TDir>());
   }
};

template <typename TMinor, typename TRenumber, int TDir>
class RowColSubset<TMinor, TRenumber, TDir, const all_selector&>
   : public redirected_container< RowColSubset<TMinor, TRenumber, TDir, const all_selector&>,
                                  mlist< ContainerRefTag< typename RowCol_helper<TMinor, TDir>::type >,
                                         HiddenTag< TMinor > > > {
   typedef redirected_container<RowColSubset> base_t;
public:
   typename base_t::container& get_container()
   {
      return reinterpret_cast<typename base_t::container&>(this->hidden().get_matrix());
   }
   const typename base_t::container& get_container() const
   {
      return reinterpret_cast<const typename base_t::container&>(this->hidden().get_matrix());
   }
};

template <typename TMinor, typename TRenumber, int TDir, typename TSliceConstructor,
          typename TCrossSelector=typename mget_template_parameter<TMinor, 3-TDir>::type>
class RowsCols
   : public modified_container_pair_impl< RowsCols<TMinor, TRenumber, TDir, TSliceConstructor, TCrossSelector>,
                                          mlist< Container1Tag< RowColSubset<TMinor, TRenumber, TDir> >,
                                                 Container2Tag< same_value_container<TCrossSelector> >,
                                                 HiddenTag< TMinor >,
                                                 OperationTag< TSliceConstructor > > > {
   typedef modified_container_pair_impl<RowsCols> base_t;
protected:
   ~RowsCols();
public:
   decltype(auto) get_container2() const
   {
      return as_same_value_container(this->hidden().get_subset_alias(int_constant<3-TDir>()));
   }
};

template <typename TMinor, typename TRenumber, int TDir, typename TSliceConstructor>
class RowsCols<TMinor, TRenumber, TDir, TSliceConstructor, const all_selector&>
   : public RowColSubset<TMinor, TRenumber, TDir> {
protected:
   ~RowsCols();
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
class Rows< MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef> >
   : public RowsCols< minor_base<MatrixRef, RowIndexSetRef, ColIndexSetRef>, std::true_type, 1,
                      operations::construct_binary2<IndexedSlice, mlist<>> > {
protected:
   ~Rows();
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
class Cols< MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef> >
   : public RowsCols< minor_base<MatrixRef, RowIndexSetRef, ColIndexSetRef>, std::true_type, 2,
                      operations::construct_binary2<IndexedSlice, mlist<>> > {
protected:
   ~Cols();
};

/* ----------
 *  BlockMatrix
 * ---------- */

template <typename MatrixList, typename rowwise>
class BlockMatrix
   : public alias_tuple<MatrixList>
   , public inherit_generic< BlockMatrix<MatrixList, rowwise>,
                             typename mlist_transform_unary<MatrixList, deref>::type >::type {

   using arg_helper = chain_arg_helper<pm::BlockMatrix, rowwise>;

   using element_types = typename mlist_transform_unary<MatrixList, extract_element_type>::type;
   static_assert(mlist_length<typename mlist_remove_duplicates<element_types>::type>::value == 1,
                 "blocks with different element types");

public:
   using traits = typename prepare_union_container_traits<MatrixList>::type;
   using value_type = typename traits::value_type;
   using reference = typename traits::reference;
   using const_reference = typename traits::const_reference;

   // TODO: =delete
   BlockMatrix(const BlockMatrix&) = default;
   BlockMatrix(BlockMatrix&&) = default;

   template <typename... Args,
             typename=std::enable_if_t<arg_helper::allow(MatrixList(), mlist<Args...>())>>
   explicit BlockMatrix(Args&&... args)
      : alias_tuple<MatrixList>(arg_helper(), std::forward<Args>(args)...)
   {
      int d=0;
      bool saw_zero_dim=false;
      foreach_in_tuple(this->aliases, [&d, &saw_zero_dim](auto&& a) -> void {
         const int d_cur= rowwise::value ? a->cols() : a->rows();
         if (d_cur) {
            if (d) {
               if (d_cur != d) throw std::runtime_error(rowwise::value ? "block matrix - col dimension mismatch"
                                                                       : "block matrix - row dimension mismatch");
            } else {
               d=d_cur;
            }
         } else {
            saw_zero_dim=true;
         }
      });

      if (saw_zero_dim && d) {
         foreach_in_tuple(this->aliases, [d](auto&& a) -> void {
            if (rowwise::value) {
               if (a->cols() == 0) a.get_object().stretch_cols(d);
            } else {
               if (a->rows() == 0) a.get_object().stretch_rows(d);
            }
         });
      }
   }

   template <typename OtherList, typename otherwise,
             typename=std::enable_if_t<mlist_length<MatrixList>::value == mlist_length<OtherList>::value>>
   explicit BlockMatrix(const BlockMatrix<OtherList, otherwise>& other)
      : alias_tuple<MatrixList>(chain_arg_helper<pm::BlockMatrix, otherwise>(), other) {}

   template <typename OtherList, typename otherwise,
             typename=std::enable_if_t<mlist_length<MatrixList>::value == mlist_length<OtherList>::value>>
   explicit BlockMatrix(BlockMatrix<OtherList, otherwise>&& other)
      : alias_tuple<MatrixList>(chain_arg_helper<pm::BlockMatrix, otherwise>(), std::move(other)) {}

   BlockMatrix& operator= (const BlockMatrix& other) { return BlockMatrix::generic_type::operator=(other); }
   using BlockMatrix::generic_type::operator=;
};

template <typename MatrixList, typename rowwise>
struct spec_object_traits< BlockMatrix<MatrixList, rowwise> >
   : spec_object_traits<is_container> {
   static constexpr bool
      is_temporary = true,
      is_lazy = mlist_or<typename mlist_transform_unary<MatrixList, extract_lazy>::type>::value,
      is_always_const = prepare_union_container_traits<MatrixList>::type::is_always_const;
};

template <typename MatrixList, typename rowwise>
struct check_container_feature< BlockMatrix<MatrixList, rowwise>, sparse>
   : mlist_or< typename mlist_transform_binary<MatrixList, mrepeat<sparse>, check_container_ref_feature>::type > {};

template <typename MatrixList, typename rowwise>
struct check_container_feature< BlockMatrix<MatrixList, rowwise>, pure_sparse>
   : mlist_and< typename mlist_transform_binary<MatrixList, mrepeat<pure_sparse>, check_container_ref_feature>::type > {};

template <typename MatrixList, typename rowwise>
struct check_container_feature< BlockMatrix<MatrixList, rowwise>, FlatStorage>
   : mlist_and< typename mlist_transform_binary<MatrixList, mrepeat<FlatStorage>, check_container_ref_feature>::type > {};

template <typename T>
struct masquerade_as_Rows {
   using type = masquerade<Rows, T>;
};
template <typename T>
struct masquerade_as_Cols {
   using type = masquerade<Cols, T>;
};
template <typename T>
struct masquerade_as_Transposed {
   using type = masquerade<Transposed, T>;
};

// to be specialized for various generic types
template <typename TGenericMatrix>
struct concat_lines_op {};

template <typename Result, typename TGenericVector>
struct generic_of_repeated_line {};

template <typename MatrixList>
using concat_lines_op_for
= typename concat_lines_op<typename deref<typename mlist_head<MatrixList>::type>::type::generic_type>::type;

template <typename MatrixList>
class Rows< BlockMatrix<MatrixList, std::true_type> >
   : public container_chain_impl< Rows< BlockMatrix<MatrixList, std::true_type> >,
                                  mlist< ContainerRefTag< typename mlist_transform_unary<MatrixList, masquerade_as_Rows>::type >,
                                         MasqueradedTop > > {
protected:
   ~Rows();
public:
   template <int i>
   decltype(auto) get_container(int_constant<i>)
   {
      return rows(this->hidden().get_container(int_constant<i>()));
   }
   template <int i>
   decltype(auto) get_container(int_constant<i>) const
   {
      return rows(this->hidden().get_container(int_constant<i>()));
   }
};

template <typename MatrixList>
class Cols< BlockMatrix<MatrixList, std::true_type> >
   : public modified_container_tuple_impl< Cols< BlockMatrix<MatrixList, std::true_type> >,
                                           mlist< ContainerRefTag< typename mlist_transform_unary<MatrixList, masquerade_as_Cols>::type >,
                                                  OperationTag< concat_lines_op_for<MatrixList> >,
                                                  MasqueradedTop > > {
protected:
   ~Cols();
public:
   template <int i>
   decltype(auto) get_container(int_constant<i>)
   {
      return cols(this->hidden().get_container(int_constant<i>()));
   }
   template <int i>
   decltype(auto) get_container(int_constant<i>) const
   {
      return cols(this->hidden().get_container(int_constant<i>()));
   }
   static constexpr auto get_operation()
   {
      return concat_lines_op_for<MatrixList>();
   }
};

template <typename MatrixList>
class Rows< BlockMatrix<MatrixList, std::false_type> >
   : public modified_container_tuple_impl< Rows< BlockMatrix<MatrixList, std::false_type> >,
                                          mlist< ContainerRefTag< typename mlist_transform_unary<MatrixList, masquerade_as_Rows>::type >,
                                                 OperationTag< concat_lines_op_for<MatrixList> >,
                                                 MasqueradedTop > > {
protected:
   ~Rows();
public:
   template <int i>
   decltype(auto) get_container(int_constant<i>)
   {
      return rows(this->hidden().get_container(int_constant<i>()));
   }
   template <int i>
   decltype(auto) get_container(int_constant<i>) const
   {
      return rows(this->hidden().get_container(int_constant<i>()));
   }
   static constexpr auto get_operation()
   {
      return concat_lines_op_for<MatrixList>();
   }
};

template <typename MatrixList>
class Cols< BlockMatrix<MatrixList, std::false_type> >
   : public container_chain_impl< Cols< BlockMatrix<MatrixList, std::false_type> >,
                                  mlist< ContainerRefTag< typename mlist_transform_unary<MatrixList, masquerade_as_Cols>::type >,
                                         MasqueradedTop > > {
protected:
   ~Cols();
public:
   template <int i>
   decltype(auto) get_container(int_constant<i>)
   {
      return cols(this->hidden().get_container(int_constant<i>()));
   }
   template <int i>
   decltype(auto) get_container(int_constant<i>) const
   {
      return cols(this->hidden().get_container(int_constant<i>()));
   }
};

template <typename MatrixList, bool rowwise>
auto T(const BlockMatrix<MatrixList, bool_constant<rowwise>>& M)
{
   return BlockMatrix<typename mlist_transform_unary<MatrixList, masquerade_as_Transposed>::type, bool_constant<!rowwise>>(M);
}

template <typename MatrixList, bool rowwise>
auto T(BlockMatrix<MatrixList, bool_constant<rowwise>>&& M)
{
   return BlockMatrix<typename mlist_transform_unary<MatrixList, masquerade_as_Transposed>::type, bool_constant<!rowwise>>(std::move(M));
}


/* ----------------------------------
 *  base for RepeatedRow, RepeatedCol
 * ---------------------------------- */
// TODO: merge with RepeatedRow, RepeatedCol, SingleIncidenceRow, SingleIncidenceCol
template <typename LineRef>
class repeated_line_matrix {
protected:
   using line_container_t = repeated_value_container<LineRef>;
   line_container_t line_container;
public:
   using line_t = pure_type_t<LineRef>;
   using value_type = typename line_t::value_type;
   using reference = std::conditional_t<is_const<LineRef>::value, typename line_t::const_reference, typename line_t::reference>;
   using const_reference = typename line_t::const_reference;

   template <typename Arg, typename=std::enable_if_t<std::is_constructible<line_container_t, Arg, int>::value>>
   repeated_line_matrix(Arg&& line_arg, int cnt_arg)
      : line_container(std::forward<Arg>(line_arg), cnt_arg) {}

   const line_container_t& get_line_container() const { return line_container; }

   decltype(auto) get_line() { return line_container.front(); }
   decltype(auto) get_line() const { return line_container.front(); }

   int get_count() const { return line_container.size(); }
};

} // end namespace pm

namespace polymake {
   using pm::Transposed;
   using pm::Symmetric;
   using pm::NonSymmetric;
   using pm::SkewSymmetric;
}

namespace std {

// due to silly overloading rules
template <typename Matrix>
void swap(pm::Transposed<Matrix>& m1, pm::Transposed<Matrix>& m2)
{
   m1.swap(m2);
}

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
void swap(pm::MatrixMinor<MatrixRef,RowIndexSetRef,ColIndexSetRef>& m1,
          pm::MatrixMinor<MatrixRef,RowIndexSetRef,ColIndexSetRef>& m2)
{
   m1.swap(m2);
}

template <typename MatrixList, typename rowwise>
void swap(pm::BlockMatrix<MatrixList, rowwise>& m1, pm::BlockMatrix<MatrixList, rowwise>& m2)
{
   m1.swap(m2);
}

}

#endif // POLYMAKE_INTERNAL_MATRIX_METHODS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
