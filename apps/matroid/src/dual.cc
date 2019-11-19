/* Copyright (c) 1997-2019
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

Array<Set<int>> dual_bases_from_bases(int n_elements, const Array<Set<int>>& bases)
{
   Array<Set<int>> result(bases.size());
   Set<int> total_set = sequence(0,n_elements);
   for (auto b = entire<indexed>(bases); !b.at_end(); ++b) {
      result[b.index()] = total_set - *b;
   }
   return result;
}

Array<Set<int>> dual_circuits_from_bases(const int n, const Array<Set<int>>& bases)
{
   if (bases.empty()) {
      return Array<Set<int>>();
   }
   const int r=bases[0].size();
   if (r==0) return Array<Set<int>>(0);
   if (r==n) {
      Array<Set<int>> c(n);
      for (int i=0; i<n;++i)
         c[i]=scalar2set(i);
      return c;
   } 
   int n_cocircuits=0;
   std::vector<Set<int>> cocircuits;
   for (int k=1; k<=n-r+1; ++k) {
      for (auto j=entire(all_subsets_of_k(sequence(0,n),k)); !j.at_end(); ++j) {
         bool is_cocircuit=true;
         for (auto i=entire(cocircuits); is_cocircuit && !i.at_end(); ++i)
            if (incl(*i,*j)<=0) is_cocircuit=false;
         for (auto i = entire(bases); is_cocircuit && !i.at_end(); ++i) {
            if (((*i)*(*j)).empty()) is_cocircuit=false;
         }
         if (is_cocircuit) {
            cocircuits.push_back(Set<int>(*j));
            ++n_cocircuits;
         }
      }
   }
   return Array<Set<int>>(n_cocircuits, entire(cocircuits));
}

Array<Set<int>> bases_from_dual_circuits(const int n, const Array<Set<int>>& cocircuits)
{
   if (cocircuits.empty()) return Array<Set<int>>(1);
   int n_bases=0;
   std::vector<Set<int>> bases;
   int r=n;
   for (int k=1; k<=r; ++k) {
      for (auto j=entire(all_subsets_of_k(sequence(0,n),k)); !j.at_end(); ++j) {
         bool is_basis=true;
         for (auto i = entire(cocircuits); is_basis && !i.at_end(); ++i) {
            if (((*i)*(*j)).empty()) is_basis=false;
         }
         if (is_basis) {
            bases.push_back(Set<int>(*j));
            ++n_bases;
            r=k;
         }
      }
   }
   return Array<Set<int>>(n_bases,entire(bases));
}


Array<Set<int>> bases_from_dual_circuits_and_rank(const int n, const int rank, const Array<Set<int> >& cocircuits)
{
   if (cocircuits.empty()) return Array<Set<int>>(1);
   int n_bases=0;
   std::vector<Set<int>> bases;
   for (auto j=entire(all_subsets_of_k(sequence(0,n),rank)); !j.at_end(); ++j) {
      bool is_basis=true;
      for (auto i = entire(cocircuits); is_basis && !i.at_end(); ++i) {
         if (((*i)*(*j)).empty()) is_basis=false;
      }
      if (is_basis) {
         bases.push_back(Set<int>(*j));
         ++n_bases;
      }
   }
   return Array<Set<int>>(n_bases, entire(bases));
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
