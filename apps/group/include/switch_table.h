/* Copyright (c) 1997-2020
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

#include "polymake/client.h"
#include "polymake/Map.h"
#include "polymake/group/orbit.h"
#include "polymake/linalg.h"
#include "polymake/Set.h"
#include <stack>
#include <sstream>


// These declarations are necessary, so we friend this class/struct later for
// accessing private members in serialization.
namespace polymake { namespace group {
class SwitchTable;
}}
namespace pm {
   template<> struct spec_object_traits< pm::Serialized< polymake::group::SwitchTable > >;
}


namespace polymake { namespace group {

namespace switchtable {
      
inline bool fixes(Int i, const Array<Int>& g)
{
   return g[i] == i;
}

// Check whether a permutation (non-)fixes entry n(=i).
struct non_fixed {
   Int i;
   non_fixed(Int n): i(n){}
   bool operator() (const Array<Int>& g) { return !fixes(i, g); }
};

class Core {
public:
private:
   Array<Int> identity;
   Map<Int, Map<Int, Array<Int>>> switch_table;
   Map<Int, pm::Set<Int>> supports;

   friend class polymake::group::SwitchTable;
   friend struct pm::spec_object_traits< pm::Serialized< polymake::group::SwitchTable > >;

   void extract_supports()
   {
      identity = switch_table[0][0];
      for (Int j = 0; j < switch_table.size(); ++j) {
         supports[j] = pm::Set<Int>();
         for (const auto g:switch_table[j]){
            supports[j] += g.first;
         }
      }
   }

   Int nFixedPts(const Array<Int>& g)
   {
      Int result = 0;
      while (fixes(result, g)){
         ++result;
      }
      return result;
   }

   void extract_switches(const Array<Array<Int>>& all)
   {
      Map<Int, std::list<Array<Int>>> fixed_pt_filter;
      identity = Array<Int>(all[0].size());
      for (Int j = 0; j < identity.size(); ++j) {
         identity[j] = j;
      }
      for (const auto& elem : all) {
         if (elem != identity) {
            fixed_pt_filter[0].push_back(elem);
         }
      }
      Int i = 0;
      // Filter by fixed pts
      while (fixed_pt_filter[i].size() != 0) {
         fixed_pt_filter[i+1] = fixed_pt_filter[i];
         fixed_pt_filter[i+1].remove_if(non_fixed(i));
         ++i;
      }
      Int bound = i-1;
      // Populate actual switch table
      for (Int j = 0; j <= bound; ++j) {
         switch_table[j] = Map<Int, Array<Int>>();
         switch_table[j][j] = identity;
         for (const auto& g : fixed_pt_filter[j]) {
            i = j;
            while (g[i] != j) {
               ++i;
            }
            if (i != j) {
               if (!switch_table[j].exists(i)){
                  switch_table[j][i] = g;
               }
            }
         }
      }
   }

public:
   Core(const Array<Array<Int>>& all)
   {
      extract_switches(all);
      extract_supports();
   }

   Core() {}

   std::string to_string() const
   {
      std::ostringstream bos;
      wrap(bos) << "  Supports: (size, content)"<<endl;
      for (Int i = 0; i < switch_table.size(); ++i) {
         wrap(bos) << "Level " << i << ": " << switch_table[i].size() << " " << supports[i] << endl;
      }
      wrap(bos) << "  Entries:" << endl;
      for (Int i = 0; i < switch_table.size(); ++i) {
         for (Int j : supports[i]) {
            wrap(bos) << "[" << i << "," << j << "]: " << switch_table[i][j] << endl;
         }
      }
      return bos.str();
   }

   void extract_switches(Int fixed, const pm::Set<Int>& desired, std::list<const Array<Int>*>& switches) const
   {
      for (const auto& j : desired) {
         switches.push_back(&(switch_table[fixed][j]));
      }
   }

   Int get_bound() const
   {
      return switch_table.size();
   }

   const Array<Int>& get_identity() const
   {
      return identity;
   }

   bool support_exists(Int fixed) const
   {
      return supports.exists(fixed);
   }

   const pm::Set<Int>& get_support(Int fixed) const
   {
      return supports[fixed];
   }
};


template<typename ActedOn>
class Optimizer {
private:
   const Core& core;
   ActedOn currentOptimal;
   Array<Int> optimalSwitch;

   // Check whether there are any switches that potentially improve a
   // vector. If there are any elements, they are stored in the list
   // 'switches'.
   // First we check whether there are any switches at level 'fixed' at all.
   // Then we ask the element v, which switches would be convenient for it.
   void find_next_switches(Int fixed, std::list<const Array<Int>*>& switches, const ActedOn& v)
   {
      if (core.support_exists(fixed)) {
         const pm::Set<Int> goodSupport = v.get_support(core.get_support(fixed));
         core.extract_switches(fixed, goodSupport, switches);
      }
   }

   void update_optimal(const ActedOn& ao, const Array<Int>& currentSwitch)
   {
      if (ao > currentOptimal) {
         currentOptimal = ao;
         optimalSwitch = Array<Int>(currentSwitch);
      }
   }

public:
   Optimizer(const Core& c, const ActedOn& ao_in)
      : core(c)
      , currentOptimal(ao_in)
      , optimalSwitch(core.get_identity()) {}
      
   void optimize()
   {
      // The following is a depth first search with three different stacks.
      // At every level we have certain switches we can apply that improve
      // the element we act on. We only update the optimal at the leaves.
      Int fixed = 0;
      std::stack<std::list<const Array<Int>*>> switchStack;
      std::stack<std::list<const Array<Int>*>::const_iterator> iteratorStack;
      std::stack<Array<Int>> currentSwitch;
      currentSwitch.push(core.get_identity());
      std::stack<ActedOn> vStack;
      vStack.push(ActedOn(currentOptimal));
      while (vStack.size()>0){
         if (fixed >= core.get_bound()){
            // Is leaf
            update_optimal(vStack.top(), currentSwitch.top());
            vStack.pop();
            currentSwitch.pop();
            --fixed;
            //std::cout << "Found leaf." << std::endl;
         } else {
            // Are the switches already computed?
            if (vStack.size() > switchStack.size()) {
               std::list<const Array<Int>*> switches;
               find_next_switches(fixed, switches, vStack.top());
               switchStack.push(std::move(switches));
               iteratorStack.push(switchStack.top().begin());
            }
            // Are we at the end?
            if (iteratorStack.top() == switchStack.top().end()) {
               // std::cout << "At end." << std::endl;
               iteratorStack.pop();
               switchStack.pop();
               vStack.pop();
               --fixed;
               currentSwitch.pop();
            } else {
               // Descend
               const Array<Int>* g = *(iteratorStack.top());
               // std::cout << "Applying " << g << std::endl;
               vStack.push(std::move(vStack.top().mutate(g)));
               currentSwitch.push(action<on_container>(currentSwitch.top(),*g));
               ++iteratorStack.top();
               ++fixed;
            }
         }
      }
   }

   std::pair<const ActedOn&, const Array<Int>&> get_optimal() const
   {
      return { currentOptimal, optimalSwitch };
   }
};

template<typename Scalar>
class PackagedVector {
private:
   Vector<Scalar> v;
   using SupportsMapType = Map<Scalar, Set<Int>>;
   SupportsMapType supports;

   PackagedVector(const Vector<Scalar>& w, const Map<Scalar, Set<Int>>& s)
      : v(w)
      , supports(s){}

public:

   PackagedVector(const Vector<Scalar>& w)
      : v(w)
   {
      for (Int i = 0; i < v.dim(); ++i) {
         supports[v[i]] += i;
      }
   }

   // We are given the support at a certain level of the switch table. Now
   // we run through the levels of our vector to find one that intersects
   // the support non-trivially. This will always terminate, since the
   // identity always satisfies the condition imposed and the identity is
   // contained in the set 'in'.
   Set<Int> get_support(const Set<Int>& in) const
   {
      auto desired = supports.begin();
      while ((desired->second * in).empty()) {
         ++desired;
      }
      return desired->second*in;
   }

   const Vector<Scalar>& inner() const
   {
      return v;
   }
      
   const Map<Scalar, Set<Int>>& get_supports() const
   {
      return supports;
   }

   bool operator> (const PackagedVector& other) const
   {
      return -1 == lex_compare(v, other.v);
   }

   PackagedVector mutate(const Array<Int>* g) const
   {
      Vector<Scalar> newV(action_inv<on_container>(*g, v));
      return PackagedVector(newV);
   }
};

} // end namespace switchtable


class SwitchTable {
private:
   switchtable::Core core;
   friend struct pm::spec_object_traits< pm::Serialized<SwitchTable> >;

public:
   SwitchTable(const Array<Array<Int>>& all)
      : core(all) {}

   SwitchTable() {}

   template<typename Scalar>
   std::pair<Vector<Scalar>, Array<Int>> lex_minimize_vector(const Vector<Scalar>& v) const
   {
      switchtable::PackagedVector<Scalar> pv(v);
      switchtable::Optimizer<switchtable::PackagedVector<Scalar>> sto(core, pv);
      sto.optimize();
      auto result = sto.get_optimal();
      return { result.first.inner(), result.second };
   }

   template<typename Scalar>
   std::pair<Vector<Scalar>, Array<Int>> lex_maximize_vector(const Vector<Scalar>& v) const
   {
      auto result = lex_minimize_vector<Scalar>(-v);
      return { -result.first, result.second };
   }

   bool operator==(const SwitchTable& other) const
   {
      return core.switch_table == other.core.switch_table;
   }

   template <typename Output> friend
   Output& operator<< (GenericOutput<Output>& out, const SwitchTable& me)
   {
      out.top() << me.core.to_string();
      return out.top();
   }
};

} // end namespace group
} // end namespace polymake

namespace pm {

template <>
struct spec_object_traits< Serialized<polymake::group::SwitchTable> >
   : spec_object_traits<is_composite> {

   typedef polymake::group::SwitchTable masquerade_for;

   typedef Map<Int, Map<Int, Array<Int>>> elements;

   template <typename Me, typename Visitor>
   static void visit_elements(Me& me, Visitor& v) //for data_load
   {
      v << me.core.switch_table;
      me.core.extract_supports();
   }

   template <typename Visitor>
   static void visit_elements(const Serialized<masquerade_for>& me, Visitor& v) //for data_save
   {
      v << me.core.switch_table;
   }
};

}

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
