/* Copyright (c) 1997-2023
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

#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/ListMatrix.h"
#include "polymake/Set.h"
#include "polymake/Rational.h"  // need this for algebraic_traits of Int and Integer

#include "polymake/internal/linalg_exceptions.h"
#include "polymake/internal/dense_linalg.h"
#include "polymake/internal/sparse_linalg.h"

namespace pm {

template <typename TVector>
std::enable_if_t<!check_container_feature<TVector, sparse>::value, bool>
is_leading_zero(const GenericVector<TVector>& V)
{
   return V.dim()==0 || is_zero(V.top().front());
}

template <typename TVector>
std::enable_if_t<check_container_feature<TVector, sparse>::value, bool>
is_leading_zero(const GenericVector<TVector>& V)
{
   return V.top().size()==0 || V.top().begin().index() > 0;
}

namespace operations {

template <typename OpRef,
          typename Discr=typename object_traits<typename deref<OpRef>::type>::generic_tag>
class normalize_impl;

template <typename OpRef>
class normalize_impl<OpRef, is_vector> {
public:
   typedef OpRef argument_type;
   typedef typename deref<OpRef>::type::element_type scalar_type;
   typedef decltype(std::declval<OpRef&&>() / std::declval<scalar_type>()) result_type;

   template <typename V>
   result_type operator() (V&& v) const
   {
      const scalar_type norm=sqrt(sqr(v));
      return std::forward<V>(v) / +(is_zero(norm) ? one_value<scalar_type>() : norm);
   }

   template <typename V>
   void assign(V&& v) const
   {
      const scalar_type norm=sqrt(sqr(v));
      if (!is_zero(norm))
         v /= norm;
   }
};

template <typename OpRef>
class normalize_vectors : public normalize_impl<OpRef> {};

template <typename OpRef,
          typename Discr=typename object_traits<typename deref<OpRef>::type>::generic_tag>
class dehomogenize_impl;

template <typename OpRef,
          typename Discr=typename object_traits<typename deref<OpRef>::type>::generic_tag>
class dehomogenize_trop_impl;

template <typename OpRef>
class dehomogenize_impl<OpRef, is_vector> {
protected:
   using slice = decltype(std::declval<OpRef&&>().slice(range_from(1)));
   using element_ref = decltype(std::declval<OpRef&&>().front());
   static const bool is_sparse=check_container_ref_feature<OpRef, sparse>::value;
   typedef LazyVector2<const slice, const same_value_container<element_ref>, polymake::operations::div>
      lazy_vector;
public:
   typedef OpRef argument_type;
   typedef ContainerUnion< mlist<slice, lazy_vector> > result_type;
protected:
   template <typename V>
   static result_type impl(V&& v, std::false_type)
   {
      auto&& first = v.front();
      if (is_zero(first) || is_one(first)) return std::forward<V>(v).slice(range_from(1));
      return lazy_vector(std::forward<V>(v).slice(range_from(1)), same_value_container<element_ref>(std::forward<decltype(first)>(first)));
   }

   template <typename V>
   static result_type impl(V&& v, std::true_type)
   {
      auto first=v.begin();
      if (first.at_end() || first.index() != 0 || is_one(*first)) return std::forward<V>(v).slice(range_from(1));
      return lazy_vector(std::forward<V>(v).slice(range_from(1)), same_value_container<element_ref>(*first));
   }

public:
   template <typename V>
   result_type operator() (V&& v) const
   {
      return impl(std::forward<V>(v), bool_constant<is_sparse>());
   }
};

template <typename OpRef>
class dehomogenize_trop_impl<OpRef, is_vector> {
protected:
   using slice = decltype(std::declval<OpRef&&>().slice(range_from(1)));
   using element_ref = decltype(std::declval<OpRef&&>().front());
   static const bool is_sparse=check_container_ref_feature<OpRef, sparse>::value;
   typedef LazyVector2<const slice, const SameElementVector<element_ref>, polymake::operations::sub>
      lazy_vector;
public:
   typedef OpRef argument_type;
   typedef ContainerUnion< mlist<slice, lazy_vector> > result_type;

   template <typename V>
   result_type operator() (V&& v) const
   {
      if (is_leading_zero(v)) return std::forward<V>(v).slice(range_from(1));
      return lazy_vector(std::forward<V>(v).slice(range_from(1)), SameElementVector<element_ref>(*v.begin(), v.dim()-1));
   }
};

template <typename OpRef>
class dehomogenize_vectors : public dehomogenize_impl<OpRef> {};

template <typename OpRef>
class dehomogenize_trop_vectors : public dehomogenize_trop_impl<OpRef> {};

template <typename OpRef>
struct get_numerator {
   typedef OpRef argument_type;
   typedef decltype(numerator(std::declval<std::add_const_t<std::remove_reference_t<OpRef>>>())) result_type;

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return numerator(x);
   }
};

template <typename OpRef>
struct get_denominator {
   typedef OpRef argument_type;
   typedef decltype(denominator(std::declval<std::add_const_t<std::remove_reference_t<OpRef>>>())) result_type;

   result_type operator() (typename function_argument<OpRef>::const_type x) const
   {
      return denominator(x);
   }
};

} } // end namespace pm::operations

namespace polymake { namespace operations {
   typedef BuildUnary<pm::operations::normalize_vectors> normalize_vectors;
   typedef BuildUnary<pm::operations::dehomogenize_vectors> dehomogenize_vectors;
   typedef BuildUnary<pm::operations::dehomogenize_trop_vectors> dehomogenize_trop_vectors;
   typedef BuildUnary<pm::operations::get_numerator> get_numerator;
   typedef BuildUnary<pm::operations::get_denominator> get_denominator;
} }

namespace pm {

/// Divide each vector in a sequence thru its length (L2-norm)
template <typename Iterator>
void normalize(Iterator dst)
{
   perform_assign(dst, polymake::operations::normalize_vectors());
}

template <typename TMatrix>
typename TMatrix::persistent_nonsymmetric_type
normalized(const GenericMatrix<TMatrix>& m)
{
   return typename TMatrix::persistent_type(m.rows(), m.cols(),
                                            entire(attach_operation(rows(m), polymake::operations::normalize_vectors())));
}

/// Compute the average over the rows of a matrix
template <typename TMatrix, typename E>
Vector<E> barycenter(const GenericMatrix<TMatrix, E>& V)
{
   return average(rows(V));
}

/// Compute the determinant of a matrix using the Gauss elimination method
template <typename TMatrix, typename E>
std::enable_if_t<is_field<E>::value, E>
det(const GenericMatrix<TMatrix, E>& m)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != m.cols())
         throw std::runtime_error("det - non-square matrix");
   }
   return det(typename TMatrix::persistent_nonsymmetric_type(m));
}

/// Compute the trace of a matrix
template <typename TMatrix, typename E>
E trace(const GenericMatrix<TMatrix, E>& m)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != m.cols())
         throw std::runtime_error("trace - non-square matrix");
   }
   return accumulate(m.diagonal(), BuildBinary<operations::add>());
}


template <typename TMatrix, typename E>
std::enable_if_t<!std::is_same<E, typename algebraic_traits<E>::field_type>::value, E>
det(const GenericMatrix<TMatrix, E>& m)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != m.cols())
         throw std::runtime_error("det - non-square matrix");
   }
   return convert_to<E>(det(typename GenericMatrix<TMatrix, typename algebraic_traits<E>::field_type>::persistent_nonsymmetric_type(m)));
}

/// Reduce a vector with a given matrix using the Gauss elimination method
template <typename TMatrix, typename TVector, typename E>
std::enable_if_t<is_field<E>::value, typename TVector::persistent_type>
reduce(const GenericMatrix<TMatrix, E>& A, const GenericVector<TVector, E>& V)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TVector>()) {
      if (V.dim() != A.cols())
         throw std::runtime_error("reduce - dimension mismatch");
   }
   using vector_type = std::conditional_t<TMatrix::is_sparse, SparseVector<E>, Vector<E>>;
   return reduce(typename TMatrix::persistent_nonsymmetric_type(A), vector_type(V));
}

template <typename TMatrix, typename TVector, typename E>
std::enable_if_t<!std::is_same<E, typename algebraic_traits<E>::field_type>::value,
                 Vector<typename algebraic_traits<E>::field_type>>
reduce(const GenericMatrix<TMatrix, E>& A, const GenericVector<TVector, E>& V)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TVector>()) {
      if (V.dim() != A.cols())
         throw std::runtime_error("reduce - dimension mismatch");
   }
   using Field = typename algebraic_traits<E>::field_type;
   using vector_type = std::conditional_t<TMatrix::is_sparse, SparseVector<Field>, Vector<Field>>;
   return reduce(typename GenericMatrix<TMatrix, Field>::persistent_nonsymmetric_type(A), vector_type(V));
}

/** Compute the inverse matrix $A^-1$ using the Gauss elimination method.
    @exception degenerate_matrix if det(A)==0
*/
template <typename TMatrix, typename E>
std::enable_if_t<is_field<E>::value,
                 typename TMatrix::persistent_nonsymmetric_type>
