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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"

namespace polymake { namespace tropical {

perl::Object tropical_matroid_polytope(perl::Object m,const Rational& value)
{
   const Array< Set<int> > bases=m.give("BASES");
   const int n_bases=bases.size();
   const int n_elements=m.give("N_ELEMENTS");

   perl::Object t("TropicalPolytope<Rational>");
   Matrix<Rational> V(n_bases,n_elements);

   for (int b=0; b<n_bases; ++b) {
      for (Entire< Set<int> >::const_iterator i=entire(bases[b]); !i.at_end(); ++i)
	 V(b,(*i))=value;
   }

   t.take("POINTS") << V;
   t.take("AMBIENT_DIM") << n_elements-1;

   return t;
}

InsertEmbeddedRule("REQUIRE_APPLICATION matroid\n\n");

UserFunction4perl("# @category Producing a tropical polytope"
		  "# Produce the tropical matroid polytope from a matroid //m//."
		  "# Each vertex corresponds to a basis of the matroid,"
		  "# the non-bases coordinates get value 0, the bases coordinates"
		  "# get value //v//, default is -1."
		  "# @param matroid::Matroid m"
		  "# @param Rational v value for the bases"
		  "# @return TropicalPolytope",
		  &tropical_matroid_polytope, "tropical_matroid_polytope(matroid::Matroid; $=-1)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
