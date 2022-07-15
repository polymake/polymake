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
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/Rational.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace matroid {

Array<Set<Int>> dual_bases_from_bases(Int n_elements, const Array<Set<Int>>& bases)
{
   Array<Set<Int>> result(bases.size());
   Set<Int> total_set = sequence(0, n_elements);
   for (auto b = entire<indexed>(bases); !b.at_end(); ++b) {
      result[b.index()] = total_set - *b;
   }
   return result;
}

Array<Set<Int>> dual_circuits_from_bases(const Int n, const Array<Set<Int>>& bases)
{
   if (bases.empty()) {
      return Array<Set<Int>>();
   }
   const Int r = bases[0].size();
   if (r == 0) return Array<Set<Int>>(0);
   if (r == n) {
      Array<Set<Int>> c(n);
      for (Int i = 0; i < n; ++i)
         c[i] = scalar2set(i);
      return c;
   } 
   Int n_cocircuits = 0;
   std::vector<Set<Int>> cocircuits;
   for (Int k = 1; k <= n-r+1; ++k) {
      for (auto j = entire(all_subsets_of_k(sequence(0, n), k)); !j.at_end(); ++j) {
         bool is_cocircuit = true;
         for (auto i = entire(cocircuits); is_cocircuit && !i.at_end(); ++i)
            if (incl(*i,*j) <= 0) is_cocircuit = false;
         for (auto i = entire(bases); is_cocircuit && !i.at_end(); ++i) {
            if (((*i)*(*j)).empty()) is_cocircuit = false;
         }
         if (is_cocircuit) {
            cocircuits.push_back(Set<Int>(*j));
            ++n_cocircuits;
         }
      }
   }
   return Array<Set<Int>>(n_cocircuits, entire(cocircuits));
}

Array<Set<Int>> bases_from_dual_circuits(const Int n, const Array<Set<Int>>& cocircuits)
{
   if (cocircuits.empty()) return Array<Set<Int>>(1);
   Int n_bases = 0;
   std::vector<Set<Int>> bases;
   Int r = n;
   for (Int k = 1; k <= r; ++k) {
      for (auto j = entire(all_subsets_of_k(sequence(0, n), k)); !j.at_end(); ++j) {
         bool is_basis = true;
         for (auto i = entire(cocircuits); is_basis && !i.at_end(); ++i) {
            if (((*i)*(*j)).empty()) is_basis = false;
         }
         if (is_basis) {
            bases.push_back(Set<Int>(*j));
            ++n_bases;
            r = k;
         }
      }
   }
   return Array<Set<Int>>(n_bases, entire(bases));
}


Array<Set<Int>> bases_from_dual_circuits_and_rank(const Int n, const Int rank, const Array<Set<Int> >& cocircuits)
{
   if (cocircuits.empty()) return Array<Set<Int>>(1);
   Int n_bases = 0;
   std::vector<Set<Int>> bases;
   for (auto j=entire(all_subsets_of_k(sequence(0,n),rank)); !j.at_end(); ++j) {
      bool is_basis=true;
      for (auto i = entire(cocircuits); is_basis && !i.at_end(); ++i) {
         if (((*i)*(*j)).empty()) is_basis=false;
      }
      if (is_basis) {
         bases.push_back(Set<Int>(*j));
         ++n_bases;
      }
   }
   return Array<Set<Int>>(n_bases, entire(bases));
}


Function4perl(&dual_bases_from_bases, "dual_bases_from_bases");
Function4perl(&dual_circuits_from_bases, "dual_circuits_from_bases");
Function4perl(&bases_from_dual_circuits, "bases_from_dual_circuits");
Function4perl(&bases_from_dual_circuits_and_rank, "bases_from_dual_circuits_and_rank");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
