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

struct Symmetric : True {};
struct NonSymmetric : False {};
struct SkewSymmetric : Symmetric {};
struct FlatStorage {};

template <typename Matrix>
struct default_check_container_feature<Matrix, Symmetric> : False {};

template <typename Matrix>
struct default_check_container_feature<Matrix, SkewSymmetric> : False {};

template <typename Matrix>
struct default_check_container_feature<Matrix, FlatStorage> : False {};

template <typename Matrix>
struct default_check_container_feature<Matrix, NonSymmetric> {
   static const bool value= !check_container_feature<Matrix,Symmetric>::value &&
                            !check_container_feature<Matrix,SkewSymmetric>::value;
};

template <typename Matrix>
struct matrix_symmetry_type
   : choice< if_else< check_container_feature<Matrix,Symmetric>::value, Symmetric,
             if_else< check_container_feature<Matrix,SkewSymmetric>::value, SkewSymmetric,
                      NonSymmetric > > > {};


template <typename Matrix> inline
int empty_rows(const Matrix& m)
{
   int cnt=0;
   for (typename Entire< Rows<Matrix> >::const_iterator r=entire(rows(m)); !r.at_end(); ++r)
      if (!r->size()) ++cnt;
   return cnt;
}

template <typename Matrix> inline
int empty_cols(const Matrix& m)
{
   int cnt=0;
   for (typename Entire< Cols<Matrix> >::const_iterator c=entire(cols(m)); !c.at_end(); ++c)
      if (!c->size()) ++cnt;
   return cnt;
}

/* -------------------------------
 *  Matrix masquerade: Transposed
 * ------------------------------- */

template <typename Matrix>
class Transposed
   : public inherit_generic<Transposed<Matrix>, Matrix>::type {
protected:
   ~Transposed();
public:
   typedef typename Matrix::value_type value_type;
   typedef typename Matrix::reference reference;
   typedef typename Matrix::const_reference const_reference;

   Matrix& hidden() { return reinterpret_cast<Matrix&>(*this); }
   const Matrix& hidden() const { return reinterpret_cast<const Matrix&>(*this); }

   void clear() { hidden().clear(); }
   void clear(int r, int c) { hidden().clear(c,r); }
};

template <typename Matrix>
struct spec_object_traits< Transposed<Matrix> >
   : spec_object_traits<is_container> {
   typedef Matrix masquerade_for;
   static const bool is_lazy         = object_traits<Matrix>::is_lazy,
                     is_always_const = object_traits<Matrix>::is_always_const;
   static const int is_resizeable= object_traits<Matrix>::is_resizeable;
};

template <typename Matrix>
struct check_container_feature<Transposed<Matrix>, sparse>
   : check_container_feature<Matrix, sparse> {};

template <typename Matrix>
struct check_container_feature<Transposed<Matrix>, pure_sparse>
   : check_container_feature<Matrix, pure_sparse> {};

template <typename Matrix> inline
Transposed<typename Concrete<Matrix>::type>& T(Matrix& m)
{
   return reinterpret_cast<Transposed<typename Concrete<Matrix>::type>&>(concrete(m));
}

template <typename Matrix> inline
const Transposed<typename Concrete<Matrix>::type>& T(const Matrix& m)
{
   return reinterpret_cast<const Transposed<typename Concrete<Matrix>::type>&>(concrete(m));
}

template <typename Matrix>
class Rows< Transposed<Matrix> > : public Cols<Matrix> {};

template <typename Matrix>
class Cols< Transposed<Matrix> > : public Rows<Matrix> {};

/* ------------------------------------------------
 *  Methods defined for all matrices,
 *  depending on Rows::category and Cols::category
 * ------------------------------------------------ */
template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef> class MatrixMinor;

template <typename Matrix, typename RowCategory=typename container_traits< Rows<Matrix> >::category>
class matrix_row_methods {
public:
   typedef typename deref<typename container_traits< Rows<Matrix> >::reference>::type row_type;
   typedef typename deref<typename container_traits< Rows<Matrix> >::const_reference>::minus_ref const_row_type;

   int rows() const
   {
      return pm::rows(static_cast<const Matrix&>(*this)).size();
   }

