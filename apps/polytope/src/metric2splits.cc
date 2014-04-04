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
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/list"

namespace polymake { namespace polytope {

typedef std::pair< Set<int>, Set<int> > PairOfSets;

Array<PairOfSets>
metric2splits(const Matrix<Rational>& dist)
{
  const int n(dist.cols());

  std::list<PairOfSets> coherent_splits;

  for (int k=2; 2*k<n; ++k) { // enumerate all possible splits
     for (Entire< Subsets_of_k<const sequence&> >::const_iterator s=entire(all_subsets_of_k(sequence(0,n),k)); !s.at_end(); ++s) {
        const Set<int> this_split_A=*s;
        const Set<int> this_split_B=sequence(0,n)-this_split_A;
        Rational alpha=-1;
        for (Entire< Subsets_of_k<const Set<int>&> >::const_iterator a=entire(all_subsets_of_k(this_split_A,2)); !a.at_end(); ++a) {
           int a1=a->front(), a2=a->back();
           for (Entire<Subsets_of_k<const Set<int>&> >::const_iterator b=entire(all_subsets_of_k(this_split_B,2)); !b.at_end(); ++b) {
              int b1=b->front(), b2=b->back();
              Rational diff=dist(a1,a2)+dist(b1,b2);
              Rational local_alpha=std::max(dist(a1,b1)+dist(a2,b2),std::max(dist(a1,b2)+dist(a2,b1),diff))-diff;
              alpha = alpha<0 ? local_alpha : std::min(alpha,local_alpha);
           }
        }
        if (alpha>0)
           coherent_splits.push_back(PairOfSets(this_split_A,this_split_B));
     }
  }
 //in the special case k=n/2 we do not want to get the splits twice
  if (n%2==0)
    for (Entire< Subsets_of_k<const sequence&> >::const_iterator s=entire(all_subsets_of_k(range(1,n-1),n/2-1)); !s.at_end(); ++s) {
      const Set<int> this_split_A=(*s)+scalar2set(0);
      const Set<int> this_split_B=sequence(0,n)-this_split_A;
      Rational alpha=-1;
      for (Entire< Subsets_of_k<const Set<int>&> >::const_iterator a=entire(all_subsets_of_k(this_split_A,2)); !a.at_end(); ++a) {
         int a1=a->front(), a2=a->back();
         for (Entire<Subsets_of_k<const Set<int>&> >::const_iterator b=entire(all_subsets_of_k(this_split_B,2)); !b.at_end(); ++b) {
            int b1=b->front(), b2=b->back();
            Rational diff=dist(a1,a2)+dist(b1,b2);
            Rational local_alpha=std::max(dist(a1,b1)+dist(a2,b2),std::max(dist(a1,b2)+dist(a2,b1),diff))-diff;
            alpha = alpha<0 ? local_alpha : std::min(alpha,local_alpha);
         }
      }
      if (alpha>0)
         coherent_splits.push_back(PairOfSets(this_split_A,this_split_B));
   }


  return Array<PairOfSets>(coherent_splits);
}

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Computes all non-trivial splits of a metric space //D// (encoded as a symmetric distance matrix)."
                  "# @param Matrix D"
                  "# @return Array<Pair<Set>> each split is encoded as a pair of two sets.",
                  &metric2splits, "metric2splits");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
