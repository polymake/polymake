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

#ifndef POLYMAKE_TROPICAL_2D_PRIMITIVES_H
#define POLYMAKE_TROPICAL_2D_PRIMITIVES_H

#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"

namespace polymake { namespace tropical {

void tropically_dehomogenize(Matrix<Rational>& M)
{
   const int d(M.cols());
   for (int i=M.rows()-1; i>=0; --i) {
      for (int j=d-1; j>=0; --j)
      M(i,j)-=M(i,0);
   }
}

// for the following primitives it is assumed that the vectors are 2d

// a is right of b == 1
inline
int right(const Vector<Rational>& a, const Vector<Rational>& b)
{
   return sign(a[0]-b[0]);
}

// a has higher y-coordinate than b == 1
inline
int y_above(const Vector<Rational>& a, const Vector<Rational>& b)
{
   return sign(a[1]-b[1]);
}

// a has higher (1,1)-ray than b == 1
inline
int ray_above(const Vector<Rational>& a, const Vector<Rational>& b)
{
   return sign(a[1]-a[0]+b[0]-b[1]);
}

// b and c both right of a, line segment from a to b below line segment from a to c == 1
int segment_below(const Vector<Rational>& a, const Vector<Rational>& b, const Vector<Rational>& c);


// b and c both left of a, line segment from a to b above line segment from a to c == 1
int segment_above(const Vector<Rational>& a, const Vector<Rational>& b, const Vector<Rational>& c);

} }

#endif // POLYMAKE_TROPICAL_2D_PRIMITIVES_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
