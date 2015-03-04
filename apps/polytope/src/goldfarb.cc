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
#include "polymake/SparseVector.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

perl::Object goldfarb(int d, const Rational& e, const Rational& g)
{
   // restriction on d, e, and g
   const int m=8 * sizeof(int) - 2; // maximal dimension that can be handled
   if (d < 1 || d > m) {
      std::ostringstream error;
      error << "goldfarb: 1 <= d <= " << m;
      throw std::runtime_error(error.str());
   }
   if (e>=Rational(1,2))
      throw std::runtime_error("goldfarb: e < 1/2");
   if (g>e/4)
      throw std::runtime_error("goldfarb: g <= e/4");

   perl::Object p("Polytope<Rational>");
   p.set_description() << "Goldfarb " << d << "-cube with parameters e=" << e << " and g=" << g << endl;

   Matrix<Rational> IE(4+2*(d-2),d+1);

   // the first 4 inequaleties
   IE(0,1)=1;
   IE(1,0)=1; IE(1,1)=-1;
   IE(2,1)=-e; IE(2,2)=1;
   IE(3,0)=1; IE(3,1)=-e; IE(3,2)=-1;
  
   for (int k=2; k<d; ++k) {
      int i=k*2;                  // row index
      IE(i,k-1)=e*g; IE(i,k)=-e; IE(i,k+1)=1;
      IE(i+1,0)=1; IE(i+1,k-1)=e*g; IE(i+1,k)=-e; IE(i+1,k+1)=-1;
   }
  
   p.take("INEQUALITIES") << IE;
   p.take("LP.LINEAR_OBJECTIVE") << unit_vector<Rational>(d+1,d);
   p.take("FEASIBLE") << 1;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produces a //d//-dimensional Goldfarb cube if //e//<1/2 and //g//<=e/4."
                  "# The Goldfarb cube is a combinatorial cube and yields a bad example"
                  "# for the Simplex Algorithm using the Shadow Vertex Pivoting Strategy."
                  "# Here we use the description as a deformed product due to Amenta and Ziegler."
                  "# For //e//<1/2 and //g//=0 we obtain the Klee-Minty cubes."
                  "# @param Int d the dimension"
                  "# @param Rational e"
                  "# @param Rational g"
                  "# @return Polytope"
                  "# @author Nikolaus Witte",

                  &goldfarb, "goldfarb($; $=1/3, $=((convert_to<Rational>($_[1]))/4))");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
