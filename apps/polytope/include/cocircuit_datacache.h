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

#ifndef POLYMAKE_POLYTOPE_COCIRCUIT_DATACACHE_H
#define POLYMAKE_POLYTOPE_COCIRCUIT_DATACACHE_H

#include "polymake/group/permlib.h"
#include "polymake/hash_map"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Bitset.h"

namespace polymake { namespace polytope {

typedef std::list<boost::shared_ptr<permlib::OrbitAsSet> > OrbitList;

template<typename CacheType, typename SetType>
SetType build(const CacheType& cache, const SetType& s, SetType) 
{
   return cache.get_group().lex_min_representative(s);
}

template<typename CacheType, typename SetType>
const boost::shared_ptr<permlib::Permutation> build(const CacheType& cache, const std::pair<SetType, SetType >& i, boost::shared_ptr<permlib::Permutation>) 
{
   return permlib::setImage(*(cache.get_group_ptr()), i.first.begin(), i.first.end(), i.second.begin(), i.second.end());
}

template<typename CacheType, typename SetType>
boost::shared_ptr<permlib::PermutationGroup> build(const CacheType& cache, const SetType& s, boost::shared_ptr<permlib::PermutationGroup>) 
{
   return permlib::setStabilizer(*(cache.get_group_ptr()), s.begin(), s.end());
}

template<typename CacheType, typename SetType>
const OrbitList build(const CacheType& cache, const SetType& s, OrbitList) 
{
   return permlib::orbits(*(cache.setStabilizer(s)));
}




template <typename Scalar, typename SetType>
class DataCache 
{
public:
   typedef Scalar ScalarType;

   typedef std::pair<SetType, SetType > IndexType;
   typedef hash_map<SetType, SetType> SetMapType;
   typedef hash_map<IndexType, boost::shared_ptr<permlib::Permutation> > PermMapType;
   typedef hash_map<SetType, boost::shared_ptr<permlib::PermutationGroup> > GroupMapType;
   typedef hash_map<SetType, size_t> OrderMapType;

   DataCache(const Matrix<Scalar>& _V, const IncidenceMatrix<>& _VIF, const group::PermlibGroup& _group) 
         : V(_V)
         , VIF(_VIF)
         , group(_group) 
         , permlib_group(_group.get_permlib_group())
         , lex_min_cache(SetMapType())
         , stab_cache(GroupMapType())
         , stab_order_cache(OrderMapType())
         , lc(0)
         , stc(0)
         , orderc(0)
   {}

   template <typename ReturnValue, typename MapType, typename KeyType>
   const ReturnValue insert_or_build(MapType& C, const KeyType& s) {
      /*
      typename const MapType::const_iterator it = C.find(s);
      if (it != C.end()) return it->second;
      const ReturnValue result = build(*this, s, ReturnValue());
      if (grow_cache) C.insert(s, result);
      return result;
   }
      */
      const typename MapType::const_iterator it = C.find(s);
      return (it != C.end()) 
         ? it->second
         : C.insert(s, build(*this, s, ReturnValue()))->second;
   }

   const SetType lex_min_representative(const SetType& s) {
      ++lc;
      return insert_or_build<SetType >(lex_min_cache, s);
   }

   const boost::shared_ptr<permlib::PermutationGroup> setStabilizer(const SetType& ridge_rep) {
      ++stc;
      return insert_or_build<boost::shared_ptr<permlib::PermutationGroup> >(stab_cache, ridge_rep);
   }

   // TODO: Templatify also the next functions.

   // Here the problem is that the member function setStabilizer is called, but it is not const.
   // One could probably get around this with some fiddling with const and mutable, but right
   // now it's not worth the time.
   const size_t stabilizer_order(const SetType& facet_rep) {
      ++orderc;
      // return insert_or_build<OrbitList>(stab_orbit_cache, ridge_rep);
      typename OrderMapType::const_iterator it = stab_order_cache.find(facet_rep);
      return (it != stab_order_cache.end())
         ? it->second
         : stab_order_cache.insert(facet_rep, setStabilizer(facet_rep)->order())->second;
   }

   void clear() {
      lex_min_cache.clear();
      stab_cache.clear();
      stab_order_cache.clear();
   }

   const Matrix<Scalar>& get_V() const { return V; }
   const IncidenceMatrix<>& get_VIF() const { return VIF; }
   const group::PermlibGroup& get_group() const { return group; }
   const boost::shared_ptr<permlib::PermutationGroup>& get_group_ptr() const { return permlib_group; }

   std::vector<double> stats() const {
      std::vector<double> s;
      s.push_back(lc / double(lex_min_cache.size()));
      s.push_back(stc / double(stab_cache.size()));
      s.push_back(orderc / double(stab_order_cache.size()));
      return s;
   }

protected:
   
   Matrix<Scalar> V;
   IncidenceMatrix<> VIF;
   group::PermlibGroup group;
   boost::shared_ptr<permlib::PermutationGroup> permlib_group;

   SetMapType lex_min_cache;
   GroupMapType stab_cache;
   OrderMapType stab_order_cache;
   int lc, stc, orderc;
};



} }

#endif // POLYMAKE_POLYTOPE_COCIRCUIT_DATACACHE_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

 