inv(const GenericMatrix<TMatrix, E>& m)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != m.cols())
         throw std::runtime_error("inv - non-square matrix");
   }
   return inv(typename TMatrix::persistent_nonsymmetric_type(m));
}

template <typename TMatrix, typename E>
std::enable_if_t<!std::is_same<E, typename algebraic_traits<E>::field_type>::value,
                 typename GenericMatrix<TMatrix, typename algebraic_traits<E>::field_type>::persistent_nonsymmetric_type>
inv(const GenericMatrix<TMatrix, E>& m)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (m.rows() != m.cols())
         throw std::runtime_error("inv - non-square matrix");
   }
   return inv(typename GenericMatrix<TMatrix, typename algebraic_traits<E>::field_type>::persistent_nonsymmetric_type(m));
}

/** Solve the linear system A*x==b
    @return xapps/polytope/src/reverse_search_graph.cco
    @exception degenerate_matrix if det(A) == 0
    @exception infeasible if rank(A) != rank(A|b)
*/
template <typename TMatrix, typename TVector, typename E>
std::enable_if_t<is_field<E>::value, Vector<E>>
lin_solve(const GenericMatrix<TMatrix, E>& A, const GenericVector<TVector, E>& b)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TVector>()) {
      if (b.dim() != A.rows())
         throw std::runtime_error("lin_solve - dimension mismatch");
   }
   return lin_solve(typename TMatrix::persistent_nonsymmetric_type(A), Vector<E>(b));
}

