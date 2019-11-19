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

#include "polymake/client.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {
namespace {

template <typename E, typename Matrix1, typename Vector2> inline
void assign_facet_through_points(const GenericMatrix<Matrix1,E>& V, GenericVector<Vector2,E>& f)
{
   f=null_space(V)[0];
}

template <typename Matrix1, typename Vector2> inline
void assign_facet_through_points(const GenericMatrix<Matrix1,double>& V, GenericVector<Vector2,double>& f)
{
   const Matrix<double> F=null_space(V);
   if (F.rows() == 1) {
      f=F[0];
   } else {
      // least squares
      const int n=V.cols();
      f=inv(( 2*T(V)*V               | ones_vector<double>(n)) /
            ( ones_vector<double>(n) | 0.0                   ) )[n].slice(sequence(0, n));
   }
}

template <typename Scalar, typename IM>
Matrix<Scalar> compute(const Matrix<Scalar>& V, const Matrix<Scalar>& L, const Matrix<Scalar>& AH,
                       const GenericIncidenceMatrix<IM>& VIF)
{
   Matrix<Scalar> F(VIF.rows(), V.cols());
   typename Rows< Matrix<Scalar> >::iterator Fi=rows(F).begin();

   for (auto vertex_list=entire(rows(VIF)); !vertex_list.at_end();  ++vertex_list, ++Fi) {
      assign_facet_through_points(V.minor(*vertex_list,All) / L/ AH, Fi->top());
      int outer_vertex=(sequence(0,V.rows()) - *vertex_list).front();
      if (*Fi * V[outer_vertex] < 0) Fi->negate();
   }

   return F;
}

} // end anonymous namespace

template <typename Scalar>
void facets_from_incidence(perl::Object p)
{
   const Matrix<Scalar> V=p.give("RAYS"),
                        L=p.give("LINEALITY_SPACE");
   const IncidenceMatrix<> VIF=p.give("RAYS_IN_FACETS");

   Matrix<Scalar> AH=null_space(V/L);
   p.take("LINEAR_SPAN") << AH;

   if (AH.rows()) AH.col(0).fill(0);
   p.take("FACETS") << compute(V, L, AH, VIF);
}

template <typename Scalar>
void vertices_from_incidence(perl::Object p)
{
   const Matrix<Scalar> F=p.give("FACETS"), AH=p.give("LINEAR_SPAN");
   const IncidenceMatrix<> VIF=p.give("RAYS_IN_FACETS");

   Matrix<Scalar> L=null_space(F/AH);
   p.take("LINEALITY_SPACE") << L;

   p.take("RAYS") << compute(F, AH, L, T(VIF));
}

FunctionTemplate4perl("facets_from_incidence<Scalar> (Cone<Scalar>)");
FunctionTemplate4perl("vertices_from_incidence<Scalar> (Cone<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
