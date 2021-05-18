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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"

namespace polymake { namespace common {

// coordinates are homogeneous, maximal cells not necessarily simplices
// returns false if not a triangulation
bool unimodular(const Matrix<Rational>& C, const Array<Set<Int>>& F)
{
   const Int d(C.cols());
   bool u(true);

   for (auto fi=entire(F); !fi.at_end(); ++fi) {
      if (fi->size() != d || abs( det(C.minor(*fi,All)) ) != 1) {
         u = false;
         break;
      }
   }

   return u;
}

Int n_unimodular(const Matrix<Rational>& C, const Array<Set<Int>>& F)
{
   const Int d(C.cols());
   Int n_unimodular = 0;

   for (auto fi = entire(F); !fi.at_end(); ++fi) {
      if (fi->size() == d && abs( det(C.minor(*fi,All)) ) == 1) {
         ++n_unimodular;
      }
   }

   return n_unimodular;
}

Function4perl(&unimodular,"unimodular");
Function4perl(&n_unimodular,"n_unimodular");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
