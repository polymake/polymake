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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/GenericMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace polytope {

template <typename MatrixTop, typename MatrixTop2>
Array<QuadraticExtension<typename MatrixTop::value_type>> facet_areas(const GenericMatrix<MatrixTop>& Vertices, const IncidenceMatrix<>& Vertices_In_Facets, const GenericMatrix<MatrixTop2>& Facets)

{
   using Field = typename MatrixTop::value_type;
   Array<QuadraticExtension<Field>> areas(Vertices_In_Facets.rows());
   Vector<Field> interior_point(accumulate(rows(Vertices), operations::add()));
   interior_point /= Vertices.rows();
   Int dim = Vertices.cols()-1; 
   for (Int i = 0; i < Vertices_In_Facets.rows(); ++i) {
      BigObject facet_cone("Polytope", mlist<Field>(), "VERTICES", Vertices.minor(Vertices_In_Facets.row(i), All) / interior_point);
      auto normal_vector = Facets.row(i);
      QuadraticExtension<Field> quad_ext(0,1, sqr(normal_vector.slice(range_from(1))));
      Field scaled_height(normal_vector * interior_point);
      QuadraticExtension<Field> inv_height(quad_ext / scaled_height);
      facet_cone.give("VOLUME") >> areas[i];
      areas[i] *= inv_height * dim;
   }
   return areas;
}
  
FunctionTemplate4perl("facet_areas(Matrix, IncidenceMatrix, Matrix)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
