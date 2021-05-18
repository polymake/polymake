/* Copyright (c) 1997-2021
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

#pragma once
/** @file ListMatrix.h
    @brief Implementation of pm::ListMatrix class
 */


#include "polymake/GenericMatrix.h"
#include "polymake/internal/CombArray.h"

namespace pm {

/** @class ListMatrix
    @brief List of row vectors. 

   A list of row vectors. The implementation is based on std::list. It can be parameterized with any @ref persistent vector type.
*/

template <typename TVector>
class ListMatrix
   : public GenericMatrix< ListMatrix<TVector>, typename TVector::element_type> {
protected:
   typedef std::list<TVector> row_list;
   shared_object<ListMatrix_data<TVector>, AliasHandlerTag<shared_alias_handler>> data;

   template <typename Iterator>
   struct fits_as_input_iterator
      : bool_constant<(assess_iterator_value<Iterator, can_initialize, typename TVector::element_type>::value ||
                       assess_iterator_value<Iterator, can_initialize, TVector>::value)> {};

   friend ListMatrix& make_mutable_alias(ListMatrix& alias, ListMatrix& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }

   // elementwise
   template <typename Iterator>
   void copy_impl(Int r, Int c, Iterator&& src, std::false_type)
   {
      data->dimr = r;
      data->dimc = c;
      row_list& R = data->R;
      while (--r >= 0) {
         Iterator src_next = src;
         std::advance(src_next, c);
         R.push_back(TVector(c, make_iterator_range(src, src_next)));
         src = src_next;
      }
   }

   // rowwise
   template <typename Iterator>
   void copy_impl(Int r, Int c, Iterator&& src, std::true_type)
   {
      data->dimr = r;
      data->dimc = c;
      row_list& R = data->R;
      while (--r >= 0) {
         R.push_back(TVector(*src)); ++src;
      }
   }

public:
   using value_type = typename TVector::value_type;
   using reference = typename TVector::reference;
   using const_reference = typename TVector::const_reference;
   using element_type = typename TVector::element_type;

   /// create as empty
   ListMatrix() {}

   /// create matrix with r rows and c columns, initialize all elements to 0
   ListMatrix(Int r, Int c)
   {
      data->dimr = r;
      data->dimc = c;
      data->R.assign(r, TVector(c));
   }

   ListMatrix(Int r, Int c, const element_type& init)
   {
      data->dimr = r;
      data->dimc = c;
      data->R.assign(r, same_element_vector(init, c));
   }

   /** Create a matrix with given dimensions.  Elements are initialized from an input sequence.
       Elements are assumed to come in the row order.
   */
   template <typename Iterator, typename=std::enable_if_t<fits_as_input_iterator<Iterator>::value>>
   ListMatrix(Int r, Int c, Iterator&& src)
   {
      copy_impl(r, c, ensure_private_mutable(std::forward<Iterator>(src)),
                bool_constant<isomorphic_types<TVector, typename iterator_traits<Iterator>::value_type>::value>());
   }

   template <typename Matrix2>
   ListMatrix(const GenericMatrix<Matrix2, element_type>& M)
   {
      copy_impl(M.rows(), M.cols(), pm::rows(M).begin(), std::true_type());
   }

   template <typename Matrix2, typename E2>
   explicit ListMatrix(const GenericMatrix<Matrix2, E2>& M)
   {
      copy_impl(M.rows(), M.cols(), pm::rows(M).begin(), std::true_type());
   }

   template <typename Container, typename=std::enable_if_t<isomorphic_to_container_of<Container, TVector>::value>>
   explicit ListMatrix(const Container& src)
   {
      copy_impl(src.size(), src.empty() ? 0 : get_dim(src.front()), src.begin(), std::true_type());
   }

private:
   template <typename Input>
   void input(Input& is)
   {
      if ((data->dimr=retrieve_container(is, data->R, io_test::as_list< array_traits<TVector> >())))
         data->dimc=data->R.front().dim();
   }

public:
   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& is, ListMatrix& M)
   {
      M.input(is.top());
      return is.top();
   }

   /**
      Persistent matrix objects have after the assignment the same dimensions as the right hand side operand. 
      Alias objects, such as matrix minor or block matrix, cannot be resized, thus must have the same dimensions as on the right hand side.
    */
   ListMatrix& operator= (const ListMatrix& other) { assign(other); return *this; }
   using ListMatrix::generic_type::operator=;

   /// Exchange the contents of two matrices in a most efficient way. 
   /// If at least one non-persistent object is involved, the operands must have equal dimensions. 
   void swap(ListMatrix& M) { data.swap(M.data); }

   friend void relocate(ListMatrix* from, ListMatrix* to)
   {
      relocate(&from->data, &to->data);
   }