template <typename TMatrix, typename TVector, typename E>
std::enable_if_t<!std::is_same<E, typename algebraic_traits<E>::field_type>::value,
                 Vector<typename algebraic_traits<E>::field_type>>
lin_solve(const GenericMatrix<TMatrix, E>& A, const GenericVector<TVector, E>& b)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TVector>()) {
      if (b.dim() != A.rows())
         throw std::runtime_error("lin_solve - dimension mismatch");
   }
   typedef typename algebraic_traits<E>::field_type Field;
   return lin_solve(typename GenericMatrix<TMatrix, Field>::persistent_nonsymmetric_type(A), Vector<Field>(b));
}

template <typename TMatrix, typename TVector, typename E>
std::enable_if_t<is_field<E>::value, Vector<E>>
cramer(const GenericMatrix<TMatrix, E>& A, const GenericVector<TVector, E>& b) {
   const Int d = A.rows();
   if (POLYMAKE_DEBUG || is_wary<TMatrix>() || is_wary<TVector>()) {
      if (A.cols() != d)
         throw std::runtime_error("cramer - non square matrix");
      if (b.dim() != d)
         throw std::runtime_error("cramer - dimension mismatch");
   }
   const E det_A = det(A);
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (det_A == 0)
         throw std::runtime_error("cramer - matrix singular");
   }
   Vector<E> x(d);
   for (Int i = 0; i < d; ++i) {
      x[i] = det(A.minor(All,sequence(0,i)) | b | A.minor(All,sequence(i+1,d-i-1))) / det_A;
   }
   return x;
}

