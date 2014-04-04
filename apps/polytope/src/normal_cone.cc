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
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope  {

template <typename Coord>
perl::Object normal_cone(perl::Object p, int v)
{
   perl::Object c("Cone<Rational>"); 
   const IncidenceMatrix<> vfi=p.give("FACETS_THRU_VERTICES");
   const Matrix<Coord> m=p.give("FACETS");
   const Matrix<Coord> mm=m.minor(vfi.row(v),~scalar2set(0));
   c.take("RAYS")<<mm; // inner normal cone
   const Matrix<Coord> ls=p.give("AFFINE_HULL");
   c.take("LINEALITY_SPACE") << ls.minor(All,~scalar2set(0));
   const int dim=p.CallPolymakeMethod("AMBIENT_DIM");
   c.take("CONE_AMBIENT_DIM")<<dim;
   return c;
}

UserFunctionTemplate4perl("# @category Producing a cone"
                          "# Computes the outer normal cone of //p// at the vertex //v//."
                          "# @param Polytope p"
                          "# @param int v vertex number",
                          "normal_cone<Coord>(polytope::Polytope<Coord> $)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
