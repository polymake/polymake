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
#include "polymake/Vector.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

perl::Object fractional_knapsack(const Vector<Rational> b)
{
  const int d = b.dim()-1;
  if (d < 1)
    throw std::runtime_error("knapsack: dimension d >= 1 required");

  perl::Object p("Polytope<Rational>");
  p.set_description() << "knapsack " << b << endl;

  Matrix<Rational> F = b / (zero_vector<Rational>(d) | unit_matrix<Rational>(d));
    
  p.take("CONE_AMBIENT_DIM") << d+1;
  p.take("INEQUALITIES") << F;
  p.take("BOUNDED") << true;

  return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a knapsack polytope defined by one linear inequality (and non-negativity constraints)."
                  "# "
                  "# @param Vector<Rational> b linear inequality"
                  "# @return Polytope",
                  &fractional_knapsack, "fractional_knapsack");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
