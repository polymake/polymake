/* Copyright (c) 1997-2014
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

#ifndef POLYMAKE_POLYTOPE_BISECTOR_H
#define POLYMAKE_POLYTOPE_BISECTOR_H

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/AccurateFloat.h"

namespace polymake { namespace polytope {

/** Compute approximately the bisector hyperplane
 *
 * @param F1, F2 side hyperplanes of the angle
 * @param V point on the ridge
 */

template <typename Scalar, typename Vector1, typename Vector2, typename Vector3>
Vector<Scalar>
bisector(const GenericVector<Vector1,Scalar>& F1, const GenericVector<Vector2,Scalar>& F2,
         const GenericVector<Vector3,Scalar>& V)
{
   Vector<AccurateFloat> f1(F1), f2(F2);
   f1[0]=0; f2[0]=0;
   Vector<Scalar> F_bisector(f1/(2*sqrt(sqr(f1))) + f2/(2*sqrt(sqr(f2))));
   F_bisector[0]=-F_bisector*V;
   return F_bisector;
}

} }

#endif // POLYMAKE_POLYTOPE_BISECTOR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
