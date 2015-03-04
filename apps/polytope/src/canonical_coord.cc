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

#include "polymake/client.h"
#include "polymake/linalg.h"
#include "polymake/permutations.h"
#include "polymake/polytope/canonicalize.h"

namespace polymake { namespace polytope {

template <typename Vector>
typename pm::enable_if<void, pm::check_container_feature<Vector,pm::sparse>::value>::type
canonicalize_rays(GenericVector<Vector>& V)
{
   typename Vector::iterator it=V.top().begin();
   if (!it.at_end())
         canonicalize_oriented(it);
}

template <typename Vector>
typename pm::disable_if<void, pm::check_container_feature<Vector,pm::sparse>::value>::type
canonicalize_rays(GenericVector<Vector>& V)
{
   if (!V.top().empty()) {
      canonicalize_oriented(find_if(entire(V.top()), operations::non_zero()));
   }
}

template <typename Vector, typename E> inline
void canonicalize_facets(GenericVector<Vector,E>& V)
{
   canonicalize_oriented(find_if(entire(V.top()), operations::non_zero()));
}

template <typename Vector> inline
void canonicalize_facets(GenericVector<Vector,double>& V)
{
   V.top() /= sqrt(sqr(V.top()));
}

template <typename Matrix> inline
void canonicalize_rays(GenericMatrix<Matrix>& M)
{
   for (typename Entire< Rows<Matrix> >::iterator r=entire(rows(M)); !r.at_end();  ++r)
      canonicalize_rays(r->top());
}


template <typename Matrix> inline
void canonicalize_facets(GenericMatrix<Matrix>& M)
{
   for (typename Entire< Rows<Matrix> >::iterator r=entire(rows(M)); !r.at_end();  ++r)
      canonicalize_facets(r->top());
}


template <typename Matrix> inline
void orthogonalize_subspace(GenericMatrix<Matrix>& M)
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
   for (typename Entire< Rows<Matrix2> >::const_iterator a=entire(rows(AH)); !a.at_end(); ++a) {
      const E s=sqr(a->slice(1));
      for (typename Entire< Rows< Matrix<E> > >::iterator f=entire(rows(F)); !f.at_end(); ++f) {
         const E x = f->slice(1) * a->slice(1);
         if (!is_zero(x)) *f -= (x/s) * (*a);
      }
   }
}

template <typename E>
struct coord_comparator {
   typedef operations::cmp type;
};

template <>
struct coord_comparator<double> {
   typedef operations::cmp_with_leeway type;
};

template <typename Matrix1, typename Matrix2, typename E>
Array<int> find_matrix_row_permutation(const GenericMatrix<Matrix1,E>& M1, const GenericMatrix<Matrix2,E>& M2)
{
   if (M1.rows() != M2.rows() || M1.cols() != M2.cols())
      throw no_match("find_matrix_row_permutation: dimension mismatch");
   return find_permutation(rows(M1), rows(M2), typename coord_comparator<E>::type());
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
   return find_permutation(rows(F1), rows(F2), typename coord_comparator<E>::type());
}

FunctionTemplate4perl("find_matrix_row_permutation(Matrix, Matrix)");
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