   // stub for ColChain
   void stretch_rows(int r) const
   {
      if (r) throw std::runtime_error("rows number mismatch");
   }
};

template <typename Matrix>
class matrix_row_methods<Matrix, random_access_iterator_tag>
   : public matrix_row_methods<Matrix, forward_iterator_tag> {
public:
   typename container_traits< Rows<Matrix> >::reference
   row(int i)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (i<0 || i>=this->rows())
            throw std::runtime_error("matrix row index out of range");
      }
      return pm::rows(*static_cast<Matrix*>(this))[i];
   }
   typename container_traits< Rows<Matrix> >::reference
   operator[] (int i)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (i<0 || i>=this->rows())
            throw std::runtime_error("matrix row index out of range");
      }
      return pm::rows(*static_cast<Matrix*>(this))[i];
   }
   typename container_traits< Rows<Matrix> >::const_reference
   row(int i) const
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (i<0 || i>=this->rows())
            throw std::runtime_error("matrix row index out of range");
      }
      return pm::rows(*static_cast<const Matrix*>(this))[i];
   }
   typename container_traits< Rows<Matrix> >::const_reference
   operator[] (int i) const
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (i<0 || i>=this->rows())
            throw std::runtime_error("matrix row index out of range");
      }
      return pm::rows(*static_cast<const Matrix*>(this))[i];
   }
};

template <typename Matrix, typename ColCategory=typename container_traits< Cols<Matrix> >::category>
class matrix_col_methods {
public:
   typedef typename deref<typename container_traits< Cols<Matrix> >::reference>::type col_type;
   typedef typename deref<typename container_traits< Cols<Matrix> >::const_reference>::minus_ref const_col_type;

   int cols() const
   {
      return pm::cols(*static_cast<const Matrix*>(this)).size();
   }

   // stub for RowChain
   void stretch_cols(int c) const
   {
      if (c) throw std::runtime_error("columns number mismatch");
   }
};

template <typename Matrix>
class matrix_col_methods<Matrix, random_access_iterator_tag>
   : public matrix_col_methods<Matrix, forward_iterator_tag> {
public:
   typename container_traits< Cols<Matrix> >::reference
   col(int i)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (i<0 || i>=this->cols())
            throw std::runtime_error("matrix column index out of range");
      }
      return pm::cols(*static_cast<Matrix*>(this))[i];
   }
   typename container_traits< Cols<Matrix> >::const_reference
   col(int i) const
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (i<0 || i>=this->cols())
            throw std::runtime_error("matrix column index out of range");
      }
      return pm::cols(*static_cast<const Matrix*>(this))[i];
   }
};

template <typename Matrix, typename E=typename Matrix::element_type,
          typename RowCategory=typename container_traits< Rows<Matrix> >::category,
          typename ColCategory=typename container_traits< Cols<Matrix> >::category>
class matrix_methods
   : public matrix_row_methods<Matrix>,
     public matrix_col_methods<Matrix> {
public:
   typedef E element_type;
   typedef typename least_derived< cons< typename container_traits< Rows<Matrix> >::category,
                                         typename container_traits< Cols<Matrix> >::category > >::type
      container_category;

   template <typename RowIndexSet, typename ColIndexSet>
   MatrixMinor<typename Unwary<Matrix>::type&,
               typename Diligent<const RowIndexSet&>::type,
               typename Diligent<const ColIndexSet&>::type>
   minor(const RowIndexSet& row_indices, const ColIndexSet& col_indices)
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (!set_within_range(row_indices, this->rows()))
            throw std::runtime_error("matrix minor - row indices out of range");

         if (!set_within_range(col_indices, this->cols()))
            throw std::runtime_error("matrix minor - column indices out of range");
      }
                                                                                            
      return MatrixMinor<typename Unwary<Matrix>::type&,
                         typename Diligent<const RowIndexSet&>::type,
                         typename Diligent<const ColIndexSet&>::type>
                        (static_cast<Matrix&>(*this).top(), diligent(row_indices), diligent(col_indices));
   }

   template <typename RowIndexSet, typename ColIndexSet>
   const MatrixMinor<const typename Unwary<Matrix>::type&,
                     typename Diligent<const RowIndexSet&>::type,
                     typename Diligent<const ColIndexSet&>::type>
   minor(const RowIndexSet& row_indices, const ColIndexSet& col_indices) const
   {
      if (POLYMAKE_DEBUG || !Unwary<Matrix>::value) {
         if (!set_within_range(row_indices, this->rows()))
            throw std::runtime_error("matrix minor - row indices out of range");

         if (!set_within_range(col_indices, this->cols()))
            throw std::runtime_error("matrix minor - column indices out of range");
      }
                                                                                            
      return MatrixMinor<const typename Unwary<Matrix>::type&,
                         typename Diligent<const RowIndexSet&>::type,
                         typename Diligent<const ColIndexSet&>::type>
                        (static_cast<const Matrix&>(*this).top(), diligent(row_indices), diligent(col_indices));
   }
};

