/* Copyright (c) 1997-2019
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

/** @file EquivalenceRelation.h
    @brief Implementation of polymake::EquivalenceRelation class
 */

#ifndef POLYMAKE_EQUIVALENCE_RELATION_H
#define POLYMAKE_EQUIVALENCE_RELATION_H

#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/GenericIO.h"

#include "polymake/list"
#include "polymake/hash_set"
#include "polymake/hash_map"

namespace pm {

/** @class EquivalenceRelation
    @brief An equivalence relation on the integers `0,..,n-1` for a given size `n`.

    Initially, each element is contained in an equivalence class by
    itself. Two equivalence classes can be merged by specifying one
    (arbitrary) representative of each class. Alternatively, a set of
    representatives may be specified and the corresponding equivalence classes
    will be merged.

    Each equivalence class is represented by its smallest element (default)
    or alternatively by a user defined element.  If classes with user defined
    representatives are merged, the smallest of the user defined
    representatives is chosen to represent the new equivalence class.
**/
class EquivalenceRelation {

protected:
   Array<int> equiv_classes;
   hash_set<int> set_rep; 
   Set<int> the_representatives;
   std::list<int> erased_rep;
   mutable bool dirty;

public:
   /// Creates the trivial equivalence relation on the empty set.
   EquivalenceRelation() { }

   /// Creates the equivalence relation on the integers 0,..,@a size-1 where
   /// each element is contained in an equivalence class by itself.
   explicit EquivalenceRelation(const int size)
      : equiv_classes(size,sequence(0,size).begin())
      , the_representatives(sequence(0,size))
      , dirty(false) { }

   /// Creates the equivalence relation on the integers 0,..,@a size-1 where
   /// each element is contained in an equivalence class by itself.  The @a
   /// representatives will be used as user defined representatives.
   template <typename Container>
   EquivalenceRelation(const int size, const Container& represent)
      : equiv_classes(size,sequence(0,size).begin())
      , the_representatives(sequence(0,size))
      , dirty(true)
   {
      for (auto v=entire(represent); !v.at_end(); ++v)
         set_rep.insert(*v);
   }

protected:
   template <typename Set>
   void add_class(const GenericSet<Set, int>& the_class)
   {
      if (the_class.top().empty()) return;
      const int highest=the_class.top().back();
      if (equiv_classes.size() <= highest)
         equiv_classes.resize(highest+1, -1);
      const int the_rep=the_class.top().front();
      the_representatives += the_rep;
      for (auto el=entire(the_class); !el.at_end(); ++el)
         equiv_classes[*el]=the_rep;
   }

public:
   void clear()
   {
      if (!equiv_classes.empty()) {
         equiv_classes.clear();
         set_rep.clear();
         the_representatives.clear();
         erased_rep.clear();
         dirty=false;
      }
   }

   /// Return the representative of the equivalence class containing @a e.
   int representative(int e) const
   {
      if (POLYMAKE_DEBUG) {
         if (e<0 || e>=equiv_classes.size())
            throw std::runtime_error("EquivalenceRelation::representative - element out of range");
      }
      if (e == equiv_classes[e])
         return e;

      int root = e;
      std::list<int> update_root;
      for ( ; equiv_classes[root]!=root; root=equiv_classes[root])
         update_root.push_back(root);

      while (!update_root.empty()) {
         const_cast<Array<int>&>(equiv_classes)[update_root.front()] = root;  // keeping the representation neat
         update_root.pop_front();
      }

      return root;
   }

   // ensures, that the pointer of each element points directly to its representative  
   void squeeze() const
   {
      for (int i=0; i<equiv_classes.size(); ++i)
         representative(i);
      dirty=false;
   }
   
   /// Return the representatives of the equivalence classes of all elements.
   const Array<int>& representatives() const
   {
      if (dirty) squeeze();
      return equiv_classes;
   }
   
   /// Merges two equivalence classes each represented by one (arbitrary) of its elements.
   void merge_classes(const int c1, const int c2)
   {
      int root1 = representative(c1);
      int root2 = representative(c2);
      if (root1 == root2) return;
      if (root1 > root2) std::swap(root1, root2);

      if (set_rep.find(root2)!=set_rep.end() && set_rep.find(root1)==set_rep.end()) {
         equiv_classes[root1] = root2;
         erased_rep.push_back(root1);
      } else {
         equiv_classes[root2] = root1;
         erased_rep.push_back(root2);
      }
      dirty=true;
   }
   
   /// Merges the classes of all the elements contained in classes.
   template <typename Container>
   void merge_classes(const Container& classes)
   {
      auto c=entire(classes);
      if (!c.at_end()) {
         const int c0=*c;
         for (++c; !c.at_end(); ++c)
            merge_classes(c0,*c);
      }
   }

   // lazy: erased representatives are collected and only erased when required
   void update_rep()
   {
      while (!erased_rep.empty()) {
         the_representatives -= erased_rep.front();
         erased_rep.pop_front();
      }
   }

   /// Return the set of all representatives.
   const Set<int>& set_of_rep() const
   {
      const_cast<EquivalenceRelation*>(this)->update_rep();
      return the_representatives;
   }

   /// Return the equivalence class of the element @a e.
   Set<int> equivalence_class(const int e) const
   {
      Set<int> class_of_e;
      const int rep = representative(e);

      for (int i=0; i<equiv_classes.size(); ++i)
         if (rep == representative(i))
            class_of_e += i;

      return class_of_e;
   }

   /// Return all equivalence classes.
   PowerSet<int> equivalence_classes() const
   {
      PowerSet<int> classes;
      hash_map< int,Set<int> > rep_map;

      for (int i=0, n=equiv_classes.size(); i<n; ++i)
         rep_map[representative(i)] += i;

      for (const auto& cl : rep_map)
         classes += cl.second;

      dirty=false;
      return classes;
   }

   /// Return true iff @a e1 and @a e2 are in the same equivalence class.
   bool related(const int e1, const int e2) const
   {
      return representative(e1)==representative(e2);
   }
   
   /// The element @a e will be used as user defined representative of it's equivalence class.
   void set_representative(const int e)
   {
      set_rep.insert(e);
      const int root = representative(e);

      if (root!=e) {  // adjust representatives
         equiv_classes[root] = e;
         equiv_classes[e] = e;
         update_rep();
         the_representatives -= root;
         the_representatives += e;
         dirty=true;
      }
   }

   template <typename Input>
   friend Input& operator>> (GenericInput<Input>& is, EquivalenceRelation& me)
   {
      PowerSet<int> classes;
      is.top() >> classes;
      me.clear();
      for (auto cl=entire(classes); !cl.at_end(); ++cl)
         me.add_class(*cl);
      return is.top();
   }
};

template <>
struct spec_object_traits< EquivalenceRelation >
   : spec_object_traits<is_container> {
   typedef PowerSet<int> serialized;

   static serialized serialize(const EquivalenceRelation& ER)
   {
      return ER.equivalence_classes();
   }
};

} // end namespace pm

namespace polymake {
   using pm::EquivalenceRelation;
}

#endif // POLYMAKE_EQUIVALENCE_RELATION_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