   /// Resize to new dimensions, added elements initialized with default constructor.
   void resize(Int r, Int c)
   {
      row_list& R = data->R;
      Int old_r = data->dimr;
      data->dimr = r;
      for (; old_r > r; --old_r)
         R.pop_back();

      if (data->dimc != c) {
         for (auto row = entire(R); !row.at_end(); ++row)
            row->resize(c);
         data->dimc = c;
      }

      for (; old_r < r; ++old_r)
         R.push_back(TVector(c));
   }

   /// Truncate to 0x0 matrix.
   void clear() { data.apply(shared_clear()); }

   /// the number of rows of the matrix
   Int rows() const { return data->dimr; }

   /// the number of columns of the matrix
   Int cols() const { return data->dimc; }

   /// Insert a new row at the given position: before the row pointed to by where.
   template <typename TVector2>
   typename row_list::iterator
   insert_row(const typename row_list::iterator& where, const GenericVector<TVector2>& v)
   {
      if (!this->rows()) {
         data->dimc=v.dim();
      } else if (POLYMAKE_DEBUG || is_wary<TVector2>()) {
         if (this->cols() != v.dim())
            throw std::runtime_error("ListMatrix::insert_row - dimension mismatch");
      }
      ++data->dimr;
      return data->R.insert(where,v.top());
   }

   /// Delete the row pointed to by where. Be aware that the iterator becomes void after the removal unless rescued in good time (with postincrement or postdecrement). 
   void delete_row(const typename row_list::iterator& where)
   {
      --data->dimr;
      data->R.erase(where);
   }
protected:
   template <typename, typename> friend class GenericMatrix;
   friend class Rows<ListMatrix>;
   template <typename,typename,typename> friend class ListMatrix_cols;

   void assign(const GenericMatrix<ListMatrix>& M) { data=M.top().data; }

   template <typename Matrix2>
   void assign(const GenericMatrix<Matrix2>& M)
   {
      Int old_r = data->dimr, r = M.rows();
      data->dimr = r;
      data->dimc = M.cols();
      row_list& R = data->R;
      for (; old_r > r; --old_r)
         R.pop_back();

      auto row2 = pm::rows(M).begin();
      for (auto row = entire(R); !row.at_end(); ++row, ++row2)
         *row = *row2;

      for (; old_r < r; ++old_r, ++row2)
         R.push_back(*row2);
   }

   template <typename Matrix2>
   void append_rows(const Matrix2& m)
   {
      copy_range(entire(pm::rows(m)), std::back_inserter(data->R));
      data->dimr += m.rows();
   }

   template <typename TVector2>
   void append_row(const TVector2& v)
   {
      data->R.push_back(v);
      ++data->dimr;
   }

   template <typename Matrix2>
   void append_cols(const Matrix2& m)
   {
      auto row2 = pm::rows(m).begin();
      for (auto row = entire(data->R); !row.at_end(); ++row, ++row2)
         *row |= *row2;
      data->dimc += m.cols();
   }

   template <typename TVector2>
   void append_col(const TVector2& v)
   {
      auto e = ensure(v.top(), dense()).begin();
      for (auto row = entire(data->R); !row.at_end(); ++row, ++e)
         *row |= *e;
      ++data->dimc;
   }
};

template <typename TVector>
struct check_container_feature<ListMatrix<TVector>, sparse>
   : check_container_feature<TVector, sparse> {};

template <typename TVector>
struct check_container_feature<ListMatrix<TVector>, pure_sparse>
   : check_container_feature<TVector, pure_sparse> {};

template <typename TVector>
class Rows< ListMatrix<TVector> >
   : public redirected_container< Rows< ListMatrix<TVector> >,
                                  mlist< ContainerTag< std::list<TVector> >,
                                         MasqueradedTop > > {
   typedef redirected_container<Rows> base_t;
protected:
   Rows();
   ~Rows();
public:
   typename base_t::container& get_container() { return this->hidden().data->R; }
   const typename base_t::container& get_container() const { return this->hidden().data->R; }

   Int size() const { return this->hidden().rows(); }
   void resize(Int n) { this->hidden().resize(n, this->hidden().cols()); }
   void clear() { this->hidden().clear(); }

   template <typename TVector2>
   typename base_t::iterator insert(typename base_t::iterator where, const GenericVector<TVector2>& v)
   {
      return this->hidden().insert_row(where, v);
   }
};

template <typename TVector>
class Cols< ListMatrix<TVector> >
   : public ListMatrix_cols<ListMatrix<TVector>, TVector> {
protected:
   Cols();
   ~Cols();
};

} // end namespace pm

namespace polymake {
   using pm::ListMatrix;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
