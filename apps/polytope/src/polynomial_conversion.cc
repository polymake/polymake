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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"

namespace polymake { namespace polytope {

template <typename VectorTop, typename E>
typename GenericVector<VectorTop, typename pm::algebraic_traits<E>::field_type>::persistent_type
power_to_binomial_basis(const GenericVector<VectorTop,E>& PB)
{
  typedef typename pm::algebraic_traits<E>::field_type Ef;
  typedef typename GenericVector<VectorTop, Ef>::persistent_type Vec;
  int d = PB.top().dim()-1;
  if ( d <= -1 ) {  // trivial case
   return Vec();
  }

  Vec BB(1);
  BB[0] = 1;

  for ( int k = 1; k <= d; ++k ) {
    Ef l = 0;
    for ( int i = 0; i <=d; ++i )
      l += PB.top()[i] * Integer::pow(k,i);
    for ( int j = 0; j < BB.dim(); ++j ) 
      l -= Integer::binom(d+k-j,d) * BB[j];
    BB |= l;
  }

  return BB;
}

template <typename VectorTop, typename E>
typename GenericVector<VectorTop, typename pm::algebraic_traits<E>::field_type>::persistent_type
binomial_to_power_basis(const GenericVector<VectorTop,E>& BB)
{
  typedef typename pm::algebraic_traits<E>::field_type Ef;
  typedef typename GenericVector<VectorTop, Ef>::persistent_type Vec;

  int d = BB.top().dim()-1;
  Vec PB(d+1);
  if ( d <= 0 ) {  // trivial case
     if ( d == 0 ) 
        PB[0] = 1;
     return PB;
  }

  for ( int k = 0; k <= d; ++k ) {
    Vec a(2);
    a[0] = d-k; a[1] = 1;
    for (int j = 1; j < d; j++ )
      a = (0|a) + (d-k-j) * (a|0); 
    PB += BB.top()[k] * a;
  }

  return 1/(Ef)(Integer::fac(d)) * PB;
}

FunctionTemplate4perl("binomial_to_power_basis(Vector)");
FunctionTemplate4perl("power_to_binomial_basis(Vector)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
