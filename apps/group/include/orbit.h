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

#ifndef POLYMAKE_GROUP_ORBITS_H
#define POLYMAKE_GROUP_ORBITS_H


#include <polymake/Array.h>
#include <polymake/Set.h>
#include <polymake/hash_set>
#include <polymake/Matrix.h>
#include <polymake/group/action.h>
#include <queue>

namespace polymake {
namespace group {

/*
 * Computes the orbit of element, where the group is spanned by generators
 */
template<typename action_t, typename Perm, typename Element, typename Container=hash_set<Element>>
auto
orbit_impl(const Array<Perm>& generators,
           const Element& element)
{
   std::vector<action_t> g_actions; g_actions.reserve(generators.size());
   for (const auto& g: generators)
      g_actions.push_back(action_t(g));

   Container orbit;
   orbit.insert(element);
   std::queue<Element> q;
   q.push(element);
   while (!q.empty()) {
      const Element orbitElement = q.front();
      q.pop();
      for (const auto& a: g_actions) {
         const Element next = a(orbitElement);
         if (!orbit.collect(next)) {
            q.push(next);
         }
      }
   }
   return orbit;
}

template<typename action_type, typename Perm, typename Element, typename Container=hash_set<Element>,
         typename op_tag=typename pm::object_traits<Element>::generic_tag, 
         typename perm_tag=typename pm::object_traits<Perm>::generic_tag,
         typename stores_ref=std::true_type>
auto
unordered_orbit(const Array<Perm>& generators, 
      const Element& element) 
{
   typedef pm::operations::group::action<Element&, action_type, Perm, op_tag, perm_tag, stores_ref> action_t;
   return orbit_impl<action_t, Perm, Element, Container>(generators, element);
}
   
template<typename action_type, typename Perm, typename Element, typename Container=hash_set<Element>,
         typename op_tag=typename pm::object_traits<Element>::generic_tag, 
         typename perm_tag=typename pm::object_traits<Perm>::generic_tag,
         typename stores_ref=std::true_type>
auto
orbit(const Array<Perm>& generators, 
      const Element& element)
// ordered, for canonical representation   
{
   typedef pm::operations::group::action<Element&, action_type, Perm, op_tag, perm_tag, stores_ref> action_t;
   return Set<Element>(entire(orbit_impl<action_t, Perm, Element, Container>(generators, element)));
}

   
namespace {

inline
int next_not_in_set(const Set<int>& the_set,
		    int initial_value)
{
   if (!the_set.size() || initial_value >= *(the_set.rbegin())) return initial_value+1;
   while(the_set.contains(++initial_value));
   return initial_value;
}

}

/// Calculates a set of orbit representatives for a permutation action
template<typename GeneratorType>
Array<int>
orbit_representatives(const Array<GeneratorType>& generators) 
{
   const int degree = generators[0].size();
   Set<int> seen_elements;
   std::vector<int> reps;
   int rep(0);
   while (rep<degree) {
      reps.push_back(rep);
      seen_elements += orbit<on_elements, GeneratorType, int, Set<int>>(generators, rep);
      rep = next_not_in_set(seen_elements, rep);
   }
   return Array<int>{reps};
}

}
}

#endif

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
