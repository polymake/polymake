/* Copyright (c) 1997-2020
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

#ifndef POLYMAKE_INTERNAL_INCIDENCE_AND_SPARSE_MATRIX_H
#define POLYMAKE_INTERNAL_INCIDENCE_AND_SPARSE_MATRIX_H

namespace pm {

template <typename IncMatrixRef, typename ElemRef>
class SameElementSparseMatrix
   : public GenericMatrix< SameElementSparseMatrix<IncMatrixRef, ElemRef>,
                           typename object_traits<typename deref<ElemRef>::type>::persistent_type > {
protected:
   using matrix_alias_t = alias<IncMatrixRef>;
   using elem_alias_t = alias<ElemRef>;
   matrix_alias_t matrix;
   elem_alias_t apparent_elem;

public:
   using value_type = typename deref<ElemRef>::type;
   using const_reference = const value_type&;
   using reference = const_reference;

   template <typename Arg1, typename Arg2,
             typename=std::enable_if_t<std::is_constructible<matrix_alias_t, Arg1>::value &&
                                       std::is_constructible<elem_alias_t, Arg2>::value>>
   SameElementSparseMatrix(Arg1&& matrix_arg, Arg2&& data_arg)
      : matrix(std::forward<Arg1>(matrix_arg))
      , apparent_elem(std::forward<Arg2>(data_arg)) {}

   decltype(auto) get_matrix() const { return *matrix; }
   const elem_alias_t& get_apparent_element() const { return apparent_elem; }
};

template <typename IncMatrixRef, typename ElemRef>
struct spec_object_traits< SameElementSparseMatrix<IncMatrixRef, ElemRef> >
   : spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = true;
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
   const typename deref<ElemRef>::type& operator() (Int i, Int j) const
   {
      const master& me=static_cast<const master&>(*this);
      if (me.get_matrix()(i,j)) return *me.get_apparent_element();
      return zero_value<typename deref<ElemRef>::type>();
   }
};

template <typename IncMatrixRef, typename ElemRef>
class Rows< SameElementSparseMatrix<IncMatrixRef, ElemRef> >
   : public modified_container_pair_impl< Rows< SameElementSparseMatrix<IncMatrixRef, ElemRef> >,
                                          mlist< Container1RefTag< masquerade<Rows, IncMatrixRef> >,
                                                 Container2RefTag< same_value_container<ElemRef> >,
                                                 OperationTag< operations::construct_binary<SameElementSparseVector> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Rows> base_t;
public:
   decltype(auto) get_container1() const
   {
      return rows(this->hidden().get_matrix());
   }
   decltype(auto) get_container2() const
   {
      return as_same_value_container(this->hidden().get_apparent_element());
   }
};

template <typename IncMatrixRef, typename ElemRef>
class Cols< SameElementSparseMatrix<IncMatrixRef, ElemRef> >
   : public modified_container_pair_impl< Cols< SameElementSparseMatrix<IncMatrixRef,ElemRef> >,
                                          mlist< Container1RefTag< masquerade<Cols, IncMatrixRef> >,
                                                 Container2RefTag< same_value_container<ElemRef> >,
                                                 OperationTag< operations::construct_binary<SameElementSparseVector> >,
                                                 MasqueradedTop > > {
   typedef modified_container_pair_impl<Cols> base_t;
public:
   decltype(auto) get_container1() const
   {
      return cols(this->hidden().get_matrix());
   }
   decltype(auto) get_container2() const
   {
      return as_same_value_container(this->hidden().get_apparent_element());
   }
};

template <typename E, typename TMatrix>
auto same_element_sparse_matrix(TMatrix&& m, E&& x, std::enable_if_t<is_generic_incidence_matrix<TMatrix>::value, void**> =nullptr)
{
   return SameElementSparseMatrix<add_const_t<unwary_t<TMatrix&&>>, prevent_int_element<E&&>>(unwary(std::forward<TMatrix>(m)), std::forward<E>(x));
}

template <typename E, typename TMatrix>
auto same_element_sparse_matrix(TMatrix&& m, std::enable_if_t<is_generic_incidence_matrix<TMatrix>::value, void**> =nullptr)
{
   return same_element_sparse_matrix(std::forward<TMatrix>(m), one_value<E>(), nullptr);
}

template <typename MatrixRef>
class IndexMatrix
   : public GenericIncidenceMatrix< IndexMatrix<MatrixRef> > {
protected:
   using alias_t = alias<MatrixRef>;
   alias_t matrix;
public:
   using value_type = bool;
   using reference = bool;
   using const_reference = bool;

   template <typename Arg, typename=std::enable_if_t<std::is_constructible<alias_t, Arg>::value>>
   explicit IndexMatrix(Arg&& matrix_arg)
      : matrix(std::forward<Arg>(matrix_arg)) {}

   decltype(auto) get_matrix() const { return *matrix; }
};

template <typename MatrixRef>
struct check_container_feature< IndexMatrix<MatrixRef>, Symmetric >
   : check_container_ref_feature<MatrixRef, Symmetric> {};

template <typename MatrixRef>
struct spec_object_traits< IndexMatrix<MatrixRef> >
   : spec_object_traits<is_container> {
   static constexpr bool is_temporary = true, is_always_const = true;
};

template <typename MatrixRef>
class matrix_random_access_methods< IndexMatrix<MatrixRef> > {
   typedef IndexMatrix<MatrixRef> master;
public:
   bool operator() (Int i, Int j) const
   {
      const master& me = static_cast<const master&>(*this);
      return !me.row(i).find(j).at_end();
   }
};

template <typename MatrixRef>
class Rows< IndexMatrix<MatrixRef> >
   : public modified_container_impl< Rows< IndexMatrix<MatrixRef> >,
                                     mlist< MasqueradedTop,
                                            ContainerRefTag< masquerade<pm::Rows, MatrixRef> >,
                                            OperationTag< operations::construct_unary<Indices> > > > {
protected:
   Rows();
   ~Rows();
public:
   decltype(auto) get_container() const { return rows(this->hidden().get_matrix()); }
};

template <typename MatrixRef>
class Cols< IndexMatrix<MatrixRef> >
   : public modified_container_impl< Cols< IndexMatrix<MatrixRef> >,
                                     mlist< MasqueradedTop,
                                            ContainerRefTag< masquerade<pm::Cols, MatrixRef> >,
                                            OperationTag< operations::construct_unary<Indices> > > > {
protected:
   Cols();
   ~Cols();
public:
   decltype(auto) get_container() const { return cols(this->hidden().get_matrix()); }
};

template <typename TMatrix, typename=std::enable_if_t<check_container_feature<TMatrix, sparse>::value>>
auto index_matrix(const GenericMatrix<TMatrix>& m)
{
   return IndexMatrix<const TMatrix&>(m.top());
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