template <typename Matrix> class matrix_random_access_methods {};

template <typename Matrix, typename E>
class matrix_methods<Matrix, E, random_access_iterator_tag, random_access_iterator_tag>
   : public matrix_methods<Matrix, E, forward_iterator_tag, forward_iterator_tag>,
     public matrix_random_access_methods<Matrix> {};

template <typename Matrix, typename E>
class matrix_methods<Wary<Matrix>, E, random_access_iterator_tag, random_access_iterator_tag>
   : public matrix_methods<Wary<Matrix>, E, forward_iterator_tag, forward_iterator_tag>
{
public:
   typename Matrix::reference
   operator() (int i, int j)
   {
      if (i<0 || i>=this->rows() || j<0 || j>=this->cols())
         throw std::runtime_error("matrix element access - index out of range");
      return unwary(static_cast<Wary<Matrix>&>(*this))(i,j);
   }
   typename Matrix::const_reference
   operator() (int i, int j) const
   {
      if (i<0 || i>=this->rows() || j<0 || j>=this->cols())
         throw std::runtime_error("matrix element access - index out of range");
      return unwary(static_cast<const Wary<Matrix>&>(*this))(i,j);
   }
};

template <typename Matrix>
class matrix_random_access_methods< Transposed<Matrix> > {
public:
   typename Matrix::reference
   operator() (int i, int j)
   {
      return static_cast<Transposed<Matrix>*>(this)->hidden()(j,i);
   }
   typename Matrix::const_reference
   operator() (int i, int j) const
   {
      return static_cast<const Transposed<Matrix>*>(this)->hidden()(j,i);
   }
};

template <typename Matrix>
class Transposed< Transposed<Matrix> > : public Matrix {
protected:
   Transposed();
   ~Transposed();
};

/* -------------
 *  MatrixMinor
 * ------------- */

enum all_selector { All };

template <>
class alias<const all_selector&, object_classifier::alias_ref> {
public:
   typedef const all_selector& arg_type;
   typedef const alias& reference;
   typedef reference const_reference;

   alias(arg_type) {}

   reference operator* () const { return *this; }
   int operator[] (int i) const { return i; }
};

inline bool set_within_range(const all_selector&, int) { return true; }

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
class minor_base {
public:
   typedef typename deref<MatrixRef>::type matrix_type;
   typedef typename deref<RowIndexSetRef>::type row_set_type;
   typedef typename deref<ColIndexSetRef>::type col_set_type;

protected:
   alias<MatrixRef> matrix;
   alias<RowIndexSetRef> rset;
   alias<ColIndexSetRef> cset;

   typedef typename alias<MatrixRef>::arg_type matrix_arg_type;
   typedef typename alias<RowIndexSetRef>::arg_type row_set_arg_type;
   typedef typename alias<ColIndexSetRef>::arg_type col_set_arg_type;

   minor_base(matrix_arg_type matrix_arg, row_set_arg_type rset_arg, col_set_arg_type cset_arg)
      : matrix(matrix_arg), rset(rset_arg), cset(cset_arg) {}

public:
   typename alias<MatrixRef>::reference get_matrix() { return *matrix; }
   typename alias<MatrixRef>::const_reference get_matrix() const { return *matrix; }

