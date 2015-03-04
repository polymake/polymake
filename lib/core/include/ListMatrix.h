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

/** @file ListMatrix.h
    @brief Implementation of pm::ListMatrix class
 */

#ifndef POLYMAKE_LIST_MATRIX_H
#define POLYMAKE_LIST_MATRIX_H

#include "polymake/GenericMatrix.h"
#include "polymake/internal/CombArray.h"

namespace pm {

/** @class ListMatrix
    @brief List of row vectors. 

   A list of row vectors. The implementation is based on std::list. It can be parameterized with any @ref persistent vector type.
*/

template <typename Vector>
class ListMatrix : public GenericMatrix< ListMatrix<Vector>, typename Vector::element_type> {
protected:
   typedef std::list<Vector> row_list;
   shared_object< ListMatrix_data<Vector>, AliasHandler<shared_alias_handler> > data;

   friend ListMatrix& make_mutable_alias(ListMatrix& alias, ListMatrix& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }

   // elementwise
   template <typename Iterator>
   void _copy(int r, int c, Iterator src, False)
   {
      data->dimr=r;
      data->dimc=c;
      row_list& R=data->R;
      while (--r>=0) {
         Iterator src_next=src;
         std::advance(src_next,c);
         R.push_back(Vector(c, make_iterator_range(src,src_next)));
         src=src_next;
      }
   }

   // rowwise
   template <typename Iterator>
   void _copy(int r, int c, Iterator src, True)
   {
      data->dimr=r;
      data->dimc=c;
      row_list& R=data->R;
      while (--r>=0) {
         R.push_back(Vector(*src)); ++src;
      }
   }

public:
   typedef typename Vector::value_type value_type;
   typedef typename Vector::reference reference;
   typedef typename Vector::const_reference const_reference;
   typedef typename Vector::element_type element_type;

   /// create as empty
   ListMatrix() {}

   /// create matrix with r rows and c columns, initialize all elements to 0
   ListMatrix(int r, int c)
   {
      data->dimr=r;
      data->dimc=c;
      data->R.assign(r, Vector(c));
   }

   ListMatrix(int r, int c, typename function_argument<element_type>::type init)
   {
      data->dimr=r;
      data->dimc=c;
      data->R.assign(r, same_element_vector(init,c));
   }

   /** Create a matrix with given dimensions.  Elements are initialized from an input sequence.
       Elements are assumed to come in the row order.
   */
   template <typename Iterator>
   ListMatrix(int r, int c, Iterator src)
   {
      _copy(r, c, src, bool2type<isomorphic_types<Vector, typename iterator_traits<Iterator>::value_type>::value>());
   }

   template <typename Matrix2>
   ListMatrix(const GenericMatrix<Matrix2,element_type>& M)
   {
      _copy(M.rows(), M.cols(), pm::rows(M).begin(), True());
   }

   template <typename Matrix2, typename E2>
   explicit ListMatrix(const GenericMatrix<Matrix2,E2>& M)
   {
      _copy(M.rows(), M.cols(), pm::rows(M).begin(), True());
   }

