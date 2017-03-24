/* Copyright (c) 1997-2016
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

#ifndef __POLYMAKE_GROUP_INDUCED_ACTION_H
#define __POLYMAKE_GROUP_INDUCED_ACTION_H

#include "polymake/group/orbit.h"
#include "polymake/Array.h"
#include "polymake/hash_map"

namespace polymake { namespace group {

namespace {

// a helper function to construct a new index_of if the given one is empty
template<typename DomainType, typename DomainConstIterator>
const hash_map<DomainType, int>&
valid_index_of(DomainConstIterator domain_it,
               const hash_map<DomainType, int>& _index_of,
               hash_map<DomainType, int>& _new_index_of)
{
   if (_index_of.size()) return _index_of;

   int i(0);
   for (DomainConstIterator d = domain_it; !d.at_end(); ++d, ++i) {
      _new_index_of[*d] = i;
   }
   return _new_index_of;
}

} // end anonymous namespace

template<typename op_tag, typename DomainType, typename PERM, typename DomainConstIterator>
Array<int>
induced_permutation_impl(const PERM& perm,
                         int domain_size,
                         DomainConstIterator domain_it,
                         const hash_map<DomainType, int>& _index_of)
{
   hash_map<DomainType, int> _new_index_of;
   const hash_map<DomainType, int>& index_of(valid_index_of(domain_it, _index_of, _new_index_of));

   Array<int> induced_perm(domain_size);
   const pm::operations::group::action<DomainType, op_tag, PERM> a(perm);
   try {
      for (auto& iperm : induced_perm)
         iperm = index_of[a(*domain_it)], ++domain_it;
   } catch (no_match) {
      std::ostringstream os;
      wrap(os) << "The given domain is not invariant under the permutation " << perm;
      throw no_match(os.str());
   }
   return induced_perm;
}


/*
  The following function takes a list of permutations and an iterator over a domain as arguments.
  To each element in the domain, all permutations are applied, and the induced permutation on the domain is recorded.

  The main use cases are:

  * converting an action on VERTICES to one on facets and vice versa, via

     DomainType          = Set<int>,
     DomainConstIterator = Entire<Rows<IncidenceMatrix<>>>::const_iterator,

  * converting an action on coordinates to an action on indices, for instance for VERTICES or FACETS, via

     DomainType          = Vector<Scalar>,
     DomainConstIterator = Entire<Rows<GenericMatrix<MatrixTop, Scalar>>>::const_iterator
*/    
template<typename op_tag, typename DomainType, typename DomainConstIterator>
Array<Array<int>> induced_permutations_impl(const Array<Array<int>>& original_permutations,
                                            int domain_size,
                                            DomainConstIterator domain_it,
                                            const hash_map<DomainType, int>& _index_of) 
{
   hash_map<DomainType, int> _new_index_of;
   const hash_map<DomainType, int>& index_of(valid_index_of(domain_it, _index_of, _new_index_of));

   Array<Array<int>> induced_permutations(original_permutations.size());
   Entire<Array<Array<int>>>::iterator iit = entire(induced_permutations);

   for (const auto& g : original_permutations)
      *iit = induced_permutation_impl<op_tag>(g, domain_size, domain_it, index_of), ++iit;

   return induced_permutations;
}


  }}

#endif // __POLYMAKE_GROUP_INDUCED_ACTION_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:


