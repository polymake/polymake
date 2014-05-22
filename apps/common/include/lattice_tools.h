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

#ifndef POLYMAKE_POLYTOPE_LATTICE_TOOLS_H
#define POLYMAKE_POLYTOPE_LATTICE_TOOLS_H

#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/Smith_normal_form.h"

namespace polymake { namespace common {

template <typename VectorTop> inline
bool is_integral(const GenericVector<VectorTop, Rational>& V)
{
   for (typename Entire<VectorTop>::const_iterator x=entire(V.top()); !x.at_end(); ++x)
      if (denominator(*x) != 1) return false;
   return true;
}

template <typename MatrixTop> inline
bool is_integral(const GenericMatrix<MatrixTop, Rational>& M,
                 typename pm::enable_if<void**, MatrixTop::is_flat>::type=0)
{
   return is_integral(concat_rows(M));
}

template <typename MatrixTop> inline
bool is_integral(const GenericMatrix<MatrixTop, Rational>& M,
                 typename pm::disable_if<void**, MatrixTop::is_flat>::type=0)
{
   for (typename Entire< Rows<MatrixTop> >::const_iterator rit=entire(rows(M)); !rit.at_end(); ++rit)
      if (!is_integral(*rit)) return false;
   return true;
}

namespace {

template <typename DstVector, typename SrcIterator> inline
void store_eliminated_denominators(DstVector& vec, SrcIterator src, const Integer& LCM, pm::False)
{
   typename DstVector::iterator dst=vec.begin();
   for (; !src.at_end(); ++src, ++dst)
      if (!is_zero(*src))
         *dst = div_exact(LCM, denominator(*src)) * numerator(*src);
}

template <typename DstVector, typename SrcIterator> inline
void store_eliminated_denominators(DstVector& vec, SrcIterator src, const Integer& LCM, pm::True)
{
   for (; !src.at_end(); ++src)
      vec.push_back(src.index(), div_exact(LCM, denominator(*src)) * numerator(*src));
}

template <typename DstVector, typename SrcVector> inline
void copy_eliminated_denominators(DstVector& vec, const SrcVector& src)
{
   store_eliminated_denominators(vec, entire(src), lcm(denominators(src)), bool2type<pm::check_container_feature<DstVector, pm::sparse>::value>());
}

}

template <typename VectorTop> inline
typename GenericVector<VectorTop, Integer>::persistent_type
eliminate_denominators(const GenericVector<VectorTop, Rational>& V)
{
   typename GenericVector<VectorTop, Integer>::persistent_type result(V.dim());
   copy_eliminated_denominators(result, V.top());
   return result;
}

template <typename MatrixTop> inline
typename GenericMatrix<MatrixTop, Integer>::persistent_type
eliminate_denominators_in_rows(const GenericMatrix<MatrixTop, Rational>& M)
{
   typename GenericMatrix<MatrixTop, Integer>::persistent_type result(M.rows(), M.cols());
   if (M.rows() && M.cols()) {
      typename Rows<typename GenericMatrix<MatrixTop, Integer>::persistent_type>::iterator d=rows(result).begin();
      for (typename Entire< Rows<MatrixTop> >::const_iterator s=entire(rows(M)); !s.at_end(); ++s, ++d)
         copy_eliminated_denominators(d->top(), *s);
   }
   return result;
}

template <typename MatrixTop> inline
typename GenericMatrix<MatrixTop, Integer>::persistent_type
eliminate_denominators_entire(const GenericMatrix<MatrixTop, Rational>& M)
{
   typename GenericMatrix<MatrixTop, Integer>::persistent_type result(M.rows(), M.cols());
   if (M.rows() && M.cols()) {
      copy_eliminated_denominators(concat_rows(result), concat_rows(M));
   }
   return result;
}

template <typename MatrixTop> inline
typename GenericMatrix<MatrixTop, Integer>::persistent_type
eliminate_denominators_entire_affine(const GenericMatrix<MatrixTop, Rational>& M)
{
   typename GenericMatrix<MatrixTop, Integer>::persistent_type result(M.rows(), M.cols());
   if (M.rows() && M.cols()) {
      if (M.cols() > 1)
         copy_eliminated_denominators(concat_rows(result.minor(All,~scalar2set(0)).top()), concat_rows(M.minor(All,~scalar2set(0))));
      result.col(0) = M.col(0);
   }
   return result;
}

template <typename VectorTop> inline
typename pm::enable_if<typename VectorTop::persistent_type,
                       pm::is_gcd_domain<typename VectorTop::element_type>::value>::type
divide_by_gcd(const GenericVector<VectorTop>& v)
{
   return div_exact(v, gcd(v));
}
      
template <typename MatrixTop> inline
typename pm::enable_if<typename MatrixTop::persistent_type,
                       pm::is_gcd_domain<typename MatrixTop::element_type>::value>::type
divide_by_gcd(const GenericMatrix<MatrixTop> & M)
{
   typename MatrixTop::persistent_type result(M.rows(), M.cols());
   if (M.cols() && M.rows()) {
      typename Rows<typename MatrixTop::persistent_type>::iterator dst = rows(result).begin();
      for (typename Entire< Rows<MatrixTop> >::const_iterator src = entire(rows(M)); !src.at_end(); ++src, ++dst)
         *dst = div_exact(*src, gcd(*src));
   }
   return result;
}


template <typename MatrixTop> inline
typename pm::enable_if<typename MatrixTop::persistent_type,
                       pm::is_gcd_domain<typename MatrixTop::element_type>::value>::type
primitive(const GenericMatrix<MatrixTop>& M)
{
   return divide_by_gcd(M);
}

template <typename MatrixTop> inline
typename pm::enable_if<typename MatrixTop::persistent_type,
                       pm::is_gcd_domain<typename MatrixTop::element_type>::value>::type
primitive_affine(const GenericMatrix<MatrixTop>& M)
{
   return M.col(0) | divide_by_gcd(M.minor(All, ~scalar2set(0)));
}

template <typename MatrixTop> inline
typename GenericMatrix<MatrixTop, Integer>::persistent_type
primitive(const GenericMatrix<MatrixTop, Rational>& M)
{
   typedef typename GenericMatrix<MatrixTop, Integer>::persistent_type Matrix;
   Matrix result = eliminate_denominators_in_rows(M);
   for (typename Entire< Rows<Matrix> >::iterator rit=entire(rows(result)); !rit.at_end(); ++rit)
      rit->div_exact(gcd(*rit));
   return result;
}

template <typename MatrixTop> inline
typename GenericMatrix<MatrixTop, Integer>::persistent_type
primitive_affine(const GenericMatrix<MatrixTop, Rational>& M)
{
   if (!is_integral(M.col(0)))
      throw std::runtime_error("homogeneous coordinates not integral");

   return numerators(M.col(0)) | primitive(M.minor(All, ~scalar2set(0)));
}

 
template <typename VectorTop> inline
typename pm::enable_if<typename VectorTop::persistent_type,
                       pm::is_gcd_domain<typename VectorTop::element_type>::value>::type
primitive(const GenericVector<VectorTop>& v)
{
   return divide_by_gcd(v);
}

template <typename VectorTop> inline
typename pm::enable_if<typename VectorTop::persistent_type,
                       pm::is_gcd_domain<typename VectorTop::element_type>::value>::type
primitive_affine(const GenericVector<VectorTop>& v)
{
   return v.top()[0] | divide_by_gcd(v.slice(1));
}

template <typename VectorTop> inline
typename GenericVector<VectorTop, Integer>::persistent_type
primitive(const GenericVector<VectorTop, Rational>& v)
{
   typedef typename GenericVector<VectorTop, Integer>::persistent_type Vector;
   Vector result = eliminate_denominators(v);
   result.div_exact(gcd(result));
   return result;
}

template <typename VectorTop> inline
typename GenericVector<VectorTop, Integer>::persistent_type
primitive_affine(const GenericVector<VectorTop, Rational>& v)
{
   if (denominator(v.top()[0]) != 1)
      throw std::runtime_error("homogeneous coordinate not integral");

   return numerator(v.top()[0]) | primitive(v.slice(1));
}

template <typename MatrixTop> inline
typename GenericMatrix<MatrixTop, Integer>::persistent_type
lattice_basis(const GenericMatrix<MatrixTop, Integer>& gens)
{
   const SmithNormalForm<Integer> SNF = smith_normal_form(gens.minor(All, ~scalar2set(0)));
   return (SNF.form * SNF.right_companion).minor(range(0, SNF.rank-1),All);
}

} }

#endif // POLYMAKE_POLYTOPE_LATTICE_TOOLS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