   template <typename Container>
   ListMatrix(const Container& src,
              typename enable_if<void**, isomorphic_to_container_of<Container, Vector>::value>::type=0)
   {
      _copy(src.size(), src.empty() ? 0 : get_dim(src.front()), src.begin(), True());
   }

private:
   template <typename Input>
   void input(Input& is)
   {
      if ((data->dimr=retrieve_container(is, data->R, io_test::as_list< array_traits<Vector> >())))
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
#ifdef __clang__
   template <typename Matrix2>
   typename ListMatrix::generic_type::template enable_if_assignable_from<Matrix2>::type&
   operator= (const GenericMatrix<Matrix2>& other) { return ListMatrix::generic_type::operator=(other); }
#else
   using ListMatrix::generic_type::operator=;
#endif

   /// Exchange the contents of two matrices in a most efficient way. 
   /// If at least one non-persistent object is involved, the operands must have equal dimensions. 
   void swap(ListMatrix& M) { data.swap(M.data); }

   friend void relocate(ListMatrix* from, ListMatrix* to)
   {
      relocate(&from->data, &to->data);
   }

   /// Resize to new dimensions, added elements initialized with default constructor.
   void resize(int r, int c)
   {
      row_list& R=data->R;
      int old_r=data->dimr;
      data->dimr=r;
      for (; old_r>r; --old_r) R.pop_back();

      if (data->dimc != c) {
         for (typename Entire<row_list>::iterator row=entire(R); !row.at_end(); ++row)
            row->resize(c);
         data->dimc=c;
      }

      for (; old_r<r; ++old_r) R.push_back(Vector(c));
   }

   /// Truncate to 0x0 matrix.
   void clear() { data.apply(shared_clear()); }

   /// the number of rows of the matrix
   int rows() const { return data->dimr; }

   /// the number of columns of the matrix
   int cols() const { return data->dimc; }

   /// Insert a new row at the given position: before the row pointed to by where.
   template <typename Vector2>
   typename row_list::iterator
   insert_row(const typename row_list::iterator& where, const GenericVector<Vector2>& v)
   {
      if (!this->rows()) {
         data->dimc=v.dim();
      } else if (POLYMAKE_DEBUG || !Unwary<Vector2>::value) {
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
      int old_r=data->dimr, r=M.rows();
      data->dimr=r;
      data->dimc=M.cols();
      row_list& R=data->R;
      for (; old_r>r; --old_r) R.pop_back();

      typename Rows<Matrix2>::const_iterator row2=pm::rows(M).begin();
      for (typename Entire<row_list>::iterator row=entire(R); !row.at_end(); ++row, ++row2)
         *row=*row2;

      for (; old_r<r; ++old_r, ++row2) R.push_back(*row2);
   }

   template <typename Matrix2>
   void append_rows(const Matrix2& m)
   {
      copy(entire(pm::rows(m)), std::back_inserter(data->R));
      data->dimr+=m.rows();
   }

   template <typename Vector2>
   void append_row(const Vector2& v)
   {
      data->R.push_back(v);
      ++data->dimr;
   }

   template <typename Matrix2>
   void append_cols(const Matrix2& m)
   {
      typename Rows<Matrix2>::const_iterator row2=pm::rows(m).begin();
      for (typename Entire<row_list>::iterator row=entire(data->R); !row.at_end(); ++row, ++row2)
         *row |= *row2;
      data->dimc+=m.cols();
   }

   template <typename Vector2>
   void append_col(const Vector2& v)
   {
      typename ensure_features<Vector2, dense>::const_iterator
         e=ensure(v.top(), (dense*)0).begin();
      for (typename Entire<row_list>::iterator row=entire(data->R); !row.at_end(); ++row, ++e)
         *row |= *e;
      ++data->dimc;
   }
};

template <typename Vector>
struct check_container_feature<ListMatrix<Vector>, sparse>
   : check_container_feature<Vector, sparse> {};

template <typename Vector>
struct check_container_feature<ListMatrix<Vector>, pure_sparse>
   : check_container_feature<Vector, pure_sparse> {};

template <typename Vector>
class Rows< ListMatrix<Vector> >
   : public redirected_container< Rows< ListMatrix<Vector> >,
                                  list( Container< std::list<Vector> >,
                                        MasqueradedTop ) > {
   typedef redirected_container<Rows> _super;
protected:
   Rows();
   ~Rows();
public:
   typename _super::container& get_container() { return this->hidden().data->R; }
   const typename _super::container& get_container() const { return this->hidden().data->R; }

   int size() const { return this->hidden().rows(); }
   void resize(int n) { this->hidden().resize(n, this->hidden().cols()); }
   void clear() { this->hidden().clear(); }

   template <typename Vector2>
   typename _super::iterator insert(typename _super::iterator where, const GenericVector<Vector2>& v)
   {
      return this->hidden().insert_row(where,v);
   }
};

template <typename Vector>
class Cols< ListMatrix<Vector> >
   : public ListMatrix_cols<ListMatrix<Vector>, Vector> {
protected:
   Cols();
   ~Cols();
};

} // end namespace pm

namespace polymake {
   using pm::ListMatrix;
}

#endif // POLYMAKE_LIST_MATRIX_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
