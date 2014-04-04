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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"

namespace polymake { namespace tropical {

perl::Object pseudovertices2poly(perl::Object t_in)
{
   Matrix<Rational> V=t_in.give("PSEUDOVERTICES");
   V.col(0)=ones_vector<Rational>(V.rows());
   perl::Object p_out("polytope::Polytope<Rational>");
   p_out.set_description()<<"Ordinary polytope of pseudovertices of "<<t_in.name()<<endl;
   p_out.take("POINTS") << V;
   return p_out;
}

UserFunction4perl("# @category Other"
                  "# Takes a tropical polytope //T// and interprets it in ordinary Euclidean space."
                  "# @param TropicalPolytope T"
                  "# @return Polytope",
                  &pseudovertices2poly,"pseudovertices2poly(TropicalPolytope)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
