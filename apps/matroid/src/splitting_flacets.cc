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
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/Set.h"

namespace polymake { namespace matroid {

void splitting_flacets(perl::Object m)
{
   const bool connected=m.give("CONNECTED");
   if (!connected) {
      std::runtime_error("implementation requires matroid to be connected");
   }

   // If the matroid is connected then the polytope is of dimension n-1.
   // This implies that each facet of the matroid polytope has a unique linear representation.
   // From that representation it is easy to recognize the splits.

   const int n=m.give("N_ELEMENTS");
   const int rk=m.give("RANK");
   const Matrix<Rational> facets=m.give("POLYTOPE.FACETS");

   // that's the equation which describes the AFFINE_HULL
   Vector<Rational> eq(same_element_vector<Rational>(-1,n+1)); eq[0]=rk;

   Set< Set<int> > sf;

   for (Entire< Rows<Matrix<Rational> > >::const_iterator rit = entire(rows(facets)); !rit.at_end(); ++rit) {
      Vector<Rational> f(*rit);
      if (f[0]!=0) f -= (f[0]/rk) * eq;
      Set<int> positive, negative;
      for (int i=0; i<n; ++i) { // add 1 for proper labeling
         const Rational& coeff(f[i+1]);
         if (coeff>0) positive += i;
         else if (coeff<0) negative += i;
      }
      if (positive.size()>1 && negative.size()>1) {
         assert(positive.size()+negative.size()==n);
         sf += negative;
      }
   }
   
   m.take("SPLITTING_FLACETS") << Array< Set<int> >(sf);
}

Function4perl(&splitting_flacets,"splitting_flacets(Matroid)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
