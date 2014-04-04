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

#ifndef POLYMAKE_LIST_INCIDENCE_MATRIX_H
#define POLYMAKE_LIST_INCIDENCE_MATRIX_H

#include "polymake/GenericIncidenceMatrix.h"
#include "polymake/TransformedContainer.h"
#include "polymake/internal/CombArray.h"

namespace pm {

template <typename Set>
class ListIncidenceMatrix
   : public GenericIncidenceMatrix< ListIncidenceMatrix<Set>, False> {
protected:
   typedef std::list<Set> row_list;
   shared_object< ListMatrix_data<Set>, AliasHandler<shared_alias_handler> > data;

   friend ListIncidenceMatrix& make_mutable_alias(ListIncidenceMatrix& alias, ListIncidenceMatrix& owner)
   {
      alias.data.make_mutable_alias(owner.data);
      return alias;
   }

   // from a dense sequence of booleans
   template <typename Iterator>
   void _copy(int r, int c, Iterator src, int2type<0>)
   {
      data->dimr=r;
      data->dimc=c;
      row_list& R=data->R;
      while (--r>=0) {
         R.push_back(Set());
         for (int i=0; i<c; ++i, ++src)
            if (*src) R.back().push_back(i);
      }
   }

   // from a sequence of sets
   template <typename Iterator>
   void _copy(int r, int c, Iterator src, int2type<1>)
   {
      data->dimr=r;
      data->dimc=c;
      row_list& R=data->R;
      while (--r>=0) {
         R.push_back(Set(*src)); ++src;
      }
   }

public:
   typedef bool value_type;
   typedef bool reference;
   typedef const bool const_reference;

   ListIncidenceMatrix() {}

   ListIncidenceMatrix(int r, int c)
   {
      data->dimr=r;
      data->dimc=c;
      data->R.assign(r, Set());
   }

   template <typename Iterator>
   ListIncidenceMatrix(int r, int c, Iterator src)
   {
      _copy(r, c, src, int2type<object_traits<typename iterator_traits<Iterator>::value_type>::total_dimension>());
   }

   template <typename Matrix2>
   ListIncidenceMatrix(const GenericIncidenceMatrix<Matrix2>& M)
   {
      _copy(M.rows(), M.cols(), pm::rows(M).begin(), int2type<1>());
   }

   template <typename Container>
   ListIncidenceMatrix(const Container& src,
                       typename enable_if<void**, isomorphic_to_container_of<Container, Set>::value>::type=0)
   {
      _copy(src.size(), src.empty() ? 0 : get_dim(src.front()), src.begin(), int2type<1>());
   }

private:
   template <typename Input>
   void input(Input& is)
   {
      row_list& R=data->R;
      data->dimr=retrieve_container(is, R, io_test::as_list< array_traits<Set> >());
      int c=0;
      for (typename Entire<row_list>::iterator row=entire(R); !row.at_end(); ++row)
         if (!row->empty()) assign_max(c, row->back());
      data->dimc=c;
   }

public:
   template <typename Input> friend
   Input& operator>> (GenericInput<Input>& is, ListIncidenceMatrix& M)
   {
      M.input(is.top());
      return is.top();
   }

   ListIncidenceMatrix& operator= (const ListIncidenceMatrix& other) { assign(other); return *this; }
#ifdef __clang__
   template <typename Matrix2>
   ListIncidenceMatrix& operator= (const GenericIncidenceMatrix<Matrix2>& other) { return ListIncidenceMatrix::generic_type::operator=(other); }
#else
   using ListIncidenceMatrix::generic_type::operator=;
#endif

   void swap(ListIncidenceMatrix& M) { data.swap(M.data); }

   friend void relocate(ListIncidenceMatrix* from, ListIncidenceMatrix* to)
   {
      relocate(&from->data, &to->data);
   }

   void resize(int r, int c)
   {
      row_list& R=data->R;
      int old_r=data->dimr;
      data->dimr=r;
      for (; old_r>r; --old_r) R.pop_back();

      if (data->dimc > c) {
         const sequence strip_off=range(c,data->dimc);
         for (typename Entire<row_list>::iterator row=entire(R); !row.at_end(); ++row)
            *row -= strip_off;
      }
      data->dimc=c;

      for (; old_r<r; ++old_r) R.push_back(Set());
   }

   void clear() { data.apply(shared_clear()); }

   int rows() const { return data->dimr; }
   int cols() const { return data->dimc; }

   template <typename Matrix2>
   ListIncidenceMatrix& operator/= (const GenericIncidenceMatrix<Matrix2>& m)
   {
      const int add_rows=M.rows();
      if (add_rows) {
         if (!this->rows()) {
            data->dimc=M.cols();
         } else if (POLYMAKE_DEBUG || !Unwary<Matrix2>::value) {
            if (this->cols() != M.cols())
               throw std::runtime_error("ListIncidenceMatrix::operator/= - dimension mismatch");
         }
         copy(entire(pm::rows(M)), std::back_inserter(data->R));
         data->dimr+=add_rows;
      }
      return *this;
   }

   template <typename Set2>
   ListIncidenceMatrix& operator/= (const GenericSet<Set2, int, operations::cmp>& s)
   {
      if (!s.top().empty()) {
         if (check_container_feature<Set2, sparse_compatible>::value) {
            if (POLYMAKE_DEBUG || !Unwary<Set2>::value) {
               if (this->cols() != get_dim(s.top()))
                  throw std::runtime_error("ListIncidenceMatrix::operator/= - dimension mismatch");
            }
         } else {
            assign_max(data->dimc, s.top().back()+1);
         }
      }
      data->R.push_back(s.top());
      ++data->dimr;
      return *this;
   }

   template <typename Matrix2>
   ListIncidenceMatrix& operator|= (const GenericIncidenceMatrix<Matrix2>& M)
   {
      const int add_cols=M.cols();
      if (add_cols) {
         if (POLYMAKE_DEBUG || !Unwary<Matrix2>::value) {
            if (this->rows() != M.rows())
               throw std::runtime_error("ListIncidenceMatrix::operator|= - dimension mismatch");
         }
         const int old_c=data->dimc;
         typename Rows<Matrix2>::const_iterator row2=pm::rows(M).begin();
         for (typename Entire<row_list>::iterator row=entire(data->R); !row.at_end(); ++row, ++row2)
            copy(entire(translate(*row2, old_c)), std::back_inserter(*row));
         data->dimc+=add_cols;
      }
      return *this;
   }

   template <typename Set2>
   ListIncidenceMatrix& operator|= (const GenericSet<Set2, int, operations::cmp>& s)
   {
      if (check_container_feature<Set2, sparse_compatible>::value) {
         if (POLYMAKE_DEBUG || !Unwary<Set2>::value) {
            if (this->rows() != get_dim(s.top()))
               throw std::runtime_error("ListIncidenceMatrix::operator|= - dimension mismatch");
         }
      }
      row_list& R=data->R;
      typename row_list::iterator row=R.begin();
      int old_r=data->dimr, old_c=data->dimc;
      int prev_row=0;
      for (typename Entire<Set2>::const_iterator e=entire(s.top()); !e.at_end(); ++e) {
         if (!check_container_feature<Set2, sparse_compatible>::value && *e >= old_r) {
            do {
               int new_r=*e+1;
               for (; old_r < new_r; ++old_r)
                  R.push_back(Set());
               R.back().push_back(old_c);
            } while (!(++e).at_end());
            data->dimr=old_r;
            break;
         }
         std::advance(row, *e-prev_row);
         row->push_back(old_c);
         prev_row=*e;
      }
      ++data->dimc;
      return *this;
   }

   template <typename Set2>
   typename row_list::iterator
   insert_row(const typename row_list::iterator& where, const GenericSet<Set2, int, operations::cmp>& s)
   {
      if (!s.top.empty()) {
         if (check_container_feature<Set2, sparse_compatible>::value) {
            if (POLYMAKE_DEBUG || !Unwary<Set2>::value) {
               if (this->cols() != get_dim(s.top()))
                  throw std::runtime_error("ListIncidenceMatrix::insert_row - dimension mismatch");
            }
         } else {
            assign_max(data->dimc, s.top().back()+1);
         }
      }
      ++data->dimr;
      return data->R.insert(where,s.top());
   }

   void delete_row(const typename row_list::iterator& where)
   {
      --data->dimr;
      data->R.erase(where);
   }
protected:
   friend class GenericIncidenceMatrix<ListIncidenceMatrix>;
   friend class Rows<ListIncidenceMatrix>;
   template <typename,typename,typename> friend class ListMatrix_cols;

   void assign(const GenericIncidenceMatrix<ListIncidenceMatrix>& M) { data=M.top().data; }

   template <typename Matrix2>
   void assign(const GenericIncidenceMatrix<Matrix2>& M)
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
};

template <typename Set>
class Rows< ListIncidenceMatrix<Set> >
   : public redirected_container< Rows< ListIncidenceMatrix<Set> >,
                                  list( Container< std::list<Set> >,
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

   template <typename Set2>
   typename _super::iterator insert(typename _super::iterator where, const GenericSet<Set2>& s)
   {
      return this->hidden().insert_row(where,s);
   }
};

template <typename Set>
class Cols< ListIncidenceMatrix<Set> >
   : public ListMatrix_cols<ListIncidenceMatrix<Set>, Set> {
protected:
   Cols();
   ~Cols();
};

} // end namespace pm

namespace polymake {
   using pm::ListIncidenceMatrix;
}

#endif // POLYMAKE_LIST_MATRIX_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
