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

/** @file TropicalArithmetic.h
    @brief Implementation of classes relevant for tropical computations.
*/

#ifndef POLYMAKE_TROPICAL_ARITHMETIC_H
#define POLYMAKE_TROPICAL_ARITHMETIC_H

#include "polymake/Polynomial.h"
#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/graph/hungarian_method.h"

namespace polymake { namespace tropical {


typedef std::pair<Matrix<int>, Vector<Rational> > TropicalPolynomial;

typedef Ring<UniPolynomial<Rational,Rational>,int> PuiseuxRing;

typedef Polynomial<UniPolynomial<Rational,Rational>,int> PuiseuxPolynomial;

template <typename Dir, typename Scalar>
Scalar tdet(const Matrix<Scalar >& matrix)
{
  Scalar value(0); // empty matrix has tropical determinant zero
  const int d(matrix.rows());
  const Array<int > perm(graph::HungarianMethod<Scalar>(Dir::orientation()*matrix).stage());
  for(int k = 0; k < d; ++k) value += matrix[k][perm[k]];

  return value;
}

} }

#endif // POLYMAKE_TROPICAL_ARITHMETIC_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
