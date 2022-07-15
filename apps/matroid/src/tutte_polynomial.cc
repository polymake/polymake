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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Polynomial.h"
#include "polymake/matroid/deletion_contraction.h"

namespace polymake { namespace matroid {

namespace {

Set<Int> coloops_from_circuits(Int n, const Array<Set<Int>>& circuits)
{
   return sequence(0,n) - accumulate( circuits, operations::add());
}

Set<Int> loops_from_circuits(const Array<Set<Int>>& circuits)
{
   Set<Int> result;
   for (auto c = entire(circuits); !c.at_end(); ++c)
      if (c->size() == 1)
         result += *c;
   return result;
}

}

/*
 * @brief Computes the Tutte polynomial of a matroid
 * @param Int n Size of the ground set 0,..,n-1
 * @param Array<Set<Int>> circuits The circuits
 * @return Polynomial<Rational>
 */
Polynomial<Rational> tutte_polynomial_from_circuits(const Int n, const Array<Set<Int>>& circuits)
{
   if (n == 0) {
      return Polynomial<Rational>(1, 2);
   }
   Set<Int> coloops = coloops_from_circuits(n, circuits);
   if (coloops.size() > 0) {
      return Polynomial<Rational>(1, coloops.size() * unit_vector<Int>(2,0)) *
         tutte_polynomial_from_circuits(n - coloops.size(), 
                                        minor_circuits(Deletion(), circuits, coloops, relabeling_map(n, coloops)));
   }
   Set<Int> loops = loops_from_circuits(circuits);
   if (loops.size() > 0) {
      return Polynomial<Rational>(1, loops.size() * unit_vector<Int>(2,1)) *
         tutte_polynomial_from_circuits(n - loops.size(), 
                                        minor_circuits(Deletion(), circuits, loops, relabeling_map(n, loops)));
   }
   Set<Int> deleted_element = scalar2set(0);
   Map<Int, Int> label_map = relabeling_map(n, deleted_element);
   return 
      tutte_polynomial_from_circuits(n-1, 
                                     minor_circuits(Deletion(), circuits, deleted_element,label_map)) +
      tutte_polynomial_from_circuits(n-1,
                                     minor_circuits(Contraction(), circuits, deleted_element,label_map));
}

Function4perl(&tutte_polynomial_from_circuits, "tutte_polynomial_from_circuits($,Array<Set<Int> >)");                      

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
