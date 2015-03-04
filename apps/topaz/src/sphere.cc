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
#include "polymake/Array.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace topaz {
  
perl::Object sphere(const int d)
{
   perl::Object p("GeometricSimplicialComplex<Rational>");
   p.set_description() << "The " << d << "-dimensional sphere.\nRealized as the boundary of a "
                       << d+1 << "-simplex.\n";

   const Array< Set<int> > F(d+2, all_subsets_less_1(sequence(0,d+2)).begin());   
   Matrix<int> Geom(d+2,d+1);
   for (int i=0; i<d+1; ++i)
      Geom(i+1,i) = 1;
   
   p.take("FACETS") << F;
   p.take("DIM") << d;
   p.take("PURE") << 1;
   p.take("MANIFOLD") << 1;
   p.take("CLOSED_PSEUDO_MANIFOLD") << 1;
   p.take("ORIENTED_PSEUDO_MANIFOLD") << 1;
   p.take("SPHERE") << 1;
   p.take("COORDINATES") << Geom;
   return p;
}

UserFunction4perl("# @category Producing from scratch\n"
                  "# The //d//-dimensional __sphere__, realized as the boundary of the (//d//+1)-simplex.\n"
                  "# @param Int d dimension"
                  "# @return GeometricSimplicialComplex",
                  &sphere, "sphere($)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
