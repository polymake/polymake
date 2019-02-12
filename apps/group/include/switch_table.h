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
      
inline bool fixes(int i, const Array<int>& g){
   return g[i] == i;
}

// Check whether a permutation (non-)fixes entry n(=i).
struct non_fixed {
   int i;
   non_fixed(int n): i(n){}
   bool operator() (const Array<int>& g) { return !fixes(i, g); }
};

class Core {
public:
   private:
      Array<int> identity;
      Map<int, Map<int, Array<int>>> switch_table;
      Map<int, pm::Set<int>> supports;

      friend class polymake::group::SwitchTable;
      friend struct pm::spec_object_traits< pm::Serialized< polymake::group::SwitchTable > >;

      void extract_supports() {
         identity = switch_table[0][0];
         for(int j=0; j<switch_table.size(); j++){
            supports[j] = pm::Set<int>();
            for(const auto g:switch_table[j]){
               supports[j] += g.first;
            }
         }
      }

      int nFixedPts(const Array<int>& g){
         int result = 0;
         while(fixes(result, g)){
            result++;
         }
         return result;
      }

      void extract_switches(const Array<Array<int>>& all){
         Map<int, std::list<Array<int>> > fixed_pt_filter;
         identity = Array<int>(all[0].size());
         for(int j=0; j<identity.size(); j++){
            identity[j] = j;
         }
         for(const auto& elem : all){
            if(elem != identity){
               fixed_pt_filter[0].push_back(elem);
            }
         }
         int i = 0;
         // Filter by fixed pts
         while(fixed_pt_filter[i].size() != 0){
            fixed_pt_filter[i+1] = fixed_pt_filter[i];
            fixed_pt_filter[i+1].remove_if(non_fixed(i));
            i++;
         }
         int bound = i-1;
         // Populate actual switch table
         for(int j=0; j<=bound; j++){
            switch_table[j] = Map<int, Array<int>>();
            switch_table[j][j] = identity;
            for(const auto& g : fixed_pt_filter[j]){
               i = j;
               while(g[i] != j){
                  i++;
               }
               if(i != j){
                  if(!switch_table[j].exists(i)){
                     (switch_table[j])[i] = g;
                  }
               }
            }
         }
      }

   public:
      Core(const Array<Array<int>>& all) {
         extract_switches(all);
         extract_supports();
      }

      Core() {}

      std::string to_string() const {
         std::ostringstream bos;
         wrap(bos) << "  Supports: (size, content)"<<endl;
         for(int i=0; i<switch_table.size(); i++){
            wrap(bos) << "Level " << i << ": " << switch_table[i].size() << " " << supports[i] << endl;
         }
         wrap(bos) << "  Entries:" << endl;
         for(int i=0; i<switch_table.size(); i++){
            for(int j:supports[i]){
               wrap(bos) << "[" << i << "," << j << "]: " << switch_table[i][j] << endl;
            }
         }
         return bos.str();
      }
      
      void extract_switches(int fixed, const pm::Set<int>& desired, std::list<const Array<int>*>& switches) const {
         for(const auto& j : desired){
            switches.push_back(&(switch_table[fixed][j]));
         }
      }

      int get_bound() const {
         return switch_table.size();
      }

      const Array<int>& get_identity() const {
         return identity;
      }

      bool support_exists(int fixed) const {
         return supports.exists(fixed);
      }

      const pm::Set<int>& get_support(int fixed) const {
         return supports[fixed];
      }

};


template<typename ActedOn>
class Optimizer {
   private:
      const Core& core;
      ActedOn currentOptimal;
      Array<int> optimalSwitch;

      // Check whether there are any switches that potentially improve a
      // vector. If there are any elements, they are stored in the list
      // 'switches'.
      // First we check whether there are any switches at level 'fixed' at all.
      // Then we ask the element v, which switches would be convenient for it.
      void find_next_switches(int fixed, std::list<const Array<int>*>& switches, const ActedOn& v){
         if(core.support_exists(fixed)){
            const pm::Set<int> goodSupport = v.get_support(core.get_support(fixed));
            core.extract_switches(fixed, goodSupport, switches);
         }
      }

      void update_optimal(const ActedOn& ao, const Array<int>& currentSwitch){
         if(ao > currentOptimal){
            currentOptimal = ao;
            optimalSwitch = Array<int>(currentSwitch);
         }
      }

   public:
      Optimizer(const Core& c, const ActedOn& ao_in):
         core(c), currentOptimal(ao_in), optimalSwitch(core.get_identity()) {}
      
