/* Copyright (c) 1997-2023
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
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/list"

namespace polymake { namespace matroid {

BigObject uniform_matroid(const Int r, const Int n)
{
   if (n < 1)
      throw std::runtime_error("uniform_matroid: at least 1 element required");
   if (r < 0 || r > n)
      throw std::runtime_error("uniform_matroid: 0 <= r <= n required");

   const Int n_bases = Int(Integer::binom(n, r));
   const Array<Set<Int>> bases(n_bases, entire(all_subsets_of_k(sequence(0, n), r)));

   BigObject m("Matroid",
               "N_ELEMENTS", n,
               "RANK", r,
               "N_BASES", n_bases,
               "BASES", bases);
   m.set_description()<<"Uniform matroid of rank "<<r<<" on "<<n<<" elements."<<endl;
   return m; 
}

UserFunction4perl("# @category Producing a matroid from scratch\n"
                  "# Creates the uniform matroid of rank //r// with //n// elements."
                  "# @param Int r"
                  "# @param Int n"
                  "# @return Matroid",
                  &uniform_matroid, "uniform_matroid");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