/**
   if the row referenced by h is not orthogonal to v,
   project out *h from all subsequent rows so that they become orthogonal to v
   @param h row iterator over a matrix
   @param v the vector that the rows should become orthogonal to
   @param row_basis_consumer output iterator consuming the indices of basis rows (=vectors)
   @param col_basis_consumer output iterator consuming the indices of basis columns (=coordinates)
   @return true if the matrix has been modified, false otherwise
*/
template<typename AHRowIterator, typename VectorType, typename RowBasisOutputIterator, typename ColBasisOutputIterator>
bool project_rest_along_row (AHRowIterator &h, const VectorType& v, RowBasisOutputIterator row_basis_consumer, ColBasisOutputIterator col_basis_consumer, Int i = 0)
{
   const typename iterator_traits<AHRowIterator>::value_type::element_type pivot = (*h) * v;
   if (is_zero(pivot)) // *h is already orthogonal to v, nothing to do
      return false;

   // else *h is not orthogonal to v.
   // Project each row *h2 that comes after *h along *h until *h2 becomes orthogonal to v.

   // first, bookkeeping if desired
   if (!is_derived_from_instance_of<RowBasisOutputIterator, black_hole>::value)
      *row_basis_consumer++ = i;
   if (!is_derived_from_instance_of<ColBasisOutputIterator, black_hole>::value)
      *col_basis_consumer++ = h->rbegin().index();

   AHRowIterator h2 = h;
   while (!(++h2).at_end()) { // don't project h itself, only later rows
      const typename iterator_traits<AHRowIterator>::value_type::element_type x = (*h2) * v;
      if (!is_zero(x))
         reduce_row(h2, h, pivot, x); // project h2 along h until orthogonal to v
   }
   return true;
}

/** Compute a basis of (subspace spanned by the rows of H) intersect (orthogonal complement of v)
    Projects all row vectors in H onto the orthogonal complement of v, and keeps a basis of rowspan(H) intersect orthogonal(v)
    @param v input iterator over the vectors
    @param row_basis_consumer output iterator consuming the indices of basis rows (=vectors)
    @param col_basis_consumer output iterator consuming the indices of basis columns (=coordinates)
    @return modified_H boolean value indicating whether the kernel matrix H has been modified
*/
template <typename VectorType, typename RowBasisOutputIterator, typename ColBasisOutputIterator, typename E>
bool
basis_of_rowspan_intersect_orthogonal_complement(ListMatrix< SparseVector<E> >& H, const VectorType& v, RowBasisOutputIterator row_basis_consumer, ColBasisOutputIterator col_basis_consumer, Int i = 0)
{
   for (auto h=entire(rows(H)); !h.at_end(); ++h) {
      if (project_rest_along_row(h, v, row_basis_consumer, col_basis_consumer, i)) {
         H.delete_row(h);
         return true;
      }
   }
   return false;
}


/** add a row to a matrix M iff this increases the rowspan of M. As a side effect, update the kernel of M.
    @param M a matrix, implemented as a ListMatrix so that the row addition is cheap
    @param kernel_so_far a matrix whose rows are supposed to be orthogonal to the rows of M.
    @param v a vector to be added to M iff this increases the dimension of the rowspan of M. We allow the entries of v and kernel_so_far to be of different type, e.g. T=Integer, R=Rational
*/
template <typename T, typename R>
bool add_row_if_rowspace_increases(ListMatrix<SparseVector<T> >& M, const SparseVector<T>& v, ListMatrix<SparseVector<R> >& kernel_so_far)
{
   const bool modified = basis_of_rowspan_intersect_orthogonal_complement(kernel_so_far, v, black_hole<Int>(), black_hole<Int>());
    if (modified) M.insert_row(rows(M).begin(), v);
    return modified;
}

template <typename Iterator, typename E>
std::enable_if_t<is_field<E>::value>
reduce_row(Iterator& h2, Iterator& h, const E& pivot, const E& x)
{
   *h2 -= (x/pivot)*(*h);
}

template <typename Iterator, typename E>
std::enable_if_t<!is_field<E>::value>
reduce_row(Iterator& h2, Iterator& h, const E& pivot, const E& x)
{
   *h2 *= pivot;  *h2 -= x*(*h);
}

template <typename TMatrix>
std::enable_if_t<is_gcd_domain<typename TMatrix::element_type>::value>
simplify_rows(GenericMatrix<TMatrix>& M)
{
   for (auto r=entire(rows(M)); !r.at_end(); ++r) {
      const auto g=gcd(*r);
      if (!is_one(g)) r->div_exact(g);
   }
}

template <typename TMatrix>
std::enable_if_t<!is_gcd_domain<typename TMatrix::element_type>::value>
simplify_rows(GenericMatrix<TMatrix>& M)
{}


