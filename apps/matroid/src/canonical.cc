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

#include "polymake/client.h"
#include "polymake/TropicalNumber.h"
#include "polymake/linalg.h"
#include "polymake/permutations.h"

namespace polymake { namespace matroid {

template <typename TVector, typename Addition, typename Scalar>
void canonicalize_tropical_rays(GenericVector<TVector, TropicalNumber<Addition, Scalar>>& V)
{
  auto e = entire(V.top());
  for (;;) {
    if (e.at_end()) return;
    if (!is_zero(*e)) break;
    ++e;
  }
  if (*e  != TropicalNumber<Addition, Scalar>::one()) {
    const auto leading = *e;
    *e = TropicalNumber<Addition, Scalar>::one();
    while (!(++e).at_end())
      *e /= leading;
  }
}

template <typename TMatrix, typename Addition, typename Scalar>
void canonicalize_tropical_rays(GenericMatrix<TMatrix, TropicalNumber<Addition, Scalar>>& M)
{
  for (auto r=entire(rows(M)); !r.at_end(); ++r)
    canonicalize_tropical_rays(r->top());
}

FunctionTemplate4perl("canonicalize_tropical_rays(Vector&)");
FunctionTemplate4perl("canonicalize_tropical_rays(Matrix&)");

} }
