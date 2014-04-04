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

#ifndef POLYMAKE_INTERNAL_ASSOC_H
#define POLYMAKE_INTERNAL_ASSOC_H

#include <stdexcept>

#include "polymake/TransformedContainer.h"
#include "polymake/GenericSet.h"

namespace pm {

template <typename> struct DefaultValue;

/** Exception type
    If an access operation on a read-only associative container, which should return a reference,
    fails due to a non-existent search key, an exception of this type is raised.
*/
class no_match : public std::runtime_error {
public:
   no_match() : std::runtime_error("key not found") {}
   no_match(const std::string& reason) : std::runtime_error(reason) { }
};

template <typename Map>
struct map_masquerade_helper {
   typedef pair<const typename Map::key_type, typename Map::mapped_type> value_type;
   typedef operations::member<value_type, const typename Map::key_type, &value_type::first> key_accessor;
   typedef operations::member<value_type, typename Map::mapped_type, &value_type::second> value_accessor;
};

template <typename Map>
class Keys
   : public modified_container_impl< Keys<Map>,
                                     list( Operation< typename map_masquerade_helper<Map>::key_accessor >,
                                           Hidden< Map > ) >,
     public GenericSet< Keys<Map>, typename Map::key_type, typename Map::key_comparator_type> {
   typedef modified_container_impl<Keys> _super;
public:
   const typename Map::key_comparator_type& get_comparator() const
   {
      return this->hidden().get_comparator();
   }
   bool contains(typename function_argument<typename Map::key_type>::type x) const
   {
      return this->hidden().exists(x);
   }
};

template <typename Map>
class Values
   : public modified_container_impl< Values<Map>,
                                     list( Operation< typename map_masquerade_helper<Map>::value_accessor >,
                                           Hidden< Map > ) > { };

template <typename Map> inline
const Keys<Map>& keys(const Map& map)
{
   return reinterpret_cast<const Keys<Map>&>(map);
}

template <typename Map> inline
const Values<Map>& values(const Map& map)
{
   return reinterpret_cast<const Values<Map>&>(map);
}

namespace operations {

template <typename MapRef, typename Key>
class associative_access {
protected:
   typename attrib<MapRef>::minus_ref *map;
public:
   associative_access(typename attrib<MapRef>::minus_ref *map_arg=NULL) : map(map_arg) {}
   associative_access(const associative_access<typename attrib<MapRef>::minus_const, Key>& op) : map(op.map) {}

   typedef Key argument_type;
   typedef typename inherit_ref<typename deref<MapRef>::type::mapped_type, MapRef>::type result_type;

   result_type operator() (typename function_argument<Key>::type k) const
   {
      return (*map)[k];
   }

   template <typename,typename> friend class associative_access;
};

template <typename MapRef>
class associative_search {
protected:
   typename attrib<MapRef>::minus_ref *map;

public:
   associative_search(typename attrib<MapRef>::minus_ref *map_arg=NULL) : map(map_arg) {}
   associative_search(const associative_search<typename attrib<MapRef>::minus_const>& op) : map(op.map) {}

   typedef void argument_type;
   typedef bool result_type;

   template <typename IteratorPair, typename Operation>
   bool operator() (binary_transform_iterator<IteratorPair,Operation>& it,
                    typename enable_if<void**, check_iterator_feature<typename IteratorPair::first_type,end_sensitive>::value>::type=0) const
   {
      return ! (static_cast<typename IteratorPair::first_type&>(it)=map->find(*it.second)).at_end();
   }

   template <typename IteratorPair, typename Operation>
   bool operator() (binary_transform_iterator<IteratorPair,Operation>& it,
                    typename disable_if<void**, check_iterator_feature<typename IteratorPair::first_type,end_sensitive>::value>::type=0) const
   {
      return (static_cast<typename IteratorPair::first_type&>(it)=map->find(*it.second)) != map->end();
   }

   template <typename> friend class associative_search;
};

} // end namespace operations

template <typename MapRef, typename Key>
struct operation_cross_const_helper< operations::associative_access<MapRef,Key> > {
   typedef operations::associative_access<typename attrib<MapRef>::minus_const,Key> operation;
   typedef operations::associative_access<typename attrib<MapRef>::plus_const,Key> const_operation;
};

template <typename MapRef>
struct operation_cross_const_helper< operations::associative_search<MapRef> > {
   typedef operations::associative_search<typename attrib<MapRef>::minus_const> operation;
   typedef operations::associative_search<typename attrib<MapRef>::plus_const> const_operation;
};

template <typename Map, typename Keys,
          bool _keys_match=isomorphic_types<typename Map::key_type, Keys>::value>
struct assoc_helper {
   typedef typename Map::mapped_type& type;
   typedef const typename Map::mapped_type& const_type;

   static type doit(Map& map, const Keys& k)
   {
      return map.insert(k)->second;
   }

   static const_type doit(const Map& map, const Keys& k)
   {
      typename Map::const_iterator e=map.find(k);
      if (e.at_end()) throw no_match();
      return e->second;
   }
};

template <typename Map, size_t s>
struct assoc_helper<Map, char[s], true> {
   typedef typename Map::mapped_type& type;
   typedef const typename Map::mapped_type& const_type;

   static type doit(Map& map, const char (&k)[s])
   {
      return map.insert(typename Map::key_type(k))->second;
   }

   static const_type doit(const Map& map, const char (&k)[s])
   {
      typename Map::const_iterator e=map.find(typename Map::key_type(k));
      if (e.at_end()) throw no_match();
      return e->second;
   }
};

template <typename Map, typename Keys>
struct assoc_helper<Map, Keys, false> {
   typedef TransformedContainer<const Keys&, operations::associative_access<Map&,       typename Keys::value_type> > type;
   typedef TransformedContainer<const Keys&, operations::associative_access<const Map&, typename Keys::value_type> > const_type;

   static type doit(Map& map, const Keys& keys)
   {
      return type(keys,&map);
   }

   static const_type doit(const Map& map, const Keys& keys)
   {
      return const_type(keys,map);
   }
};

template <typename Map, typename Keys> inline
typename assoc_helper<Map, Keys>::type
select_by_keys(Map& map, const Keys& keys)
{
   return assoc_helper<Map, Keys>::doit(map,keys);
}

template <typename Map, typename Keys> inline
typename assoc_helper<Map, Keys>::const_type
select_by_keys(const Map& map, const Keys& keys)
{
   return assoc_helper<Map, Keys>::doit(map,keys);
}

} // end namespace pm

namespace polymake {
using pm::Keys;
using pm::Values;
using pm::select_by_keys;
using pm::DefaultValue;
using pm::no_match;
}

#endif // POLYMAKE_INTERNAL_ASSOC_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
