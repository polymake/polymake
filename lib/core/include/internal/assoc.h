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

#include <stdexcept>

#include "polymake/TransformedContainer.h"
#include "polymake/GenericSet.h"

namespace pm {

template <typename> struct DefaultValueTag;

/** Exception type
    If an access operation on a read-only associative container, which should return a reference,
    fails due to a non-existent search key, an exception of this type is raised.
*/
class no_match : public std::runtime_error {
public:
   no_match() : std::runtime_error("key not found") {}
   no_match(const std::string& reason) : std::runtime_error(reason) { }
};

template <typename TMap>
struct map_masquerade_helper {
   typedef pair<const typename TMap::key_type, typename TMap::mapped_type> value_type;
   typedef operations::member<value_type, const typename TMap::key_type, &value_type::first> key_accessor;
   typedef operations::member<value_type, typename TMap::mapped_type, &value_type::second> value_accessor;
};

template <typename TMap>
class Keys
   : public modified_container_impl< Keys<TMap>,
                                     mlist< OperationTag< typename map_masquerade_helper<TMap>::key_accessor >,
                                            HiddenTag< TMap > > >
   , public GenericSet< Keys<TMap>, typename TMap::key_type, typename TMap::key_comparator_type> {
public:
   const typename TMap::key_comparator_type& get_comparator() const
   {
      return this->hidden().get_comparator();
   }
   bool contains(const typename TMap::key_type& x) const
   {
      return this->hidden().exists(x);
   }
};

template <typename TMap>
class Values
   : public modified_container_impl< Values<TMap>,
                                     mlist< OperationTag< typename map_masquerade_helper<TMap>::value_accessor >,
                                            HiddenTag< TMap > > > { };

template <typename TMap> inline
const Keys<TMap>& keys(const TMap& map)
{
   return reinterpret_cast<const Keys<TMap>&>(map);
}

template <typename TMap> inline
const Values<TMap>& values(const TMap& map)
{
   return reinterpret_cast<const Values<TMap>&>(map);
}

namespace operations {

template <typename TMapRef, typename TKey>
class associative_access {
protected:
   typename attrib<TMapRef>::minus_ref* map;
public:
   associative_access(std::remove_reference_t<TMapRef>* map_arg=nullptr) : map(map_arg) {}
   associative_access(const associative_access<std::remove_const_t<TMapRef>, TKey>& op) : map(op.map) {}

   typedef TKey argument_type;
   typedef typename inherit_ref<typename std::remove_reference_t<TMapRef>::mapped_type, TMapRef>::type result_type;

   result_type operator() (const TKey& k) const
   {
      return (*map)[k];
   }

   template <typename, typename> friend class associative_access;
};

template <typename TMapRef>
class associative_search {
protected:
   typename attrib<TMapRef>::minus_ref* map;

public:
   associative_search(typename attrib<TMapRef>::minus_ref* map_arg=nullptr) : map(map_arg) {}
   associative_search(const associative_search<typename attrib<TMapRef>::minus_const>& op) : map(op.map) {}

   typedef void argument_type;
   typedef bool result_type;

   template <typename IteratorPair, typename Operation>
   typename std::enable_if<check_iterator_feature<typename IteratorPair::first_type, end_sensitive>::value, bool>::type
   operator() (binary_transform_iterator<IteratorPair, Operation>& it) const
   {
      return ! (static_cast<typename IteratorPair::first_type&>(it)=map->find(*it.second)).at_end();
   }

   template <typename IteratorPair, typename Operation>
   typename std::enable_if<!check_iterator_feature<typename IteratorPair::first_type, end_sensitive>::value, bool>::type
   operator() (binary_transform_iterator<IteratorPair, Operation>& it) const
   {
      return (static_cast<typename IteratorPair::first_type&>(it)=map->find(*it.second)) != map->end();
   }

   template <typename> friend class associative_search;
};

} // end namespace operations

template <typename TMapRef, typename TKey>
struct operation_cross_const_helper< operations::associative_access<TMapRef, TKey> > {
   typedef operations::associative_access<typename attrib<TMapRef>::minus_const, TKey> operation;
   typedef operations::associative_access<typename attrib<TMapRef>::plus_const, TKey> const_operation;
};

template <typename TMapRef>
struct operation_cross_const_helper< operations::associative_search<TMapRef> > {
   typedef operations::associative_search<typename attrib<TMapRef>::minus_const> operation;
   typedef operations::associative_search<typename attrib<TMapRef>::plus_const> const_operation;
};

template <typename TMap, typename TKeys,
          bool is_multimap=TMap::is_multimap,
          bool keys_match=isomorphic_types<typename TMap::key_type, TKeys>::value>
class assoc_helper {};

template <typename TMap, typename TKeys>
class assoc_helper<TMap, TKeys, false, true> {
public:
   using result_type = typename inherit_const<typename TMap::mapped_type, TMap>::type&;

   result_type operator()(TMap& map, const TKeys& k) const
   {
      return impl(map, k, std::is_const<TMap>());
   }

protected:
   static result_type impl(TMap& map, const TKeys& k, std::false_type)
   {
      return map.insert(k)->second;
   }

   static result_type impl(TMap& map, const TKeys& k, std::true_type)
   {
      typename TMap::const_iterator e=map.find(k);
      if (e.at_end()) throw no_match();
      return e->second;
   }
};

template <typename TMap, size_t s>
class assoc_helper<TMap, char[s], false, true>
   : assoc_helper<TMap, typename TMap::key_type, false, true> {
public:
   using base_t = assoc_helper<TMap, typename TMap::key_type, false, true>;

   typename base_t::result_type operator()(TMap& map, const char (&k)[s]) const
   {
      return base_t::operator()(map, typename TMap::key_type(k));
   }
};

template <typename TMap, typename TKeys>
class assoc_helper<TMap, TKeys, false, false> {
public:
   typedef TransformedContainer<const TKeys&, operations::associative_access<TMap&, typename TKeys::value_type> > result_type;

   result_type operator()(TMap& map, const TKeys& keys)
   {
      return result_type(keys, &map);
   }
};

template <typename TMap, typename TKeys>
typename assoc_helper<const TMap, TKeys>::type
select_by_keys(const TMap& map, const TKeys& keys)
{
   return assoc_helper<const TMap, TKeys>()(map, keys);
}

template <typename TMap, typename TKeys> inline
typename assoc_helper<TMap, TKeys>::type
select_by_keys(TMap& map, const TKeys& keys)
{
   return assoc_helper<TMap, TKeys>()(map, keys);
}

struct is_map;

template <typename TMap>
struct hash_func<TMap, is_map> {
   size_t operator() (const TMap& m) const
   {
      hash_func<typename TMap::key_type> key_hasher;
      hash_func<typename TMap::mapped_type> mapped_hasher;

      size_t a(1);
      // we cannot depend on the keys being visited in any particular order because
      // the Map might be a hash_map, so we need a commutative and associative operation
      for (const auto& e : m)
         a += key_hasher(e.first) + mapped_hasher(e.second);
      return a;
   }
};


} // end namespace pm

namespace polymake {
using pm::Keys;
using pm::Values;
using pm::select_by_keys;
using pm::DefaultValueTag;
using pm::no_match;
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
