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

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/TropicalNumber.h"

namespace polymake{ namespace tropical {

template <typename Coefficient, typename VType>
Vector<Coefficient> thomog_vec(const GenericVector<VType, Coefficient>& affine, Int chart = 0, bool has_leading_coordinate = true)
{
   if (affine.dim() <= 1)
      return Vector<Coefficient>(affine);
   if (chart < 0 || chart > (affine.dim()-has_leading_coordinate))
      throw std::runtime_error("Invalid chart coordinate");

   Vector<Coefficient> proj(affine.dim()+1);
   proj.slice(~scalar2set(chart+has_leading_coordinate)) = affine;
   return proj;
}

template <typename Coefficient, typename MType>
Matrix<Coefficient> thomog(const GenericMatrix<MType, Coefficient>& affine, Int chart = 0, bool has_leading_coordinate = true)
{
   if (affine.rows() == 0)
      return Matrix<Coefficient>(0,affine.cols()+1);
   if (chart < 0 || chart > (affine.cols()-has_leading_coordinate))
      throw std::runtime_error("Invalid chart coordinate.");  

   Matrix<Coefficient> proj(affine.rows(), affine.cols()+1);
   proj.minor(All,~scalar2set(chart+has_leading_coordinate)) = affine;
   return proj;
}

template <typename Affine, typename Projective>
void tdehomog_elim_col(Affine&& affine, const Projective& proj, Int chart, bool has_leading_coordinate)
{
   auto elim_col = proj.begin();
   std::advance(elim_col, chart+has_leading_coordinate);
   auto it = entire(affine);
   if (has_leading_coordinate) ++it;
   for (; !it.at_end(); ++it)
      *it -= *elim_col;
}

// This does the inverse of thomog, i.e. removes a coordinate by shifting it to 0.
template <typename TMatrix, typename Coefficient>
Matrix<Coefficient> tdehomog(const GenericMatrix<TMatrix, Coefficient>& proj, Int chart = 0, bool has_leading_coordinate = true)
{
   if (chart < 0 || chart > (proj.cols()-has_leading_coordinate-1))
      throw std::runtime_error("Invalid chart coordinate");

   Matrix<Coefficient> affine = proj.minor(All, ~scalar2set(chart+has_leading_coordinate));
   tdehomog_elim_col(cols(affine), cols(proj), chart, has_leading_coordinate);
   return affine;
}

template <typename TVector, typename Coefficient>
Vector<Coefficient> tdehomog_vec(const GenericVector<TVector, Coefficient>& proj, Int chart = 0, bool has_leading_coordinate = true)
{
   if (proj.dim() <= 1)
      return Vector<Coefficient>();
   if (chart < 0 || chart > (proj.dim()-has_leading_coordinate-1)) 
      throw std::runtime_error("Invalid chart coordinate");

   Vector<Coefficient> affine = proj.slice(~scalar2set(chart+has_leading_coordinate));
   tdehomog_elim_col(affine, proj.top(), chart, has_leading_coordinate);
   return affine;
}

/*
 * @brief: scale the rows of a matrix so that
 * the first non-null entry of each row is tropical one
 */
template <typename Addition, typename Scalar, typename MatrixTop>
Matrix<TropicalNumber<Addition, Scalar>>
normalized_first(const GenericMatrix<MatrixTop, TropicalNumber<Addition, Scalar>>& homogeneous_points)
{
  using TNumber = TropicalNumber<Addition,Scalar>;

  Matrix<TNumber> result(homogeneous_points);
  for (auto r : rows(result)) {
    TNumber value;
    for (auto entry : r) {
      if (!is_zero(entry)) {
        value = entry;
        break;
      }
    }
    if (!is_zero(value)) r /= value;
  }
  return result;
}

/*
 * @brief: scale the rows of a matrix so that
 * the first non-null entry of each row is tropical one
 */
template <typename Addition, typename Scalar, typename VectorTop>
Vector<TropicalNumber<Addition, Scalar>>
normalized_first(const GenericVector<VectorTop, TropicalNumber<Addition, Scalar>>& homogeneous_point)
{
  using TNumber = TropicalNumber<Addition,Scalar>;

  Vector<TNumber> result(homogeneous_point);
  TNumber value;
  for (auto entry : result) {
    if (!is_zero(entry)) {
      value = entry;
      break;
    }
  }
  if (!is_zero(value)) result /= value;
  return result;
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
