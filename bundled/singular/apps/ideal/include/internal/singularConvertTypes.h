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



#include "polymake/ideal/singularInit.h"
#include "polymake/ideal/internal/singularRingManager.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include <coeffs/coeffs.h>
#include <coeffs/longrat.h>

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace polymake { 
namespace ideal {
namespace singular {
   

   
// Convert functions:
Rational convert_number_to_Rational(number singularNumber, ring singularRing);
number convert_Rational_to_number(const Rational& gmpRational);
Polynomial<> convert_poly_to_Polynomial(const poly singularPolynomial);
std::pair<std::vector<Rational>, ListMatrix<Vector<Int>>> convert_poly_to_vector_and_matrix(const poly q);
poly convert_Polynomial_to_poly(const Polynomial<>& polymakePolynomial, ring singularRing);

   


} // end namespace singular
} // end namespace ideal
} // end namespace polymake


