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
#include "polymake/IncidenceMatrix.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace polytope {

namespace {      

Int codegree_impl(const Int d, const IncidenceMatrix<>& PIF)
{
   for (Int c = 2; c <= d; ++c) {
      for (auto s = entire(all_subsets_of_k(sequence(0, PIF.cols()), c)); !s.at_end(); ++s) {
         bool in_a_facet = false;
         for (auto rit = entire(rows(PIF)); !rit.at_end() && !in_a_facet; ++rit)
            in_a_facet = incl(*s, *rit) <= 0;
         if (!in_a_facet)
            return c-1;
      }
   }
   throw std::runtime_error("codegree_impl: strange. Not every subset of size <= dim should be contained in the boundary.");
}

} // end anonymous namespace

Function4perl(&codegree_impl, "codegree_impl(Int IncidenceMatrix)");

                          
} }  // end namespaces


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
