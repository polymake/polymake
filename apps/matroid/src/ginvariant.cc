/* Copyright (c) 1997-2015
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
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/vector"
#include "polymake/list"

namespace polymake { namespace matroid {

   // For a composition (a0,..,ar) computes the (ordered) support set
   // of the vector (0^a0, 1, 0^(a1-1),1,..,1,0^(ar-1)), 
   // where i^j means concatenate i j times.
   // This is of course just (a0, a0+a1,...a0+...+a(r-1)).
   Vector<int> set_from_composition(const Vector<int> &comp) {
      std::list<int> result;
      if (comp.dim() == 1)
         return Vector<int>({});
      int sum = 0;
      for(auto c  = entire(comp.slice(0,comp.dim()-1)); !c.at_end(); ++c) {
         sum += *c;
         result.push_back(sum);
      }
      return Vector<int>(result);
   }

   // The inverse function of set_from_composition 
   // I.e. given (ordered) set s = (s1,..,sr) returns the vector
   // (s1,s2-s1,s3-s2-s1,...,n-s(r-1)-...-s1)
   Vector<int> composition_from_set(const int n, const Vector<int> &s) {
      std::list<int> result;
      int sub = 0;
      for(auto el : s) {
         result.push_back(el - sub);
         sub = el;
      }
      result.push_back(n -sub);
      return Vector<int>(result);
   }

   // Computes the falling factorial t*(t-1)*...*(t-k+1)
   Integer falling_factorial(const Integer& t, const Integer& k) {
      if(t == 0 || k == 0) return 1;
      Integer result = t;
      for(int i = 1; i < k; i++) 
         result *= (t-i);
      return result;
   }


   // For two r-sets S,T in (0,..,n-1) we say that S <= T, if for all j = 0,...,n-1
   // we have |S \cap (0,..,j)| <= |T \cap (0,..,j)|
   // This computes, for a given set S, the list of all T >= S.
   // (Note that instead of sets we use ordered lists)
   Matrix<int> upper_interval(const Vector<int> &ordered_list) {
      //Convert the set to an ordered list
      Vector<int> current_vector = ordered_list;
      int r = ordered_list.dim();
      ListMatrix<Vector<int>> result;
      //Matrix<int> final_result;
      result /= current_vector;
      const Vector<int> max_elt(sequence(0,r));
      while( current_vector != max_elt) {
         for(auto ol = ensure( current_vector,  (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin();
               !ol.at_end(); ++ol) {
            //Find the first entry that can be shifted to the left
            if( *ol > ol.index()) {
               (*ol)--;
               //Reset entries coming before that, respectively: Shift only as far left as we have to
               auto orig = entire(ordered_list);
               for(auto prev = ensure( current_vector,  (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin();
                     prev != ol; ++prev, ++orig) {
                  (*prev) = std::min(*orig, *ol - (ol.index() - prev.index())); 
               }
               break;
            }
         }
         result /= current_vector;
      }
      //final_result = std::move(result);
      return result;
   }

   //Computes [[CATENARY_G_INVARIANT]]
   Map< Vector<int>, Integer> catenary_g_invariant(perl::Object matroid) {
      perl::Object lattice_of_flats = matroid.give("LATTICE_OF_FLATS");
      const int rank = matroid.give("RANK");
      const IncidenceMatrix<> maximal_chains = call_function("graph::maximal_chains",lattice_of_flats);
      const IncidenceMatrix<> flats = lattice_of_flats.give("FACES");
      //Check whether flats are sorted bottom to top (-1) or the other way around (1)
      const bool forward_numbered = 
         flats.row(0).size() < flats.row(flats.rows()-1).size()? true : false;

      Map<Vector<int>, Integer> result;

      for(auto mc = entire(rows(maximal_chains)); !mc.at_end(); ++mc) {
         Vector<int> composition(rank+1);
         auto comp_it = entire(composition);
         auto face_chain = flats.minor(*mc,All);
         int last_size = 0;
         auto flat_forward = entire(rows(face_chain));
         auto flat_backward = rentire(rows(face_chain));
         for(;!flat_forward.at_end(); ++flat_forward, ++flat_backward, ++comp_it) {
            int new_size = forward_numbered? (*flat_forward).size() : (*flat_backward).size();
            (*comp_it) = new_size - last_size;
            last_size = new_size;
         }
         result[ composition ] += 1;
      }
      return result;
   }

   // Computes [[G_INVARIANT]] from [[CATENARY_G_INVARIANT]]
   // Based on the formula in [Bonin, Kung: G-invariant and catenary data..., p.3]
   Map< Set<int>, Integer> g_invariant_from_catenary( int n, const Map<Vector<int>, Integer> &catenary_map) {
      Map< Set<int>, Integer> result;
      for(auto&& kv_pair : catenary_map) {
         auto&& key = kv_pair.first;
         auto&& value = kv_pair.second;
         const Matrix<int> uintv = upper_interval( set_from_composition(key) );
         for(auto&& bset = entire(rows(uintv)); !bset.at_end(); ++bset) {
            const Vector<int> bcomp = composition_from_set(n,*bset);
            auto&& a_it = entire(key);
            auto&& b_it = entire(bcomp);
            Integer coeff = falling_factorial( *a_it, *b_it);
            Integer diff_sum = *a_it - *b_it;
            ++a_it; ++b_it;
            for(;!a_it.at_end(); ++a_it, ++b_it) {
               coeff *= ( *a_it * falling_factorial( *a_it - 1 + diff_sum, *b_it - 1));
               diff_sum += ( *a_it - *b_it);
            }
            const Set<int> dummy(entire(*bset));
            result[ dummy ] += value*coeff;
         }

      }

      return result;
   }

   Function4perl(&catenary_g_invariant, "catenary_g_invariant(Matroid)");
   //Function4perl(&set_from_composition, "set_from_composition(Vector<Int>)");
   //Function4perl(&falling_factorial, "ff(Integer, Integer)");
   //Function4perl(&upper_interval, "upper_interval(Vector<Int>)");
   //Function4perl(&composition_from_set, "cfs($,Vector<Int>)");
   Function4perl(&g_invariant_from_catenary, "g_invariant_from_catenary($, Map<Vector<Int>, Integer>)");
}}
