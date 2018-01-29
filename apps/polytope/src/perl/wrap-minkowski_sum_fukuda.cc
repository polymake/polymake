/* Copyright (c) 1997-2018
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

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/QuadraticExtension.h"

namespace polymake { namespace polytope { namespace {
///==== Automatically generated contents follow.    Please do not delete this line. ====
   template <typename T0, typename T1>
   FunctionInterface4perl( zonotope_vertices_fukuda_T_X_o, T0,T1 ) {
      perl::Value arg0(stack[0]), arg1(stack[1]);
      WrapperReturn( (zonotope_vertices_fukuda<T0>(arg0.get<T1>(), arg1)) );
   };

   template <typename T0>
   FunctionInterface4perl( minkowski_sum_fukuda_T_x, T0 ) {
      perl::Value arg0(stack[0]);
      WrapperReturn( (minkowski_sum_fukuda<T0>(arg0)) );
   };

   FunctionInstance4perl(minkowski_sum_fukuda_T_x, Rational);
   FunctionInstance4perl(minkowski_sum_fukuda_T_x, QuadraticExtension< Rational >);
   FunctionInstance4perl(zonotope_vertices_fukuda_T_X_o, Rational, perl::Canned< const Matrix< Rational > >);
   FunctionInstance4perl(zonotope_vertices_fukuda_T_X_o, QuadraticExtension< Rational >, perl::Canned< const Matrix< QuadraticExtension< Rational > > >);

///==== Automatically generated contents end here.  Please do not delete this line. ====
}

// The following is a temporary hack enabling fan::tiling_quotient to call the functions from this client.
// llvm automatically hides any function template in a bundle unless its address is taken.
#ifdef __APPLE__
typedef Matrix<Rational>(*ptr1)(const Array<perl::Object>&);
ptr1 EnforceAddressIsTaken1() { return &minkowski_sum_vertices_fukuda<Rational>; }
typedef Matrix< QuadraticExtension<Rational> >(*ptr2)(const Array<perl::Object>&);
ptr2 EnforceAddressIsTaken2() { return &minkowski_sum_vertices_fukuda< QuadraticExtension<Rational> >; }
#endif

} }
