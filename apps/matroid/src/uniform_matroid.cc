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
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/list"

namespace polymake { namespace matroid {

perl::Object uniform_matroid(const int r, const int n)
{

   if (n < 1)
      throw std::runtime_error("uniform_matroid: at least 1 element requiered");
   if (r < 1 || r > n)
      throw std::runtime_error("uniform_matroid: 1 <= r <= n required");


  perl::Object m("Matroid");
  m.set_description()<<"Uniform matroid of rank "<<r<<" on "<<n<<" elements."<<endl;
  m.take("N_ELEMENTS")<< n;
  m.take("RANK")<<r;
  // we already know the number of bases
  const int n_bases=Integer::binom(n,r).to_int();
  m.take("N_BASES") << n_bases;

  Array<Set<int> > bases(n_bases);
  int l=0;
  for (Entire< Subsets_of_k<const sequence&> >::const_iterator i=entire(all_subsets_of_k(sequence(0,n),r)); !i.at_end(); ++i)
    bases[l++]=*i;


  m.take("BASES") << bases;
  return m; 
}

UserFunction4perl("# @category Producing from scratch\n"
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