/** Compute the basis of the subspaces spanned by a sequence of vectors and orthogonal one.
    Projects all row vectors in H into the orthogonal complement of the vectors that v iterates over,
    and keeps a basis of rowspan(H) intersect orthogonal(v_i)
    @param v input iterator over the vectors
    @param row_basis_consumer output iterator consuming the indices of basis rows (=vectors)
    @param col_basis_consumer output iterator consuming the indices of basis columns (=coordinates)
*/
template <typename VectorIterator, typename RowBasisOutputIterator, typename ColBasisOutputIterator, typename AH_matrix>
void
null_space(VectorIterator v, RowBasisOutputIterator row_basis_consumer, ColBasisOutputIterator col_basis_consumer, AH_matrix& H, bool simplify = false)
{
   for (Int i = 0; H.rows() > 0 && !v.at_end(); ++v, ++i)
      basis_of_rowspan_intersect_orthogonal_complement(H, *v, row_basis_consumer, col_basis_consumer, i);
   if (simplify) simplify_rows(H);
}


template <typename TMatrix, typename E>
typename TMatrix::persistent_nonsymmetric_type
null_space(const GenericMatrix<TMatrix, E>& M)
{
   ListMatrix< SparseVector<E> > H=unit_matrix<E>(M.cols());
   null_space(entire(rows(M)), black_hole<Int>(), black_hole<Int>(), H, true);
   return H;
}

template <typename TVector, typename E>
ListMatrix< SparseVector<E> >
null_space(const GenericVector<TVector, E>& V)
{
   ListMatrix< SparseVector<E> > H=unit_matrix<E>(V.dim());
   null_space(entire(single_value_as_container(V.top())), black_hole<Int>(), black_hole<Int>(), H, true);
   return H;
}

/// @param req_sign expected sign of det( null_space(V) / V )
template <typename TVector, typename E>
ListMatrix< SparseVector<E> >
null_space_oriented(const GenericVector<TVector, E>& V, Int req_sign)
{
   ListMatrix< SparseVector<E> > H=unit_matrix<E>(V.dim());
   null_space(entire(single_value_as_container(V.top())), black_hole<Int>(), black_hole<Int>(), H, true);
   auto v_pivot=ensure(V.top(), pure_sparse()).begin();
   if (v_pivot.at_end() && req_sign != 0)
      throw infeasible("null_space_oriented: zero vector has no orientation");
   if ((sign(*v_pivot) == req_sign) == (v_pivot.index() + V.dim()+1)%2)
      rows(H).back().negate();
   return H;
}

template <typename TMatrix, typename E>
typename TMatrix::persistent_nonsymmetric_type
lineality_space(const GenericMatrix<TMatrix, E>& M)
{
   ListMatrix< SparseVector<E> > H=unit_matrix<E>(M.cols()-1);
   null_space(entire(rows(M.minor(All, range(1,M.cols()-1)))), black_hole<Int>(), black_hole<Int>(), H, true);
   return zero_vector<E>(H.rows()) | H;
}

template <typename VectorIterator>
Set<Int>
basis_vectors(VectorIterator v)
{
   typedef typename iterator_traits<VectorIterator>::value_type::element_type E;
   ListMatrix< SparseVector<E> > H=unit_matrix<E>(v.at_end() ? 0 : v->dim());
   Set<Int> b;
   null_space(v, std::back_inserter(b), black_hole<Int>(), H);
   return b;
}

template <typename TMatrix, typename E>
Set<Int>
basis_rows(const GenericMatrix<TMatrix, E>& M)
{
   ListMatrix< SparseVector<E> > H=unit_matrix<E>(M.cols());
   Set<Int> b;
   null_space(entire(rows(M)), std::back_inserter(b), black_hole<Int>(), H);
   return b;
}

template <typename TMatrix>
Set<Int>
basis_cols(const GenericMatrix<TMatrix>& M)
{
   return basis_rows(T(M));
}

template <typename TMatrix, typename E>
std::pair<Set<Int>, Set<Int>>
basis(const GenericMatrix<TMatrix, E>& M)
{
   ListMatrix< SparseVector<E> > H=unit_matrix<E>(M.cols());
   Set<Int> br, bc;
   null_space(entire(rows(M)), std::back_inserter(br), inserter(bc), H);
   return std::make_pair(br, bc);
}

