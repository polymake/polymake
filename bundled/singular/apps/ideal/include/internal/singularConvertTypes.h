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

#ifndef POLYMAKE_IDEAL_SINGULAR_CONVERT_TYPES_H
#define POLYMAKE_IDEAL_SINGULAR_CONVERT_TYPES_H



#include "polymake/ideal/singularInit.h"
#include "polymake/ideal/internal/singularRingManager.h"

#include <coeffs/coeffs.h>
#include <coeffs/longrat.h>

namespace polymake { 
namespace ideal {
namespace singular {
   

   
   // Convert functions:
   Rational convert_number_to_Rational(number singularNumber, ring singularRing);
   number convert_Rational_to_number(const Rational& gmpRational);
   Polynomial<> convert_poly_to_Polynomial(const poly singularPolynomial);
   std::pair<std::vector<Rational>, ListMatrix<Vector<int>>> convert_poly_to_vector_and_matrix(const poly q);
   poly convert_Polynomial_to_poly(const Polynomial<>& polymakePolynomial, ring singularRing);

   


} // end namespace singular
} // end namespace ideal
} // end namespace polymake


#endif
