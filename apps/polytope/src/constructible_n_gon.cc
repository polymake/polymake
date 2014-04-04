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
#include "polymake/QuadraticExtension.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

perl::Object constructible_n_gon(const int n)
{
   /// factorize n to decide of polygon constructible
   ///   throw std::runtime_error("Parameter needs to be product of power of 2 and distinct Fermat primes.");

   typedef QuadraticExtension<Rational> QE;
   Matrix<QE> EM(n,3);
   EM.col(0) = same_element_vector(1,n); EM(0,1) = 1;

   perl::Object p;
   
   switch(n) {
   case 3: {
      QE sqrt3_2(0, Rational(1,2), 3);
      EM(1,1) = EM(2,1) = Rational(-1,2);
      EM(1,2) = sqrt3_2; EM(2,2) = -sqrt3_2;
      break;
   }
   default:
      throw std::runtime_error("Not implemented.");
   }

   p = perl::Object("Polytope<QuadraticExtension>");
   p.take("VERTICES") << EM;
   p.take("LINEALITY_SPACE") << Matrix<QE>();

   p.take("BOUNDED") << true;
   p.take("FEASIBLE") << true;
   p.take("POINTED") << true;
   p.take("CENTERED") << true;

   p.set_description() << "regular " << n << "-gon" << endl;

   return p;
}


UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create exact constructible regular polygon."
                  "# @return Polytope",
                  &constructible_n_gon, "constructible_n_gon($)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