template <typename TMatrix, typename E>
Int rank(const GenericMatrix<TMatrix, E>& M)
{
   if (M.rows() <= M.cols()) {
      ListMatrix< SparseVector<E> > H=unit_matrix<E>(M.rows());
      null_space(entire(cols(M)), black_hole<Int>(), black_hole<Int>(), H);
      return M.rows() - H.rows();
   }
   ListMatrix< SparseVector<E> > H=unit_matrix<E>(M.cols());
   null_space(entire(rows(M)), black_hole<Int>(), black_hole<Int>(), H);
   return M.cols()-H.rows();
}

template <typename TMatrix>
Set<Int>
basis_rows(const GenericMatrix<TMatrix, double>& M)
{
   ListMatrix< SparseVector<double> > H=unit_matrix<double>(M.cols());
   Set<Int> b;
   null_space(entire(attach_operation(rows(M), polymake::operations::normalize_vectors())),
              std::back_inserter(b), black_hole<Int>(), H);
   return b;
}

template <typename TMatrix>
std::pair<Set<Int>, Set<Int>>
basis(const GenericMatrix<TMatrix, double>& M)
{
   ListMatrix< SparseVector<double> > H=unit_matrix<double>(M.cols());
   Set<Int> br, bc;
   null_space(entire(attach_operation(rows(M), polymake::operations::normalize_vectors())),
              std::back_inserter(br), inserter(bc), H);
   return std::make_pair(br,bc);
}

template <typename TMatrix>
Int rank(const GenericMatrix<TMatrix, double>& M)
{
   if (M.rows() <= M.cols()) {
      ListMatrix< SparseVector<double> > H=unit_matrix<double>(M.rows());
      null_space(entire(attach_operation(cols(M), polymake::operations::normalize_vectors())),
                 black_hole<Int>(), black_hole<Int>(), H);
      return M.rows()-H.rows();
   }
   ListMatrix< SparseVector<double> > H=unit_matrix<double>(M.cols());
   null_space(entire(attach_operation(rows(M), polymake::operations::normalize_vectors())),
              black_hole<Int>(), black_hole<Int>(), H);
   return M.cols() - H.rows();
}

/// The same as basis(), but ignoring the first column of the matrix.
template <typename TMatrix, typename E>
std::pair< Set<Int>, Set<Int>>
basis_affine(const GenericMatrix<TMatrix, E>& M)
{
   Int ad = M.cols()-1;
   ListMatrix<SparseVector<E>> H = unit_matrix<E>(ad);
   Set<Int> br, bc;
   null_space(entire(rows(M.minor(All, range(1, ad)))), std::back_inserter(br),
              make_output_transform_iterator(inserter(bc), operations::fix2<Int, operations::add<Int, Int> >(1)),
              H);
   return std::make_pair(br,bc);
}

/// Project u to v.
template <typename E, typename Vector1, typename Vector2>
typename Vector2::persistent_type
proj(const GenericVector<Vector1,E>& u, const GenericVector<Vector2,E>& v)
{
   return (u*v)/sqr(v)*v;
}

/// project the rows of M into the orthogonal complement of N
/// the rows of N need to be orthogonal
template <typename Matrix1, typename Matrix2>
void
project_to_orthogonal_complement(Matrix1& M, const Matrix2& N)
{
    for (auto nit = entire(rows(N)); !nit.at_end(); ++nit) {
       const typename Matrix2::element_type normsquared = sqr(*nit);
       if (POLYMAKE_DEBUG || is_wary<Matrix2>()) {
         for (auto nit2 = nit+1; !nit2.at_end(); ++nit2)
           if (!is_zero((*nit) * (*nit2)))
              throw std::runtime_error("project_to_orthogonal_complement: error: non-orthogonal matrix");
       }
       if (!is_zero(normsquared))
          for (auto mit = entire(rows(M)); !mit.at_end(); ++mit) {
             const typename Matrix1::element_type pivot = (*mit) * (*nit);
             if (!is_zero(pivot))
                *mit -= pivot/normsquared * (*nit);
          }
    }
}

/// the indices of nonzero entries
template <typename TVector>
Set<Int> support(const GenericVector<TVector>& v)
{
   return indices(ensure(v.top(), pure_sparse()));
}

