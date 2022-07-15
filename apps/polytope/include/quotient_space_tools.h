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

#pragma once

#include "polymake/hash_set"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/group/permlib.h"

namespace polymake { namespace polytope {

Array<Array<Int>> induced_symmetry_group_generators(Int n_, 
                                                    const Array<Array<Int>>& sym_group_generators, 
                                                    const Array<Set<Set<Set<Int>>>>& face_orbits)
{
   const permlib::dom_int n = permlib::safe_to_dom_int(n_);
   typedef permlib::Permutation PERM;
   typedef permlib::SchreierTreeTransversal<PERM> TRANSVERSAL;
   typedef permlib::BSGS<PERM, TRANSVERSAL> BSGSType;

   // construct BSGS of symmetry group
   permlib::SchreierSimsConstruction<PERM, TRANSVERSAL> schreierSims(n);
   std::list<PERM::ptr> gen_list;
   for (auto perm = entire(sym_group_generators); !perm.at_end(); ++perm){
      PERM::ptr gen(new PERM((*perm).begin(), (*perm).end()));
      gen_list.push_back(gen);
   }

   BSGSType sym_bsgs = schreierSims.construct(gen_list.begin(), gen_list.end());


   // prepare search without DCM pruning
   typedef Set<Set<Int>> Container;
   typedef Array<Set<Container>> ArrayType;

   typedef permlib::LayeredSetSystemStabilizerPredicate<PERM, Container, ArrayType> PredType;
   permlib::classic::SetSystemStabilizerSearch<BSGSType, TRANSVERSAL, PredType> backtrackSearch(sym_bsgs, 0);
   backtrackSearch.construct(n, face_orbits);
        
   // start the search
   BSGSType stabilizer(n);
   backtrackSearch.search(stabilizer);

   // extract a strong generating set
   Array<Array<Int>> new_bsgs(stabilizer.S.size());
   auto pit = entire(new_bsgs);
   for (const auto& perm : stabilizer.S)
      *pit++ = group::PermlibGroup::perm2Array(perm);
   return new_bsgs;
}
      
} } // end namespaces


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:


