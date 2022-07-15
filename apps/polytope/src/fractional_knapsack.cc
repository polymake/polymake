/* Copyright (c) 1997-2022
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
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

BigObject fractional_knapsack(const Vector<Rational> b)
{
  const Int d = b.dim()-1;
  if (d < 1)
    throw std::runtime_error("knapsack: dimension d >= 1 required");

  const Matrix<Rational> F = b / (zero_vector<Rational>(d) | unit_matrix<Rational>(d));
    
  BigObject p("Polytope<Rational>",
              "CONE_AMBIENT_DIM", d+1,
              "INEQUALITIES", F,
              "BOUNDED", true);
  p.set_description() << "knapsack " << b << endl;
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
