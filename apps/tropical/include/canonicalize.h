/* Copyright (c) 1997-2019
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

#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/TropicalNumber.h"
#include "polymake/polytope/canonicalize.h"

namespace polymake { namespace tropical {

// The following all canonicalize a matrix/vector of tropical numbers such that the first entry
// is zero.

template <typename TVector, typename Addition, typename Scalar>
void canonicalize_to_leading_zero(GenericVector<TVector, TropicalNumber<Addition, Scalar>>& V)
{
   if (!is_leading_zero(V)) {
      const auto first = *V.top().begin();
      V /= first;
   }
}

template <typename TVector, typename Addition, typename Scalar>
void canonicalize_to_leading_zero(GenericVector<TVector, TropicalNumber<Addition, Scalar>>&& V)
{
   canonicalize_to_leading_zero(V);
}

template <typename TMatrix, typename Addition, typename Scalar>
void canonicalize_to_leading_zero(GenericMatrix<TMatrix, TropicalNumber<Addition,Scalar>>& M)
{
   if (M.rows() == 0)
      throw std::runtime_error("point matrix may not be empty");
   for (auto r=entire(rows(M)); !r.at_end();  ++r)
      canonicalize_to_leading_zero(*r);
}

template <typename TMatrix, typename Addition, typename Scalar>
void canonicalize_to_leading_zero(GenericMatrix<TMatrix, TropicalNumber<Addition,Scalar>>&& M)
{
   canonicalize_to_leading_zero(M);
}

// Does the same as canonicalize_to_leading_zero, and checks and complains if there is a zero column
template <typename TMatrix, typename Addition, typename Scalar>
void canonicalize_to_leading_zero_and_check_columns(GenericMatrix<TMatrix, TropicalNumber<Addition, Scalar>>& M)
{
   for (auto c = entire(rows(T(M))); !c.at_end(); ++c) {
      if (support(*c).empty())
         throw std::runtime_error("The points can't all lie in the same boundary stratum of projective space. Maybe use a projection?");
   }
   return canonicalize_to_leading_zero(M);
}

template <typename TMatrix, typename Addition, typename Scalar>
void canonicalize_to_leading_zero_and_check_columns(GenericMatrix<TMatrix, TropicalNumber<Addition, Scalar>>&& M)
{
   canonicalize_to_leading_zero_and_check_columns(M);
}

// The following canonicalize matrices/vectors of scalars, but representing (finite) 
// tropical homogeneous coordinates.

template <typename TVector, typename Scalar>
void canonicalize_scalar_to_leading_zero(GenericVector<TVector, Scalar>& V)
{
   if (!is_leading_zero(V)) {
      const auto first = *V.top().begin();
      V -= same_element_vector(first, V.dim());
   }
}

template <typename TVector, typename Scalar>
void canonicalize_scalar_to_leading_zero(GenericVector<TVector, Scalar>&& V)
{
   canonicalize_scalar_to_leading_zero(V);
}

template <typename TMatrix, typename Scalar>
void canonicalize_scalar_to_leading_zero(GenericMatrix<TMatrix, Scalar>& M)
{
   if (M.rows() == 0)
      throw std::runtime_error("point matrix may not be empty");
   for (auto r=entire(rows(M)); !r.at_end();  ++r)
      canonicalize_scalar_to_leading_zero(*r);
}

template <typename TMatrix, typename Scalar>
void canonicalize_scalar_to_leading_zero(GenericMatrix<TMatrix, Scalar>&& M)
{
   canonicalize_scalar_to_leading_zero(M);
}

// FIXME Should this be swapped for min/max, i.e. should there be a nonpositive - or rather,
// a templated version of this?

template <typename TVector, typename Scalar>
void canonicalize_scalar_to_nonnegative(GenericVector<TVector, Scalar>& V)
{
   const auto x_min = accumulate(V.top(), operations::min());
   if (pm::check_container_feature<TVector, pm::sparse>::value
       ? x_min<0 || V.top().size()==V.dim()
       : !is_zero(x_min))
      V -= same_element_vector(x_min, V.dim());
}

template <typename TVector, typename Scalar>
void canonicalize_scalar_to_nonnegative(GenericVector<TVector, Scalar>&& V)
{
   canonicalize_scalar_to_nonnegative(V);
}

template <typename TMatrix, typename Scalar>
void canonicalize_scalar_to_nonnegative(GenericMatrix<TMatrix, Scalar>& M)
{
   if (M.rows() == 0)
      throw std::runtime_error("point matrix may not be empty");
   for (auto r=entire(rows(M)); !r.at_end();  ++r)
      canonicalize_scalar_to_nonnegative(*r);
}

template <typename TMatrix, typename Scalar>
void canonicalize_scalar_to_nonnegative(GenericMatrix<TMatrix, Scalar>&& M)
{
   canonicalize_scalar_to_nonnegative(M);
}

// Assumes as input a matrix of tropically homogeneous vectors with a leading 1/0 indicating
// vertex or far vertex. Will canonicalize M.minor(All,~[0]) to have first coordinate 0. 
// Then will canonicalize the full matrix as a usual vertex matrix.
template <typename TMatrix, typename Scalar>
void canonicalize_vertices_to_leading_zero(GenericMatrix<TMatrix, Scalar>& M)
{
  canonicalize_scalar_to_leading_zero(M.minor(All, range_from(1)));
  for (auto r = entire(rows(M)); !r.at_end(); ++r) {
    polytope::canonicalize_oriented( find_in_range_if(entire(*r), operations::non_zero()) );
  }
}

template <typename TMatrix, typename Scalar>
void canonicalize_vertices_to_leading_zero(GenericMatrix<TMatrix, Scalar>&& M)
{
   canonicalize_vertices_to_leading_zero(M);
}

template <typename TVector, typename Scalar>
void canonicalize_vertex_to_leading_zero(GenericVector<TVector,Scalar>& V)
{
   canonicalize_scalar_to_leading_zero(V.slice(1));
   polytope::canonicalize_oriented( find_in_range_if(entire(V.top()), operations::non_zero()) );
}

template <typename TVector, typename Scalar>
void canonicalize_vertex_to_leading_zero(GenericVector<TVector,Scalar>&& V)
{
   canonicalize_vertex_to_leading_zero(V);
}

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
