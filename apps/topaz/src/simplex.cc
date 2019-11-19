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
#include "polymake/Set.h"

namespace polymake { namespace topaz {

perl::Object simplex(const int n)
{
   perl::Object p("SimplicialComplex");
   Array<Set<int> > facet(1);
   facet[0]=sequence(0,n+1);
   p.take("FACETS")<<facet;
   p.take("N_VERTICES")<<n+1;
   p.take("BALL")<<true;
   p.set_description()<<"Simplex of dimension "<<n<<"."<<endl;
   return p;
}

UserFunction4perl("# @category Producing from scratch"
                  "# A __simplex__ of dimension //d//."
                  "# @param Int d dimension"
                  "# @return SimplicialComplex",
                  &simplex, "simplex");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
