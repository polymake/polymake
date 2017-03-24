/* Copyright (c) 1997-2016
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

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/common/find_matrix_row_permutation.h"
#include "polymake/polytope/canonicalize.h"

namespace polymake { namespace polytope {

template <typename TVector>
typename std::enable_if<pm::check_container_feature<TVector, pm::sparse>::value>::type
canonicalize_rays(GenericVector<TVector>& V)
{
   auto it=V.top().begin();
   if (!it.at_end())
      canonicalize_oriented(it);
}

template <typename TVector>
typename std::enable_if<!pm::check_container_feature<TVector, pm::sparse>::value>::type
canonicalize_rays(GenericVector<TVector>& V)
{
   if (!V.top().empty())
      canonicalize_oriented(find_in_range_if(entire(V.top()), operations::non_zero()));
}

template <typename TVector>
void canonicalize_rays(GenericVector<TVector>&& V)
{
   canonicalize_rays(V);
}

template <typename TVector, typename E>
void canonicalize_facets(GenericVector<TVector, E>& V)
{
   canonicalize_oriented(find_in_range_if(entire(V.top()), operations::non_zero()));
}

template <typename TVector>
void canonicalize_facets(GenericVector<TVector, double>& V)
{
   V.top() /= sqrt(sqr(V.top()));
}

template <typename TVector>
void canonicalize_facets(GenericVector<TVector>&& V)
{
   canonicalize_facets(V);
}

template <typename TMatrix>
void canonicalize_rays(GenericMatrix<TMatrix>& M)
{
   if (M.cols() == 0 && M.rows() != 0)
      throw std::runtime_error("canonicalize_rays - ambient dimension is 0");
   for (auto r=entire(rows(M)); !r.at_end();  ++r)
      canonicalize_rays(*r);
}


template <typename TMatrix>
void canonicalize_facets(GenericMatrix<TMatrix>& M)
{
   if (M.cols() == 0 && M.rows() != 0)
      throw std::runtime_error("canonicalize_facets - ambient dimension is 0");
   for (auto r=entire(rows(M)); !r.at_end();  ++r)
      canonicalize_facets(*r);
}


template <typename TMatrix>
void orthogonalize_subspace(GenericMatrix<TMatrix>& M)
{
   orthogonalize_affine(entire(rows(M)));
}

FunctionTemplate4perl("canonicalize_rays(Vector&) : void");
FunctionTemplate4perl("canonicalize_rays(Matrix&) : void");
FunctionTemplate4perl("canonicalize_facets(Vector&) : void");
FunctionTemplate4perl("canonicalize_facets(Matrix&) : void");
FunctionTemplate4perl("orthogonalize_subspace(Matrix&) : void");
FunctionTemplate4perl("dehomogenize(Vector)");
FunctionTemplate4perl("dehomogenize(Matrix)");


template <typename Matrix2, typename E>
void orthogonalize_facets(Matrix<E>& F, const GenericMatrix<Matrix2,E>& AH)
{
   for (auto a=entire(rows(AH)); !a.at_end(); ++a) {
      const E s=sqr(a->slice(1));
      for (auto f=entire(rows(F)); !f.at_end(); ++f) {
         const E x = f->slice(1) * a->slice(1);
         if (!is_zero(x)) *f -= (x/s) * (*a);
      }
   }
}

template <typename Matrix1, typename Matrix2, typename Matrix3, typename E>
Array<int> find_representation_permutation(const GenericMatrix<Matrix1,E>& Facets, const GenericMatrix<Matrix2,E>& otherFacets,
                                  const GenericMatrix<Matrix3,E>& AH, bool dual)
{
   if ((Facets.rows()==0 || Facets.cols()==0) && (otherFacets.rows()==0 || otherFacets.cols()==0))
      return Array<int>();
   if (Facets.rows() != otherFacets.rows() || Facets.cols() != otherFacets.cols())
      throw no_match("find_representation_permutation: dimension mismatch");
   Matrix<E> F1(Facets), F2(otherFacets);
   if (AH.rows()) {
      orthogonalize_facets(F1, AH);
      orthogonalize_facets(F2, AH);
   }
   if (dual) {
      canonicalize_facets(F1);
      canonicalize_facets(F2);
   } else {
      canonicalize_rays(F1);
      canonicalize_rays(F2);
   }
   return find_permutation(rows(F1), rows(F2), typename common::matrix_elem_comparator<E>::type());
}

FunctionTemplate4perl("find_representation_permutation(Matrix, Matrix, Matrix,$)");


// rotation matrix: move the hyperplane with the given normal vector to (0 0 ... last_sign)
// CAUTION: resulting matrix is applicable to affine coordinates (from the right)!

template <typename Vector>
Matrix<double> rotate_hyperplane(const GenericVector<Vector>& F, int last_sign)
{
   Matrix<double> R(T(null_space_oriented(F.slice(1), last_sign)));
   orthogonalize(entire(cols(R)));
   normalize(entire(cols(R)));
   return R;
}

FunctionTemplate4perl("rotate_hyperplane(Vector; $=1)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