/// reflect u in the plane normal to nv
template <typename Vector1, typename Vector2>
typename Vector1::persistent_type
reflect(const GenericVector<Vector1>& u, const GenericVector<Vector2>& nv)
{
   if (!is_leading_zero(nv))
      throw std::runtime_error("cannot reflect in a vector at infinity (first coordinate zero)");
   return u - (2 * (u.slice(range_from(1))*nv.slice(range_from(1))) / sqr(nv.slice(range_from(1)))) * nv;
}

/// Divide by the first element and strip it off.
/// As a special case, an empty vector is passed unchanged.
template <typename TVector>
typename TVector::persistent_type
dehomogenize(const GenericVector<TVector>& V)
{
   if (V.dim()==0)
      return typename TVector::persistent_type();

   return operations::dehomogenize_vectors<const TVector&>()(V.top());
}

/// Divide rowwise by the elements of the first column and strip it off.
/// As a special case, an empty matrix is passed unchanged.
template <typename TMatrix>
typename TMatrix::persistent_nonsymmetric_type
dehomogenize(const GenericMatrix<TMatrix>& M)
{
   if (M.cols()==0)
      return typename TMatrix::persistent_nonsymmetric_type();

   return typename TMatrix::persistent_nonsymmetric_type(M.rows(), M.cols()-1,
                                                         entire(attach_operation(rows(M), polymake::operations::dehomogenize_vectors())));
}

/// Subtract the first element and strip it off.
/// As a special case, an empty vector is passed unchanged.
template <typename TVector>
typename TVector::persistent_type
dehomogenize_trop(const GenericVector<TVector>& V)
{
   if (V.dim()==0)
      return typename TVector::persistent_type();

   return operations::dehomogenize_trop_vectors<const TVector&>()(V.top());
}

/// Subtract rowwise the elements of the first column and strip it off.
/// As a special case, an empty matrix is passed unchanged.
template <typename TMatrix>
typename TMatrix::persistent_nonsymmetric_type
dehomogenize_trop(const GenericMatrix<TMatrix>& M)
{
   if (M.cols()==0)
      return typename TMatrix::persistent_nonsymmetric_type();

   return typename TMatrix::persistent_nonsymmetric_type(M.rows(), M.cols()-1,
                                                         entire(attach_operation(rows(M), polymake::operations::dehomogenize_trop_vectors())));
}

/// Remove all matrix rows that contain only zeros.
template <typename TMatrix>
typename TMatrix::persistent_nonsymmetric_type
remove_zero_rows(const GenericMatrix<TMatrix>& m)
{
   auto subset = attach_selector(rows(m),polymake::operations::non_zero());
   return typename TMatrix::persistent_nonsymmetric_type(subset.size(), m.cols(), entire(subset));
}

template <typename VectorIterator, typename OutputIterator>
void orthogonalize(VectorIterator v, OutputIterator sqr_consumer)
{
   // the first vector will not be modified
   while (!v.at_end()) {
      const auto s=sqr(*v);
      if (!is_zero(s)) {
         VectorIterator v2=v;
         for (++v2; !v2.at_end(); ++v2) {
            const auto x=(*v2) * (*v);
            if (!is_zero(x)) reduce_row(v2, v, s, x);
         }
      }
      *sqr_consumer++ = s;
      ++v;
   }
}

template <typename VectorIterator, typename OutputIterator>
void orthogonalize_affine(VectorIterator v, OutputIterator sqr_consumer)
{
   // the first vector will not be modified
   while (!v.at_end()) {
      const auto s=sqr(v->slice(range_from(1)));
      if (!is_zero(s)) {
         VectorIterator v2=v;
         for (++v2; !v2.at_end(); ++v2) {
            const auto x = v2->slice(range_from(1)) * v->slice(range_from(1));
            if (!is_zero(x)) reduce_row(v2, v, s, x);
         }
      }
      *sqr_consumer++ = s;
      ++v;
   }
}

/// Apply the Gram-Schmidt orthogonalization to the vector sequence.
template <typename VectorIterator>
void orthogonalize(VectorIterator v)
{
   orthogonalize(v, black_hole<typename iterator_traits<VectorIterator>::value_type::element_type>());
}

/** The same as orthogonalize(.), but making the affine parts of the resulting vectors
    (without 0-th coordinate) orthogonal
*/
template <typename VectorIterator>
void orthogonalize_affine(VectorIterator v)
{
   orthogonalize_affine(v, black_hole<typename iterator_traits<VectorIterator>::value_type::element_type>());
}

