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
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/Smith_normal_form.h"

namespace polymake { namespace common {

template <typename TVector>
bool is_integral(const GenericVector<TVector, Rational>& V)
{
   for (auto x=entire(V.top()); !x.at_end(); ++x)
      if (denominator(*x) != 1) return false;
   return true;
}

template <typename TMatrix>
typename std::enable_if<TMatrix::is_flat, bool>::type
is_integral(const GenericMatrix<TMatrix, Rational>& M)
{
   return is_integral(concat_rows(M));
}

template <typename TMatrix>
typename std::enable_if<!TMatrix::is_flat, bool>::type
is_integral(const GenericMatrix<TMatrix, Rational>& M)
{
   for (auto r=entire(rows(M)); !r.at_end(); ++r)
      if (!is_integral(*r)) return false;
   return true;
}

namespace {

template <typename DstVector, typename SrcIterator>
void store_eliminated_denominators(DstVector& vec, SrcIterator src, const Integer& LCM, std::false_type)
{
   for (auto dst=vec.begin(); !src.at_end(); ++src, ++dst)
      if (!is_zero(*src))
         *dst = div_exact(LCM, denominator(*src)) * numerator(*src);
}

template <typename DstVector, typename SrcIterator>
void store_eliminated_denominators(DstVector& vec, SrcIterator src, const Integer& LCM, std::true_type)
{
   for (; !src.at_end(); ++src)
      vec.push_back(src.index(), div_exact(LCM, denominator(*src)) * numerator(*src));
}

template <typename DstVector, typename SrcVector>
void copy_eliminated_denominators(DstVector&& vec, const SrcVector& src)
{
   store_eliminated_denominators(vec, entire(src), lcm(denominators(src)),
                                 bool_constant<pm::check_container_feature<pure_type_t<DstVector>, pm::sparse>::value>());
}

}

template <typename TVector>
typename GenericVector<TVector, Integer>::persistent_type
eliminate_denominators(const GenericVector<TVector, Rational>& V)
{
   typename GenericVector<TVector, Integer>::persistent_type result(V.dim());
   copy_eliminated_denominators(result, V.top());
   return result;
}

template <typename TMatrix>
typename GenericMatrix<TMatrix, Integer>::persistent_type
eliminate_denominators_in_rows(const GenericMatrix<TMatrix, Rational>& M)
{
   typename GenericMatrix<TMatrix, Integer>::persistent_type result(M.rows(), M.cols());
   auto d=rows(result).begin();
   for (auto s=entire(rows(M)); !s.at_end(); ++s, ++d)
      copy_eliminated_denominators(*d, *s);
   return result;
}

template <typename TMatrix>
typename GenericMatrix<TMatrix, Integer>::persistent_type
eliminate_denominators_entire(const GenericMatrix<TMatrix, Rational>& M)
{
   typename GenericMatrix<TMatrix, Integer>::persistent_type result(M.rows(), M.cols());
   copy_eliminated_denominators(concat_rows(result), concat_rows(M));
   return result;
}

template <typename TMatrix>
typename GenericMatrix<TMatrix, Integer>::persistent_type
eliminate_denominators_entire_affine(const GenericMatrix<TMatrix, Rational>& M)
{
   typename GenericMatrix<TMatrix, Integer>::persistent_type result(M.rows(), M.cols());
   if (M.rows() && M.cols()) {
      if (M.cols() > 1)
         copy_eliminated_denominators(concat_rows(result.minor(All, range_from(1))), concat_rows(M.minor(All, range_from(1))));
      result.col(0) = numerators(M.col(0));
   }
   return result;
}

template <typename TVector>
typename std::enable_if<is_gcd_domain<typename TVector::element_type>::value, typename TVector::persistent_type>::type
divide_by_gcd(const GenericVector<TVector>& v)
{
   return div_exact(v, gcd(v));
}
      
template <typename TMatrix>
typename std::enable_if<is_gcd_domain<typename TMatrix::element_type>::value, typename TMatrix::persistent_type>::type
divide_by_gcd(const GenericMatrix<TMatrix>& M)
{
   // TODO: reformulate as a constructor taking a transformed iterator over rows using lambda expression.
   typename TMatrix::persistent_type result(M.rows(), M.cols());
   auto dst = rows(result).begin();
   for (auto src = entire(rows(M)); !src.at_end(); ++src, ++dst)
      *dst = div_exact(*src, gcd(*src));
   return result;
}


template <typename TMatrix>
typename std::enable_if<is_gcd_domain<typename TMatrix::element_type>::value, typename TMatrix::persistent_type>::type
primitive(const GenericMatrix<TMatrix>& M)
{
   return divide_by_gcd(M);
}

template <typename TMatrix>
typename std::enable_if<is_gcd_domain<typename TMatrix::element_type>::value, typename TMatrix::persistent_type>::type
primitive_affine(const GenericMatrix<TMatrix>& M)
{
   return M.col(0) | divide_by_gcd(M.minor(All, range_from(1)));
}

template <typename TMatrix>
typename GenericMatrix<TMatrix, Integer>::persistent_type
primitive(const GenericMatrix<TMatrix, Rational>& M)
{
   typename GenericMatrix<TMatrix, Integer>::persistent_type result = eliminate_denominators_in_rows(M);
   for (auto r=entire(rows(result)); !r.at_end(); ++r)
      r->div_exact(gcd(*r));
   return result;
}

template <typename TMatrix>
typename GenericMatrix<TMatrix, Integer>::persistent_type
primitive_affine(const GenericMatrix<TMatrix, Rational>& M)
{
   if (!is_integral(M.col(0)))
      throw std::runtime_error("homogeneous coordinates not integral");

   return numerators(M.col(0)) | primitive(M.minor(All, range_from(1)));
}

 
template <typename TVector>
typename std::enable_if<is_gcd_domain<typename TVector::element_type>::value, typename TVector::persistent_type>::type
primitive(const GenericVector<TVector>& v)
{
   return divide_by_gcd(v);
}

template <typename TVector>
typename std::enable_if<is_gcd_domain<typename TVector::element_type>::value, typename TVector::persistent_type>::type
primitive_affine(const GenericVector<TVector>& v)
{
   return v.top()[0] | divide_by_gcd(v.slice(range_from(1)));
}

template <typename TVector>
typename GenericVector<TVector, Integer>::persistent_type
primitive(const GenericVector<TVector, Rational>& v)
{
   typename GenericVector<TVector, Integer>::persistent_type result = eliminate_denominators(v);
   result.div_exact(gcd(result));
   return result;
}

template <typename TVector>
typename GenericVector<TVector, Integer>::persistent_type
primitive_affine(const GenericVector<TVector, Rational>& v)
{
   if (denominator(v.top()[0]) != 1)
      throw std::runtime_error("homogeneous coordinate not integral");

   return numerator(v.top()[0]) | primitive(v.slice(range_from(1)));
}

template <typename TMatrix>
typename GenericMatrix<TMatrix, Integer>::persistent_type
lattice_basis(const GenericMatrix<TMatrix, Integer>& gens)
{
   const SmithNormalForm<Integer> SNF = smith_normal_form(gens);
   return (SNF.form * SNF.right_companion).minor(range(0, SNF.rank-1),All);
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