      void optimize(){
         // The following is a depth first search with three different stacks.
         // At every level we have certain switches we can apply that improve
         // the element we act on. We only update the optimal at the leaves.
         int fixed = 0;
         std::stack<std::list<const Array<int>*>> switchStack;
         std::stack<std::list<const Array<int>*>::const_iterator> iteratorStack;
         std::stack<Array<int>> currentSwitch;
         currentSwitch.push(core.get_identity());
         std::stack<ActedOn> vStack;
         vStack.push(ActedOn(currentOptimal));
         while(vStack.size()>0){
            if(fixed >= core.get_bound()){
               // Is leaf
               update_optimal(vStack.top(), currentSwitch.top());
               vStack.pop();
               currentSwitch.pop();
               fixed--;
               //std::cout << "Found leaf." << std::endl;
            } else {
               // Are the switches already computed?
               if(vStack.size() > switchStack.size()){
                  std::list<const Array<int>*> switches;
                  find_next_switches(fixed, switches, vStack.top());
                  switchStack.push(std::move(switches));
                  iteratorStack.push(switchStack.top().begin());
               }
               // Are we at the end?
               if(iteratorStack.top() == switchStack.top().end()){
                  // std::cout << "At end." << std::endl;
                  iteratorStack.pop();
                  switchStack.pop();
                  vStack.pop();
                  fixed--;
                  currentSwitch.pop();
               } else {
                  // Descent
                  const Array<int>* g = *(iteratorStack.top());
                  // std::cout << "Applying " << g << std::endl;
                  vStack.push(std::move(vStack.top().mutate(g)));
                  currentSwitch.push(action<on_container>(currentSwitch.top(),*g));
                  ++iteratorStack.top();
                  fixed++;
               }
            }
         }
      }

      std::pair<const ActedOn&, const Array<int>&> get_optimal() const {
         return std::pair<const ActedOn&, const Array<int>&>(currentOptimal, optimalSwitch);
      }
};

template<typename Scalar>
class PackagedVector {
   
   private:
      Vector<Scalar> v;
      using SupportsMapType = Map<Scalar, Set<int>>;
      SupportsMapType supports;
      
      PackagedVector(const Vector<Scalar>& w, const Map<Scalar, Set<int>>& s): v(w), supports(s){}

   public:

      PackagedVector(const Vector<Scalar>& w): v(w){
         for(int i=0; i<v.dim(); i++){
            supports[v[i]] += i;
         }
      }

      // We are given the support at a certain level of the switch table. Now
      // we run through the levels of our vector to find one that intersects
      // the support non-trivially. This will always terminate, since the
      // identity always satisfies the condition imposed and the identity is
      // contained in the set 'in'.
      const pm::Set<int> get_support(const pm::Set<int>& in) const {
         auto desired = supports.begin();
         while((desired->second * in).empty()){
            ++desired;
         }
         return desired->second*in;
      }

      const Vector<Scalar>& inner() const {
         return v;
      }
      

      const Map<Scalar, Set<int>>& get_supports() const {
         return supports;
      }

      bool operator >(const PackagedVector& other) const {
         return -1 == lex_compare(v, other.v);
      }

      PackagedVector mutate(const Array<int>* g) const {
         Vector<Scalar> newV(action_inv<on_container>(*g, v));
         return PackagedVector(newV);
      }
};

} // end namespace switchtable


class SwitchTable {
   private:
      switchtable::Core core;
      friend struct pm::spec_object_traits< pm::Serialized< polymake::group::SwitchTable > >;

   public:
      SwitchTable(const Array<Array<int>>& all) : core(all) {}

      SwitchTable() {}

      template<typename Scalar>
         std::pair<Vector<Scalar>, Array<int>> lex_minimize_vector(const Vector<Scalar>& v) const {
            switchtable::PackagedVector<Scalar> pv(v);
            switchtable::Optimizer<switchtable::PackagedVector<Scalar>> sto(core, pv);
            sto.optimize();
            auto result = sto.get_optimal();
            return std::pair<Vector<Scalar>, Array<int>>(result.first.inner(), result.second);
         }

      template<typename Scalar>
         std::pair<Vector<Scalar>, Array<int>> lex_maximize_vector(const Vector<Scalar>& v) const {
            auto result = lex_minimize_vector<Scalar>(-v);
            return std::pair<Vector<Scalar>, Array<int>>(-result.first, result.second);
         }

      bool operator==(const SwitchTable& other) const {
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

namespace pm{
   template<>
      struct spec_object_traits< Serialized< polymake::group::SwitchTable > > :
      spec_object_traits<is_composite> {

         typedef polymake::group::SwitchTable masquerade_for;

         typedef Map<int, Map<int, Array<int>>> elements;

         template <typename Me, typename Visitor>
            static void visit_elements(Me& me, Visitor& v) //for data_load
            {
               v << me.core.switch_table;
               me.core.extract_supports();
            }

         template <typename Visitor>
            static void visit_elements(const pm::Serialized<masquerade_for>& me, Visitor& v) //for data_save
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