/// Find row indices of all far points (that is, having zero in the first column). For dense matrices.
template <typename TMatrix>
std::enable_if_t<!TMatrix::is_sparse, Set<Int>>
far_points(const GenericMatrix<TMatrix>& M)
{
   if (M.cols() == 0) return Set<Int>();
   return indices(attach_selector(M.col(0), polymake::operations::is_zero()));
}

/// Find row indices of all far points (that is, having zero in the first column). For sparse matrices.
template <typename TMatrix>
std::enable_if_t<TMatrix::is_sparse, Set<Int>>
far_points(const GenericMatrix<TMatrix>& M)
{
   if (M.cols() == 0) return Set<Int>();
   return sequence(0, M.rows()) - indices(M.col(0));
}

/// Find indices of rows orthogonal to the given vector
template <typename E, typename TMatrix, typename TVector>
Set<Int>
orthogonal_rows(const GenericMatrix<TMatrix, E>& M, const GenericVector<TVector, E>& v)
{
   return indices(attach_selector(attach_operation(rows(M), same_value(v), polymake::operations::mul()),
                                  polymake::operations::is_zero()));
}

template <typename Iterator>
typename iterator_traits<Iterator>::value_type
gcd_of_sequence(Iterator it)
{
   typedef typename iterator_traits<Iterator>::value_type T;
   if (it.at_end()) return zero_value<T>();
   T res=abs(*it);
   while (!is_one(res) && !(++it).at_end())
      res=gcd(res, *it);
   return res;
}


template <typename Iterator>
typename iterator_traits<Iterator>::value_type
lcm_of_sequence(Iterator it)
{
   typedef typename iterator_traits<Iterator>::value_type T;
   if (it.at_end()) return zero_value<T>();
   T res=abs(*it);
   while (!(++it).at_end())
      if (!is_one(*it)) res=lcm(res, *it);
   return res;
}

template <typename TVector, typename E>
E gcd(const GenericVector<TVector, E>& v)
{
   return gcd_of_sequence(entire(v.top()));
}

template <typename TVector, typename E>
E lcm(const GenericVector<TVector, E>& v)
{
   return lcm_of_sequence(entire(v.top()));
}

template <typename TVector, typename E>
auto numerators(const GenericVector<TVector, E>& v,
                std::enable_if_t<is_field_of_fractions<E>::value, std::nullptr_t> = nullptr)
{
   return apply_operation(v, polymake::operations::get_numerator());
}

template <typename TVector, typename E>
auto denominators(const GenericVector<TVector, E>& v,
                  std::enable_if_t<is_field_of_fractions<E>::value, std::nullptr_t> = nullptr)
{
   return apply_operation(v, polymake::operations::get_denominator());
}

template <typename TMatrix, typename E>
auto numerators(const GenericMatrix<TMatrix, E>& m,
                std::enable_if_t<is_field_of_fractions<E>::value, std::nullptr_t> = nullptr)
{
   return apply_operation(m, polymake::operations::get_numerator());
}

template <typename TMatrix, typename E>
auto denominators(const GenericMatrix<TMatrix, E>& m,
                  std::enable_if_t<is_field_of_fractions<E>::value, std::nullptr_t> = nullptr)
{
   return apply_operation(m, polymake::operations::get_denominator());
}

template <typename TMatrix>
typename TMatrix::persistent_nonsymmetric_type
pow(const GenericMatrix<TMatrix>& base, long exp)
{
   if (POLYMAKE_DEBUG || is_wary<TMatrix>()) {
      if (base.rows() != base.cols())
         throw std::runtime_error("pow - non-square matrix");
   }
   auto one = unit_matrix<typename TMatrix::element_type>(base.rows());
   if (exp < 0) {
      return pow_impl<typename TMatrix::persistent_nonsymmetric_type>(inv(base), one, -exp);
   } else if (exp == 0) {
      return one;
   }
   return pow_impl<typename TMatrix::persistent_nonsymmetric_type>(base, one, exp);
}

} // end namespace pm

namespace polymake {
   using pm::null_space;
   using pm::lineality_space;
   using pm::basis_vectors;
   using pm::orthogonalize;
   using pm::orthogonalize_affine;
   using pm::normalize;
   using pm::gcd_of_sequence;
   using pm::lcm_of_sequence;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
