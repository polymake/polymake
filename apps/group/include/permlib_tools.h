/* Copyright (c) 1997-2021
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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include "permlib/common.h"
#include "permlib/permutation.h"
#include "permlib/bsgs.h"
#include "permlib/transversal/schreier_tree_transversal.h"
#include "permlib/construct/schreier_sims_construction.h"
#include "permlib/search/classic/set_stabilizer_search.h"
#include "permlib/predicate/subgroup_predicate.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace permlib {

template <typename INT>
permlib::dom_int safe_to_dom_int(INT i)
{
   if (i > static_cast<INT>(std::numeric_limits<permlib::dom_int>::max()))
      throw std::runtime_error("input is too big for permlib");
   return static_cast<permlib::dom_int>(i);
}

// the following could probably be incorporated into permlib, as follows.

/*  
    This function is recursively instantiated to evaluate the action of p on a nested container.
    An example usage is for containers of type Set<Set<Int>>, in which case this gets instantiated with
    PERM = permlib::Permutation; T = pm::Set<Int, pm::operations::cmp>; Comparator = pm::operations::cmp; Container = pm::Set]
    In this case, the function is then recursively instantiated with
    PERM = permlib::Permutation; T = Int; helper_type = pm::operations::cmp; Container = pm::Set
*/
template <class PERM, typename T, typename Comparator, template <typename, typename> class Container>
Container<T, Comparator> action_on_container(const PERM& p, const Container<T, Comparator>& c)
{
   Container<T, Comparator> image;
   for (const T& s : c) 
      image += action_on_container(p, s);
   return image;
}

template <typename Perm>
permlib::dom_int action_on_container(const Perm& p, long i)
{
   return p/safe_to_dom_int(i);
}

// A struct to pass into Orbit classes and their derivatives as ACTION parameter
template <class PERM, typename Container> 
struct ContainerAction {
   Container operator()(const PERM& p, const Container& c) const {
      return action_on_container(p, c);
   }
};


// ---------------------------------------------------------------------------------------
//    this part would go into a file permlib/predicate/set_system_stabilizer_predicate.h
// ---------------------------------------------------------------------------------------


template <class PERM>
class SetSystemStabilizerPredicateBase : public SubgroupPredicate<PERM> {
protected:
   int _n;

public:
   /// constructor
   /**
    * @param n     int   size of permutation
    */
   SetSystemStabilizerPredicateBase(int n) 
      :_n(n) {}

   virtual bool preserves_set_system(const PERM& p) const = 0;

   virtual bool operator()(const PERM &p) const {
      return preserves_set_system(p);
   }
   virtual bool childRestriction(const PERM &h, unsigned int i, unsigned long beta_i) const {
      return preserves_set_system(h);
   }
   virtual unsigned int limit() const {
      return this->_n;
   }
};

/// predicate for the subgroup that stabilizes a given set system
template <class PERM, typename T, typename Container>
class SetSystemStabilizerPredicate : public SetSystemStabilizerPredicateBase<PERM> {
protected:
   Container _set_system;

   virtual bool preserves_set_system(const PERM& p) const
   {
      for (const T& c : _set_system)
         if (!_set_system.exists(action_on_container(p, c))) 
            return false;
      return true;
   }

public:
   /// constructor
   /**
    * @param n     int   size of permutation
    * @param set_system   Container   the set system to be preserved
    */
   explicit
   SetSystemStabilizerPredicate (int n, const Container& set_system)
      : SetSystemStabilizerPredicateBase<PERM>(n)
      , _set_system(set_system) {}
};


/// predicate for the subgroup that stabilizes a given layered set system.
/// A layered set system is an ArrayType of Containers, each of which has to be stabilized individually.
/// The ArrayType nedds to have a method size(), and 
/// the Container needs to implement a method exists().
template <class PERM, typename Container, typename ArrayType>
class LayeredSetSystemStabilizerPredicate : public SetSystemStabilizerPredicateBase<PERM> {
protected:
   ArrayType _layered_set_system;

   virtual bool preserves_set_system(const PERM& p) const
   {
      for (int i=0; i<_layered_set_system.size(); ++i)
         for (const Container& c : _layered_set_system[i])
            if (!_layered_set_system[i].exists(action_on_container(p, c))) 
               return false;
      return true;
   }

public:
   /// constructor
   /**
    * @param n     int   size of permutation
    * @param layered_set_system   ArrayType  an array of set systems to be stabilized individually.
    */
   explicit
   LayeredSetSystemStabilizerPredicate (int n, const ArrayType& layered_set_system)
      : SetSystemStabilizerPredicateBase<PERM>(n)
      , _layered_set_system(layered_set_system) {}
};


// -----------------------------------------------------------------------------------------
//    this part would go into a file permlib/search/classic/set_system_stabilizer_search.h
// -----------------------------------------------------------------------------------------

namespace classic {

/// subgroup search for a set system stabilizer based on classical backtracking.
/// The nature of the stabilizer is given by the PredType.
template<class BSGSIN, class TRANSRET, typename PredType>
class SetSystemStabilizerSearch : public BacktrackSearch<BSGSIN, TRANSRET> {
public:
   typedef typename BacktrackSearch<BSGSIN, TRANSRET>::PERM PERM;
	
   /// constructor
   /**
    * @param bsgs BSGS of group
    * @param pruningLevelDCM level up to which expensive double coset minimality pruning is performed; zero to disable
    */
   SetSystemStabilizerSearch(const BSGSIN& bsgs, unsigned int pruningLevelDCM)
      : BacktrackSearch<BSGSIN,TRANSRET>(bsgs, pruningLevelDCM, false, false)
   {}
	
   /// initializes search
   /**
    * @param begin iterator(unsigned long) begin of the set to be stabilized
    * @param end iterator(unsigned long) end of the set to be stabilized
    */
   template <typename Container>
   void construct(int n, const Container& set_system) {
      PredType* stabPred = new PredType(n, set_system);
	
      this->m_limitLevel = stabPred->limit();
      this->m_limitBase = this->m_limitLevel;
      this->m_limitInitialized = true;
	
      BacktrackSearch<BSGSIN,TRANSRET>::construct(stabPred, false);
   }

};

} // end namespace classic
} // end namespace permlib


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:


