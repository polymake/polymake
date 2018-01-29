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

#ifndef POLYMAKE_INTERNAL_INCIDENCE_AND_SPARSE_MATRIX_H
#define POLYMAKE_INTERNAL_INCIDENCE_AND_SPARSE_MATRIX_H

namespace pm {

template <typename IncMatrixRef, typename ElemRef>
class SameElementSparseMatrix
   : public GenericMatrix< SameElementSparseMatrix<IncMatrixRef, ElemRef>,
                           typename object_traits<typename deref<ElemRef>::type>::persistent_type > {
protected:
   alias<IncMatrixRef> matrix;
   alias<ElemRef> apparent_elem;

public:
   typedef typename alias<IncMatrixRef>::arg_type first_arg_type;
   typedef typename alias<ElemRef>::arg_type second_arg_type;

   typedef typename deref<ElemRef>::type value_type;
   typedef const value_type& const_reference;
   typedef const_reference reference;

   SameElementSparseMatrix(first_arg_type matrix_arg, second_arg_type data_arg)
      : matrix(matrix_arg), apparent_elem(data_arg) {}

   typename alias<IncMatrixRef>::const_reference get_matrix() const { return *matrix; }
   const alias<ElemRef>& get_apparent_element() const { return apparent_elem; }
};

template <typename IncMatrixRef, typename ElemRef>
struct spec_object_traits< SameElementSparseMatrix<IncMatrixRef, ElemRef> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename IncMatrixRef, typename ElemRef>
struct check_container_feature< SameElementSparseMatrix<IncMatrixRef, ElemRef>, Symmetric >
   : check_container_ref_feature< IncMatrixRef, Symmetric > {};

template <typename IncMatrixRef, typename ElemRef>
struct check_container_feature< SameElementSparseMatrix<IncMatrixRef, ElemRef>, pure_sparse> : std::true_type {};

template <typename IncMatrixRef, typename ElemRef>
class matrix_random_access_methods< SameElementSparseMatrix<IncMatrixRef, ElemRef> > {
   typedef SameElementSparseMatrix<IncMatrixRef, ElemRef> master;
public:
   const typename deref<ElemRef>::type& operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      if (me.get_matrix()(i,j)) return *me.get_apparent_element();
      return zero_value<typename deref<ElemRef>::type>();
   }
};

template <typename IncMatrixRef, typename ElemRef>
class Rows< SameElementSparseMatrix<IncMatrixRef, ElemRef> >
   : public modified_container_pair_impl< Rows< SameElementSparseMatrix<IncMatrixRef, ElemRef> >,
                                          mlist< Container1Tag< masquerade<Rows, IncMatrixRef> >,
                                                 Container2Tag< constant_value_container<ElemRef> >,
                                                 OperationTag< operations::construct_binary<SameElementSparseVector> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Rows> base_t;
public:
   const typename base_t::container1& get_container1() const
   {
      return rows(this->hidden().get_matrix());
   }
   const typename base_t::container2& get_container2() const
   {
      return constant(this->hidden().get_apparent_element());
   }
};

template <typename IncMatrixRef, typename ElemRef>
class Cols< SameElementSparseMatrix<IncMatrixRef, ElemRef> >
   : public modified_container_pair_impl< Cols< SameElementSparseMatrix<IncMatrixRef,ElemRef> >,
                                          mlist< Container1Tag< masquerade<Cols, IncMatrixRef> >,
                                                 Container2Tag< constant_value_container<ElemRef> >,
                                                 OperationTag< operations::construct_binary<SameElementSparseVector> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Cols> base_t;
public:
   const typename base_t::container1& get_container1() const
   {
      return cols(this->hidden().get_matrix());
   }
   const typename base_t::container2& get_container2() const
   {
      return constant(this->hidden().get_apparent_element());
   }
};

template <typename E, typename Matrix> inline
const SameElementSparseMatrix<const Matrix&, E>
same_element_sparse_matrix(const GenericIncidenceMatrix<Matrix>& m)
{
   return SameElementSparseMatrix<const Matrix&, E>(m.top(), E(1));
}

template <typename E, typename Matrix> inline
const SameElementSparseMatrix<const Matrix&, const E&>
same_element_sparse_matrix(const GenericIncidenceMatrix<Matrix>& m, const E& x)
{
   return SameElementSparseMatrix<const Matrix&, const E&>(m.top(), x);
}

template <typename MatrixRef>
class IndexMatrix
   : public GenericIncidenceMatrix< IndexMatrix<MatrixRef> > {
protected:
   alias<MatrixRef> m;
public:
   typedef bool value_type;
   typedef bool reference;
   typedef const bool const_reference;

   IndexMatrix(typename alias<MatrixRef>::arg_type m_arg) : m(m_arg) {}

   const typename deref<MatrixRef>::plus_const_ref get_matrix() const { return *m; }
};

template <typename MatrixRef>
struct check_container_feature< IndexMatrix<MatrixRef>, Symmetric >
   : check_container_ref_feature<MatrixRef, Symmetric> {};

template <typename MatrixRef>
struct spec_object_traits< IndexMatrix<MatrixRef> >
   : spec_object_traits<is_container> {
   static const bool is_temporary=true, is_always_const=true;
};

template <typename MatrixRef>
class matrix_random_access_methods< IndexMatrix<MatrixRef> > {
   typedef IndexMatrix<MatrixRef> master;
public:
   bool operator() (int i, int j) const
   {
      const master& me=static_cast<const master&>(*this);
      return !me.row(i).find(j).at_end();
   }
};

template <typename MatrixRef>
class Rows< IndexMatrix<MatrixRef> >
   : public modified_container_impl< Rows< IndexMatrix<MatrixRef> >,
                                     mlist< MasqueradedTop,
                                            ContainerTag< masquerade<pm::Rows, MatrixRef> >,
                                            OperationTag< operations::construct_unary<Indices> > > > {
   typedef modified_container_impl<Rows> base_t;
protected:
   Rows();
   ~Rows();
public:
   const typename base_t::container& get_container() const { return rows(this->hidden().get_matrix()); }
};

template <typename MatrixRef>
class Cols< IndexMatrix<MatrixRef> >
   : public modified_container_impl< Cols< IndexMatrix<MatrixRef> >,
                                     mlist< MasqueradedTop,
                                            ContainerTag< masquerade<pm::Cols, MatrixRef> >,
                                            OperationTag< operations::construct_unary<Indices> > > > {
   typedef modified_container_impl<Cols> base_t;
protected:
   Cols();
   ~Cols();
public:
   const typename base_t::container& get_container() const { return cols(this->hidden().get_matrix()); }
};

template <typename TMatrix> inline
typename std::enable_if<check_container_feature<TMatrix, sparse>::value, const IndexMatrix<const TMatrix&>>::type
index_matrix(const GenericMatrix<TMatrix>& m)
{
   return m.top();
}

} // end namespace pm

namespace polymake {
   using pm::same_element_sparse_matrix;
   using pm::index_matrix;
}

#endif // POLYMAKE_INTERNAL_INCIDENCE_AND_SPARSE_MATRIX_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
