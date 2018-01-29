/* Copyright (c) 1997-2018
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
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/ListMatrix.h"

namespace polymake { namespace matroid {

perl::Object positroid_from_decorated_permutation(const Array<int>& perm,  const Set<int>& loops)
{
  const int n = perm.size();
  Set<int> set;
  perl::Object p("polytope::Polytope<Rational>");
  ListMatrix<Vector<Rational>> ineq_list;
  Vector<Rational> ineq(n+1);
  int j=0;
  for (auto i=entire(perm); !i.at_end(); ++i,++j) {
    if (loops.contains(j) && *i!=j)
       throw std::runtime_error("A loop has to be fix point of the permutation");
    if (*i<=j && !loops.contains(*i))
       set+=*i;
  }
  const int rank=set.size();
  j=0;
  for (auto it=entire(set); !it.at_end(); ++it,++j) {
     ineq[0]=-j;
     if(*it!=0){
        ineq.slice(1,*it)=ones_vector<Rational>(*it);
        ineq_list/=-ineq;
     }
     ineq=zero_vector<Rational>(n+1);
  }

  ineq[0]=-rank;
  ineq.slice(1)=ones_vector<Rational>(n);
  p.take("EQUATIONS") << ineq;

  for (int s=1;s<n;++s) {
     int j=0;
     set.clear();
     for (auto i=entire(perm); !i.at_end(); ++i, ++j) {
        if ((*i-s+n)%n<=(j+n-s)%n && !loops.contains(*i)) {
           if (*i>=s) {
              set+=*i;
           } else {
              set+=*i+n;
           }
        }
     }
     j=0;
     for (auto it=entire(set); !it.at_end(); ++it, ++j) {
        if (*it<n) {
           ineq=zero_vector<Rational>(n+1);
           ineq[0]=-j;
           if (*it!=s) {
              ineq.slice(s+1,*it-s)=ones_vector<Rational>(*it-s);
              ineq_list/=-ineq;
           }
        } else {
           ineq=ones_vector<Rational>(n+1);
           ineq.slice(*it-n+1,n-*it+s)=zero_vector<Rational>(n-*it+s);
           ineq[0]=-j;
           if (*it!=s)
              ineq_list/=-ineq;
        }
     }
  }


  for (auto it=entire(loops); !it.at_end(); ++it) {
     ineq_list/=-unit_vector<Rational>(n+1,1+*it);
  }

  for (int i=0; i<n; ++i) {
     ineq_list/=unit_vector<Rational>(n+1,1+i);
  }
  p.take("INEQUALITIES") << ineq_list;
  return call_function("matroid_from_matroid_polytope", p);
}

UserFunction4perl("# @category Producing a matroid from other objects"
                  "# Producing a positroid from a decorated permutation"
                  "# @param Array<Int> perm a permutation"
                  "# @param Set<Int> loops the loops/decoration"
                  "# @return Matroid",
                  &positroid_from_decorated_permutation, "positroid_from_decorated_permutation($, $)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
