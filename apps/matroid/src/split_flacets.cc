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
#include "polymake/Set.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"

namespace polymake { namespace matroid {

void split_flacets(BigObject m)
{
   // We use a unique linear representation for the facets.
   // From that representation it is easy to recognize the splits.

   const Int n = m.give("N_ELEMENTS");
   const Int rk = m.give("RANK");
   const Matrix<Rational> facets = m.give("POLYTOPE.FACETS");
   const Matrix<Rational> aff_hull = m.give("POLYTOPE.AFFINE_HULL");
   const Array<Set<Int>> components = m.give("CONNECTED_COMPONENTS");

   // the equations which describes the AFFINE_HULL
   Set<Int> reps;
   Matrix<Rational> eq(components.size(),n+1);
   for (Int i = 0; i < components.size(); ++i) {
      reps += components[i].front()+1;
   }
   for (Int i = 0; i < components.size(); ++i) {
      const auto v = -unit_vector<Rational>(n+1,components[i].front()+1);
      eq.row(i) = lin_solve(T(aff_hull.minor(sequence(0, eq.rows()), reps)), v.slice(reps)) * aff_hull;
   }

   Array<Set<Set<Int>>> sf(rk);      
   for (auto rit = entire(rows(facets)); !rit.at_end(); ++rit) {
      Vector<Rational> f(*rit);
      // special form of f: orth. to AFF_HULL.minor(All,~[0])
      // note that the support of f.slice(range_from(1)) exactly coincides with one component of the matroid
      for (Int i = 0; i < components.size(); ++i) {
         f -= ( eq.row(i).slice(range_from(1)) * f.slice(range_from(1)) /components[i].size()) * eq.row(i);
      }
      Set<Int> positive, negative;
      for (Int i = 0; i < n; ++i) { // add 1 for proper labeling
         const Rational& coeff = f[i+1];
         if (coeff > 0)
            positive += i;
         else if (coeff < 0)
            negative += i;
      }
      if (positive.size() > 1 && negative.size() > 1) {
         for (Int i = 0; i < components.size(); ++i) {
            if (components[i].contains(positive.front()+1)) {
               const Rational sf_rank =(f[0]+eq.row(i)[0]*f[positive.front()+1])/(f[positive.front()+1]-f[negative.front()+1]);
               sf[Int(sf_rank.floor())] += negative;
               break;
            }
         }
      }
   }
   
   m.take("SPLIT_FLACETS") <<  Array<Array<Set<Int>>>(sf);
}


bool split_compatibility_check(BigObject m)
{
   const Array<Array<Set<Int>>> sf = m.give("SPLIT_FLACETS");
   const Array<Set<Int>> components = m.give("CONNECTED_COMPONENTS");
   const Array<Set<Int>> circuits = m.give("CIRCUITS");

   if (circuits.size() == 0) { // uniform matroid of rank d=n on n elements
      return true;
   }

   Array<Int> rk(components.size());
   Int rank = 0;
   for (Int i = 0; i < components.size(); ++i) {
       auto set = components[i];
       for (auto it = entire(circuits); !it.at_end(); ++it) {
           if (incl(*it,set)<1) {
               set-= it->front();
           }
       }
       rk[i] = set.size();
   }
   for (Int i = 0; i < components.size(); ++i) {
      rank += rk[i];
   }

   for (Int f_rk = 0; f_rk < rank; ++f_rk) {
      const Array<Set<Int>> F = sf[f_rk];
      for (Int g_rk = 0; g_rk <= f_rk; ++g_rk) {
         const Array<Set<Int>> G = sf[g_rk];
         const Int val2 = f_rk - g_rk;
         for (Int f = 0; f < F.size(); ++f) {
            Int i = 0;
            while ((F[f] * components[i]).empty()) {
               ++i;
            }
            const Int n = components[i].size();
            const Int val1 = f_rk + g_rk - rk[i];
            for (Int g = 0; g < G.size(); ++g) {
               if (!(G[g]*components[i]).empty()) {
		  if (f_rk == g_rk && f == g) {
                     break;
                  }
                  if ((G[g]*F[f]).size() <= val1)
                     continue;
                  if (F[f].size()-(G[g]*F[f]).size() <= val2)
                     continue;
                  if ( G[g].size()-(G[g]*F[f]).size() <= -val2)
                     continue;
                  if (n-(G[g]+F[f]).size() <= -val1)
                     continue;
                  return false;
               } else {
                   return false; // splits in distinct components
               }
            }
         }
      }
   }
   return true;
}

Function4perl(&split_compatibility_check,"split_compatibility_check(Matroid)");

Function4perl(&split_flacets,"split_flacets(Matroid)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
