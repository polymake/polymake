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

#pragma once

#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/FacetList.h"

namespace polymake { namespace matroid {

template<typename Container>
bool check_basis_exchange_axiom_impl(const Container& bases, bool verbose = false)
{
   Set<Set<Int>> basis_set;
   for (auto bit = entire(bases); !bit.at_end(); ++bit)
      basis_set += *bit; // have to do it like this so that the comparison tree gets built properly
   
   for (auto bit1 = entire(bases); !bit1.at_end(); ++bit1) {
      for (auto bit2 = entire(bases); !bit2.at_end(); ++bit2) {
         const Set<Int> AmB = *bit1 - *bit2;
         const Set<Int> BmA = *bit2 - *bit1;
         for (auto ambit = entire(AmB); !ambit.at_end(); ++ambit) {
            bool verified = false;
            for (auto bmait = entire(BmA); !verified && !bmait.at_end(); ++bmait) {
               verified = basis_set.contains(*bit1 - *ambit + *bmait);
            }
            if (!verified) {
               if (verbose) {
                  cout << "The given set of bases\n" << basis_set
                       << "\nis not a matroid.\nProof: A=" << *bit1 << ", B=" << *bit2 << "; A-B contains " << *ambit << ", B-A=" << BmA 
                       << "; but A - " << *ambit << " + b is not a basis for any b in " << BmA << endl;
               }
               return false;
            }
         }
      }
   }
   return true;
}

template <typename SetType>
bool check_hyperplane_axiom_impl(const Array<SetType>& H, bool verbose = false)
{
   /*
     The hyperplane axioms are:
     (H1) E is not in H;
     (H2) No set in H properly contains any other;
     (H3) If  h1 ne h2 in H , and  x in E setminus (h1 cup h2) ,
          then there exists h in H such that (h1 intersect h2) union x subset h.
    */
   SetType E; // ground set
   for (auto hit = entire(H); !hit.at_end(); ++hit)
      E += *hit;

   for (auto pit=entire(all_subsets_of_k(H, 2)); !pit.at_end(); ++pit) {
      const Set<SetType> p(*pit);
      const SetType& h1(p.front()), h2(p.back());
      if( E==h1 || E==h2){
         if (verbose) cout << "The given sets H =\n" << H << endl 
                           << "do not form the sets of hyperplanes of a matroid, because the groud set is in H."  << endl;
         return false;
      }
      if (incl(h1,h2) != 2) {
         if (verbose) cout << "The given sets H =\n" << H << endl 
                           << "do not form the sets of hyperplanes of a matroid, because the sets " 
                           << h1 << " and " << h2 << " are not independent." << endl;
         return false;
      }
      const SetType C(E-h1-h2);
      for (auto sit = entire(C); !sit.at_end(); ++sit) {
         const SetType U(h1 * h2 + *sit);
         bool found_container(false);
         for (auto hit = entire(H); !hit.at_end() && !found_container; ++hit) {
            found_container = incl(U, *hit) <= 0;
         }
         if (!found_container) {
            if (verbose) cout << "The given sets H =\n" << H << endl 
                              << "do not form the sets of hyperplanes of a matroid, because " 
                              << "h1=" << h1 << ", h2=" << h2 << ", x=" << *sit 
                              << " do not satisfy that there exists h in H such that (h1 intersect h2) union x subset h." << endl;
            return false;
         }
      }
   }
   return true;
}

template <typename SetType>
bool check_flat_axiom_impl(const Array<SetType>& F, bool verbose = false)
{
   // Extract the hyperplanes from the flats, then check the hyperplane axioms.
   SetType E; // ground set
   for (auto fit = entire(F); !fit.at_end(); ++fit)
      E += *fit;

   FacetList HL(E.size());
   for (auto fit = entire(F); !fit.at_end(); ++fit)
      if (fit->size() != E.size())
         HL.insertMax(*fit);

   Array<Set<Int>> H(HL.size(), entire(HL));

   return check_hyperplane_axiom_impl(H, verbose);
}

template<typename Container>
bool check_circuits_axiom_impl(const Container& circuits, bool verbose = false){
	/*
	   The circuits axioms are:
	   (C1) The empty set is not in C;
	   (C2) If c, c' are in C, and c is a subset of c' then c=c'
	   (C3) If  c ne c' both in C and e is an element in there intersection   
	   then there exists c* in C such that c* si a subset of c cup c' - e
	   */
	int r = 0; 
	for (auto c1it = entire(circuits); !c1it.at_end(); ++c1it, ++r){
		// Check for the empty set
		if ((*c1it).empty()) {
			if (verbose) cout << "The given set of circuits\n" << circuits << "do not form a matroid.\nCircuit " << r << " is empty. " << endl;
			return false;
		}
	}

	for (auto c1it = entire(circuits); !c1it.at_end(); ++c1it){
		for (auto c2it = entire(circuits); !c2it.at_end(); ++c2it){
			if(c1it==c2it){break;}
			Set<Int> inter = (*c1it) * (*c2it);
			// Check for equal or contained supports   
			if (inter.size() == (*c1it).size() || inter.size() == (*c2it).size()) {
				if (verbose) cout << "The given set of circuits\n" << circuits << "do not form a matroid.\nCircuits " << *c1it << " and " << *c2it << " are contained in one another." << endl;
				return false;
			}

			// Check valuated circuit exchange axiom:
			for (auto e = entire(inter); !e.at_end(); e++) {
				bool found_one = false;
				// Go through circuits that don't contain e
				for (auto c3it = entire(circuits); !c3it.at_end(); ++c3it){
					if(!(*c3it).contains(*e) && incl(*c3it,(*c1it)+(*c2it))==-1){
						found_one = true;
						break;
					}
				}
				if (!found_one) {
					if (verbose) {
						cout << "The given set of circuits\n" << circuits << "do not form a matroid.\nThe Circuit exchange axiom fails for the circuits " << *c1it << " and " << *c2it << " wich have " << *e << " in their intersection." << endl;
					}
					return false;
				}
			}
		}
	}
	return true;
}

}}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
