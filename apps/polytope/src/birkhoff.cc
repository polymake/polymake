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
#include "polymake/permutations.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"

namespace polymake { namespace polytope {

perl::Object birkhoff(int n, bool even)
{
   perl::Object p("Polytope<Rational>");
   std::ostringstream out_name;
   out_name << "B" << n;
   p.set_name(out_name.str());
   if (even)
      p.set_description() << "Even Birkhoff polytope for n=" << n << endl;
   else
      p.set_description() << "Birkhoff polytope for n=" << n << endl;

   Matrix<int> V((even ? Integer::fac(n)/2 : Integer::fac(n)).to_int(),  n*n+1);
   Rows< Matrix<int> >::iterator v_i=rows(V).begin();
  
   AllPermutations<> perms(n);
   for (AllPermutations<>::const_iterator perm=entire(perms);  !perm.at_end();  ++perm, ++v_i) {
      *v_i = 1 | concat_rows( permutation_matrix<int>(*perm) );

      // only every second permutation is even
      if (even)
         ++perm;
   }
   p.take("VERTICES") << V;
   p.take("LINEALITY_SPACE") << Matrix<Rational>();
   return p;
}


UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Constructs the Birkhoff polytope of dimension //n//<sup>2</sup> (also called the"
                  "# assignment polytope, the polytope of doubly stochastic matrices, or the perfect matching polytope)."
                  "# @param Int n"
                  "# @param Bool even"
                  "# @return Polytope",
                  &birkhoff, "birkhoff($;$=0)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
