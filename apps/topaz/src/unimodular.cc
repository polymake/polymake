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
#include "polymake/Array.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"

namespace polymake { namespace topaz {

bool unimodular(perl::Object p)
{
   const Matrix<Rational> C = p.give("COORDINATES");
   const Array< Set<int> > F = p.give("FACETS");
   const Vector<Rational> leading_col(ones_vector<Rational>(C.cols()+1));

   bool unimodular(true);

   for (auto fi=entire(F); !fi.at_end(); ++fi) {
      if (abs( det(leading_col|C.minor(*fi,All)) ) != 1) {
         unimodular = false;
         break;
      }
   }

   return unimodular;
}

int n_unimodular(perl::Object p)
{
   const Matrix<Rational> C = p.give("COORDINATES");
   const Array< Set<int> > F = p.give("FACETS");
   const Vector<Rational> leading_col(ones_vector<Rational>(C.cols()+1));

   int n_unimodular(0);

   for (auto fi=entire(F); !fi.at_end(); ++fi) {
      if (abs( det(leading_col|C.minor(*fi,All)) ) == 1) {
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
