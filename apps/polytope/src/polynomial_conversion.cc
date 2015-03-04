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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"

namespace polymake { namespace polytope {

template <typename E>
Vector<E> power_to_binomial_basis(const Vector<E>& PB)
{
  int d = PB.size()-1;
  if ( d <= -1 ) {  // trivial case
   return Vector<E>();
  }

  Vector<E> BB(1);
  BB[0] = 1;

  for ( int k = 1; k <= d; ++k ) {
    E l = 0;
    for ( int i = 0; i <=d; ++i )
      l += PB[i] * Integer::pow(k,i);
    for ( int j = 0; j < BB.size(); ++j ) 
      l -= Integer::binom(d+k-j,d) * BB[j];
    BB |= l;
  }

  return BB;
}

template <typename E>
Vector<E> binomial_to_power_basis(const Vector<E>& BB)
{
  int d = BB.size()-1;
  Vector<E> PB(d+1);
  if ( d <= 0 ) {  // trivial case
     if ( d == 0 ) 
        PB[0] = 1;
     return PB;
  }

  for ( int k = 0; k <= d; ++k ) {
    Vector<E> a(2);
    a[0] = d-k; a[1] = 1;
    for (int j = 1; j < d; j++ ) 
      a = (0|a) + (d-k-j) * (a|0); 
    PB += BB[k] * a;
  }

  return 1/(E)(Integer::fac(d)) * PB;
}

FunctionTemplate4perl("binomial_to_power_basis(Vector)");
FunctionTemplate4perl("power_to_binomial_basis(Vector)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