   const alias<RowIndexSetRef>& get_subset_alias(int2type<1>) const { return rset; }
   const alias<ColIndexSetRef>& get_subset_alias(int2type<2>) const { return cset; }
   typename alias<RowIndexSetRef>::const_reference get_subset(int2type<1>) const { return *rset; }
   typename alias<ColIndexSetRef>::const_reference get_subset(int2type<2>) const { return *cset; }

   template <typename RowIndexSet>
   int random_row(int i, type2type<RowIndexSet>) const { return get_subset(int2type<1>())[i]; }
   template <typename ColIndexSet>
   int random_col(int i, type2type<ColIndexSet>) const { return get_subset(int2type<2>())[i]; }
   int random_row(int i, type2type<all_selector>) const { return i; }
   int random_col(int i, type2type<all_selector>) const { return i; }

   int random_row(int i) const { return random_row(i, type2type<typename deref<RowIndexSetRef>::type>()); }
   int random_col(int i) const { return random_col(i, type2type<typename deref<ColIndexSetRef>::type>()); }
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
class MatrixMinor
   : public minor_base<MatrixRef, RowIndexSetRef, ColIndexSetRef>,
     public inherit_generic<MatrixMinor<MatrixRef,RowIndexSetRef,ColIndexSetRef>, typename deref<MatrixRef>::type>::type {
   typedef minor_base<MatrixRef, RowIndexSetRef, ColIndexSetRef> _base;
public:
   typedef typename _base::matrix_type matrix_type;
   typedef typename container_traits<MatrixRef>::value_type value_type;
   typedef typename container_traits<MatrixRef>::reference reference;
   typedef typename container_traits<MatrixRef>::const_reference const_reference;

   MatrixMinor(typename _base::matrix_arg_type matrix_arg,
               typename _base::row_set_arg_type rset_arg,
               typename _base::col_set_arg_type cset_arg)
      : _base(matrix_arg,rset_arg,cset_arg) {}

   /// Assignment operator should copy elements instead of alias pointers
   MatrixMinor& operator= (const MatrixMinor& other) { return MatrixMinor::generic_type::operator=(other); }
   using MatrixMinor::generic_type::operator=;

protected:
   void clear(True)
   {
      for (typename Entire< Cols<MatrixMinor> >::iterator c=entire(pm::cols(*this)); !c.at_end(); ++c)
         c->fill(0);
   }
   void clear(False)
   {
      for (typename Entire< Rows<MatrixMinor> >::iterator r=entire(pm::rows(*this)); !r.at_end(); ++r)
         r->fill(0);
   }
public:
   /// fill with zeroes (if dense), delete elements (if sparse)
   void clear()
   {
      clear(identical<typename _base::row_set_type,all_selector>());
   }
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
struct spec_object_traits< MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef> >
   : spec_object_traits<is_container> {
   static const bool is_temporary    = true,
                     is_lazy         = object_traits<typename deref<MatrixRef>::type>::is_lazy,
                     is_always_const = effectively_const<MatrixRef>::value;
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
struct check_container_feature<MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef>, sparse>
   : check_container_ref_feature<MatrixRef, sparse> {};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
struct check_container_feature<MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef>, pure_sparse>
   : check_container_ref_feature<MatrixRef, pure_sparse> {};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
struct check_container_feature<MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef>, FlatStorage>
{
   static const bool value=check_container_ref_feature<MatrixRef, FlatStorage>::value &&
                           identical_minus_const_ref<ColIndexSetRef, all_selector>::value;
};

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

template <typename Minor, int dir>
struct RowCol_helper;

template <typename Minor>
struct RowCol_helper<Minor,1> : masquerade<Rows, typename extract_template_type_param<Minor,0>::type> {};

template <typename Minor>
struct RowCol_helper<Minor,2> : masquerade<Cols, typename extract_template_type_param<Minor,0>::type> {};

template <typename Minor, typename RenumberVal, int dir, typename Selector=typename extract_template_type_param<Minor,dir>::type>
class RowColSubset
   : public indexed_subset_impl< RowColSubset<Minor, RenumberVal, dir, Selector>,
                                 list( Container1< typename RowCol_helper<Minor,dir>::type >,
                                       Container2< Selector >,
                                       Renumber< RenumberVal >,
                                       Hidden< Minor > ) > {
   typedef indexed_subset_impl<RowColSubset> _super;
public:
   typename _super::container1& get_container1()
   {
      return reinterpret_cast<typename _super::container1&>(this->hidden().get_matrix());
   }
   const typename _super::container1& get_container1() const
   {
      return reinterpret_cast<const typename _super::container1&>(this->hidden().get_matrix());
   }
   const typename _super::container2& get_container2() const
   {
      return this->hidden().get_subset(int2type<dir>());
   }
};

template <typename Minor, typename RenumberVal, int dir>
class RowColSubset<Minor, RenumberVal, dir, const all_selector&>
   : public redirected_container< RowColSubset<Minor, RenumberVal, dir, const all_selector&>,
                                  list( Container< typename RowCol_helper<Minor,dir>::type >,
                                        Hidden< Minor > ) > {
   typedef redirected_container<RowColSubset> _super;
public:
   typename _super::container& get_container()
   {
      return reinterpret_cast<typename _super::container&>(this->hidden().get_matrix());
   }
   const typename _super::container& get_container() const
   {
      return reinterpret_cast<const typename _super::container&>(this->hidden().get_matrix());
   }
};

template <typename Minor, typename RenumberVal, int dir, typename SliceConstructor,
          typename CrossSelector=typename extract_template_type_param<Minor,3-dir>::type>
class RowsCols
   : public modified_container_pair_impl< RowsCols<Minor, RenumberVal, dir, SliceConstructor,CrossSelector>,
                                          list( Container1< RowColSubset<Minor, RenumberVal, dir> >,
                                                Container2< constant_value_container<CrossSelector> >,
                                                Hidden< Minor >,
                                                Operation< SliceConstructor > ) > {
   typedef modified_container_pair_impl<RowsCols> _super;
protected:
   ~RowsCols();
public:
   const typename _super::container2& get_container2() const
   {
      return constant(this->hidden().get_subset_alias(int2type<3-dir>()));
   }
};

template <typename Minor, typename RenumberVal, int dir, typename SliceConstructor>
class RowsCols<Minor, RenumberVal, dir, SliceConstructor, const all_selector&>
   : public RowColSubset<Minor, RenumberVal, dir> {
protected:
   ~RowsCols();
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
class Rows< MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef> >
   : public RowsCols< minor_base<MatrixRef, RowIndexSetRef, ColIndexSetRef>, True, 1,
                      operations::construct_binary2<IndexedSlice,void> > {
protected:
   ~Rows();
};

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef>
class Cols< MatrixMinor<MatrixRef, RowIndexSetRef, ColIndexSetRef> >
   : public RowsCols< minor_base<MatrixRef, RowIndexSetRef, ColIndexSetRef>, True, 2,
                      operations::construct_binary2<IndexedSlice,void> > {
protected:
   ~Cols();
};

/* ----------
 *  RowChain
 * ---------- */

template <typename MatrixRef1, typename MatrixRef2>
class RowChain
   : public container_pair_base<MatrixRef1, MatrixRef2>,
     public inherit_generic< RowChain<MatrixRef1,MatrixRef2>,
                             cons<typename deref<MatrixRef1>::type, typename deref<MatrixRef2>::type> >::type {

   typedef container_pair_base<MatrixRef1, MatrixRef2> _base;
   typedef typename deref<MatrixRef1>::type matrix1_type;
   typedef typename deref<MatrixRef2>::type matrix2_type;
public:
   typedef typename identical<typename container_traits<MatrixRef1>::value_type,
                              typename container_traits<MatrixRef2>::value_type>::type
      value_type;
   typedef typename compatible<typename container_traits<MatrixRef1>::reference,
                               typename container_traits<MatrixRef2>::reference>::type
      reference;
   typedef typename compatible<typename container_traits<MatrixRef1>::const_reference,
                               typename container_traits<MatrixRef2>::const_reference>::type
      const_reference;

   RowChain(typename _base::first_arg_type m1, typename _base::second_arg_type m2)
      : _base(m1,m2)
   {
      const int c1=m1.cols(), c2=m2.cols();
      if (c1) {
         if (c2) {
            if (c1!=c2) throw std::runtime_error("block matrix - different number of columns");
         } else {
            this->src2.get_object().stretch_cols(c1);
         }
      } else if (c2) {
         this->src1.get_object().stretch_cols(c2);
      }
   }

   RowChain& operator= (const RowChain& other) { return RowChain::generic_type::operator=(other); }
   using RowChain::generic_type::operator=;
};

template <typename MatrixRef1, typename MatrixRef2>
struct spec_object_traits< RowChain<MatrixRef1, MatrixRef2> >
   : spec_object_traits<is_container> {
   static const bool
      is_temporary = true,
      is_lazy = object_traits<typename deref<MatrixRef1>::type>::is_lazy || object_traits<typename deref<MatrixRef2>::type>::is_lazy,
      is_always_const = effectively_const<MatrixRef1>::value || effectively_const<MatrixRef2>::value;
};

template <typename MatrixRef1, typename MatrixRef2>
struct check_container_feature< RowChain<MatrixRef1, MatrixRef2>, sparse> {
   static const bool value=check_container_ref_feature<MatrixRef1, sparse>::value ||
                           check_container_ref_feature<MatrixRef2, sparse>::value;
};

template <typename MatrixRef1, typename MatrixRef2>
struct check_container_feature< RowChain<MatrixRef1, MatrixRef2>, pure_sparse> {
   static const bool value=check_container_ref_feature<MatrixRef1, pure_sparse>::value &&
                           check_container_ref_feature<MatrixRef2, pure_sparse>::value;
};

template <typename MatrixRef1, typename MatrixRef2>
struct check_container_feature< RowChain<MatrixRef1, MatrixRef2>, FlatStorage> {
   static const bool value=check_container_ref_feature<MatrixRef1, FlatStorage>::value &&
                           check_container_ref_feature<MatrixRef2, FlatStorage>::value;
};

template <typename MatrixRef1, typename MatrixRef2>
class matrix_random_access_methods< RowChain<MatrixRef1, MatrixRef2> > {
   typedef RowChain<MatrixRef1,MatrixRef2> master;
public:
   typename compatible<typename container_traits<MatrixRef1>::reference,
                       typename container_traits<MatrixRef2>::reference>::type
   operator() (int i, int j)
   {
      master& me=static_cast<master&>(*this);
      const int r1=me.get_container1().rows();
      if (i < r1) return me.get_container1()(i,j);
      return me.get_container2()(i-r1,j);
   }
   typename compatible<typename container_traits<MatrixRef1>::const_reference,
                       typename container_traits<MatrixRef2>::const_reference>::type
   operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      const int r1=me.get_container1().rows();
      if (i < r1) return me.get_container1()(i,j);
      return me.get_container2()(i-r1,j);
   }
};

template <typename MatrixRef1, typename MatrixRef2>
class Rows< RowChain<MatrixRef1, MatrixRef2> >
   : public container_chain_impl< Rows< RowChain<MatrixRef1,MatrixRef2> >,
                                  list( Container1< masquerade<pm::Rows,MatrixRef1> >,
                                        Container2< masquerade<pm::Rows,MatrixRef2> >,
                                        MasqueradedTop ) > {
   typedef container_chain_impl<Rows> _super;
protected:
   ~Rows();
public:
   typename _super::container1& get_container1()
   {
      return rows(this->hidden().get_container1());
   }
   typename _super::container2& get_container2()
   {
      return rows(this->hidden().get_container2());
   }
   const typename _super::container1& get_container1() const
   {
      return rows(this->hidden().get_container1());
   }
   const typename _super::container2& get_container2() const
   {
      return rows(this->hidden().get_container2());
   }
   int size() const
   {
      return get_container1().size() + get_container2().size();
   }
};

template <typename MatrixRef1, typename MatrixRef2>
class Cols< RowChain<MatrixRef1,MatrixRef2> >
   : public modified_container_pair_impl< Cols< RowChain<MatrixRef1,MatrixRef2> >,
                                          list( Container1< masquerade<pm::Cols,MatrixRef1> >,
                                                Container2< masquerade<pm::Cols,MatrixRef2> >,
                                                Operation< BuildBinary<operations::concat> >,
                                                MasqueradedTop ) > {
   typedef modified_container_pair_impl<Cols> _super;
protected:
   ~Cols();
public:
   typename _super::container1& get_container1()
   {
      return cols(this->hidden().get_container1());
   }
   typename _super::container2& get_container2()
   {
      return cols(this->hidden().get_container2());
   }
   const typename _super::container1& get_container1() const
   {
      return cols(this->hidden().get_container1());
   }
   const typename _super::container2& get_container2() const
   {
      return cols(this->hidden().get_container2());
   }
   int size() const
   {
      int c=get_container1().size();
      if (!c) c=get_container2().size();
      return c;
   }
};

/* ----------
 *  ColChain
 * ---------- */

template <typename MatrixRef1, typename MatrixRef2>
class ColChain
   : public container_pair_base<MatrixRef1, MatrixRef2>,
     public inherit_generic<ColChain<MatrixRef1,MatrixRef2>,
                            cons<typename deref<MatrixRef1>::type, typename deref<MatrixRef2>::type> >::type {

   typedef container_pair_base<MatrixRef1, MatrixRef2> _base;
   typedef typename deref<MatrixRef1>::type matrix1_type;
   typedef typename deref<MatrixRef2>::type matrix2_type;
public:
   typedef typename identical<typename container_traits<MatrixRef1>::value_type,
                              typename container_traits<MatrixRef2>::value_type>::type
      value_type;
   typedef typename compatible<typename container_traits<MatrixRef1>::reference,
                               typename container_traits<MatrixRef2>::reference>::type
      reference;
   typedef typename compatible<typename container_traits<MatrixRef1>::const_reference,
                               typename container_traits<MatrixRef2>::const_reference>::type
      const_reference;

   ColChain(typename _base::first_arg_type m1, typename _base::second_arg_type m2)
      : _base(m1,m2)
   {
      const int r1=m1.rows(), r2=m2.rows();
      if (r1) {
         if (r2) {
            if (r1!=r2) throw std::runtime_error("block matrix - different number of rows");
         } else {
            this->src2.get_object().stretch_rows(r1);
         }
      } else if (r2) {
         this->src1.get_object().stretch_rows(r2);
      }
   }

   ColChain& operator= (const ColChain& other) { return ColChain::generic_type::operator=(other); }
   using ColChain::generic_type::operator=;
};

template <typename MatrixRef1, typename MatrixRef2>
struct spec_object_traits< ColChain<MatrixRef1, MatrixRef2> >
   : spec_object_traits< RowChain<MatrixRef1, MatrixRef2> > {};

template <typename MatrixRef1, typename MatrixRef2>
struct check_container_feature<ColChain<MatrixRef1, MatrixRef2>, sparse> {
   static const bool value=check_container_ref_feature<MatrixRef1, sparse>::value ||
                           check_container_ref_feature<MatrixRef2, sparse>::value;
};

template <typename MatrixRef1, typename MatrixRef2>
struct check_container_feature<ColChain<MatrixRef1, MatrixRef2>, pure_sparse> {
   static const bool value=check_container_ref_feature<MatrixRef1, pure_sparse>::value &&
                           check_container_ref_feature<MatrixRef2, pure_sparse>::value;
};

template <typename MatrixRef1, typename MatrixRef2>
class matrix_random_access_methods< ColChain<MatrixRef1, MatrixRef2> > {
   typedef ColChain<MatrixRef1,MatrixRef2> master;
public:
   typename compatible<typename container_traits<MatrixRef1>::reference,
                       typename container_traits<MatrixRef2>::reference>::type
   operator() (int i, int j)
   {
      master& me=static_cast<master&>(*this);
      const int c1=me.get_container1().cols();
      if (j < c1) return me.get_container1()(i,j);
      return me.get_container2()(i,j-c1);
   }
   typename compatible<typename container_traits<MatrixRef1>::const_reference,
                       typename container_traits<MatrixRef2>::const_reference>::type
   operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      const int c1=me.get_container1().cols();
      if (j < c1) return me.get_container1()(i,j);
      return me.get_container2()(i,j-c1);
   }
};

template <typename MatrixRef1, typename MatrixRef2>
class Rows< ColChain<MatrixRef1,MatrixRef2> >
   : public modified_container_pair_impl< Rows< ColChain<MatrixRef1,MatrixRef2> >,
                                          list( Container1< masquerade<pm::Rows,MatrixRef1> >,
                                                Container2< masquerade<pm::Rows,MatrixRef2> >,
                                                Operation< BuildBinary<operations::concat> >,
                                                MasqueradedTop ) > {
   typedef modified_container_pair_impl<Rows> _super;
protected:
   ~Rows();
public:
   typename _super::container1& get_container1()
   {
      return rows(this->hidden().get_container1());
   }
   typename _super::container2& get_container2()
   {
      return rows(this->hidden().get_container2());
   }
   const typename _super::container1& get_container1() const
   {
      return rows(this->hidden().get_container1());
   }
   const typename _super::container2& get_container2() const
   {
      return rows(this->hidden().get_container2());
   }
   int size() const
   {
      int r=get_container1().size();
      if (!r) r=get_container2().size();
      return r;
   }
};

template <typename MatrixRef1, typename MatrixRef2>
class Cols< ColChain<MatrixRef1,MatrixRef2> >
   : public container_chain_impl< Cols< ColChain<MatrixRef1,MatrixRef2> >,
                                  list( Container1< masquerade<pm::Cols,MatrixRef1> >,
                                        Container2< masquerade<pm::Cols,MatrixRef2> >,
                                        MasqueradedTop ) > {
   typedef container_chain_impl<Cols> _super;
protected:
   ~Cols();
public:
   typename _super::container1& get_container1()
   {
      return cols(this->hidden().get_container1());
   }
   typename _super::container2& get_container2()
   {
      return cols(this->hidden().get_container2());
   }
   const typename _super::container1& get_container1() const
   {
      return cols(this->hidden().get_container1());
   }
   const typename _super::container2& get_container2() const
   {
      return cols(this->hidden().get_container2());
   }
   int size() const
   {
      return get_container1().size() + get_container2().size();
   }
};

/* ------------------------------
 *  base for SingleRow, SingleCol
 * ------------------------------ */

template <typename LineRef>
class single_line_matrix {
protected:
   single_value_container<LineRef> _line;
   static const bool is_always_const=object_traits< single_value_container<LineRef> >::is_always_const;
public:
   typedef typename single_value_container<LineRef>::arg_type arg_type;
   single_line_matrix(arg_type arg) : _line(arg) {}

   typename single_value_container<LineRef>::reference get_line() { return _line.front(); }
   typename single_value_container<LineRef>::const_reference get_line() const { return _line.front(); }
};

} // end namespace pm

namespace polymake {
   using pm::Transposed;
   using pm::All;
   using pm::Symmetric;
   using pm::NonSymmetric;
   using pm::SkewSymmetric;
}

namespace std {

// due to silly overloading rules
template <typename Matrix> inline
void swap(pm::Transposed<Matrix>& m1, pm::Transposed<Matrix>& m2)
{
   m1.swap(m2);
}

template <typename MatrixRef, typename RowIndexSetRef, typename ColIndexSetRef> inline
void swap(pm::MatrixMinor<MatrixRef,RowIndexSetRef,ColIndexSetRef>& m1,
          pm::MatrixMinor<MatrixRef,RowIndexSetRef,ColIndexSetRef>& m2)
{
   m1.swap(m2);
}

template <typename MatrixRef1, typename MatrixRef2> inline
void swap(pm::RowChain<MatrixRef1,MatrixRef2>& m1, pm::RowChain<MatrixRef1,MatrixRef2>& m2)
{
   m1.swap(m2);
}

template <typename MatrixRef1, typename MatrixRef2> inline
void swap(pm::ColChain<MatrixRef1,MatrixRef2>& m1, pm::ColChain<MatrixRef1,MatrixRef2>& m2)
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
