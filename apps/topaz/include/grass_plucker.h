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

#ifndef __POLYMAKE_GRASS_PLUCKER_H
#define __POLYMAKE_GRASS_PLUCKER_H

#include <vector>

namespace pm {

// this is to be able to make hash_set<std::vector<Sush>>   
template<typename SizeT>   
void
hash_combine(SizeT& h, long int k)   
{
   const std::size_t c1 = 0xcc9e2d51;
   const std::size_t c2 = 0x1b873593;

   if (k<0)
      k = -k;
   k *= c1;
   k = (k << 15) | (k >> (32-15));
   k *= c2;

   h ^= k;
   h = (h << 13) | (h >> (32-13));
   h = h*5+0xe6546b64;
}
   
} // end namespace pm

#include <map>
#include <unordered_set>

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Map.h"
#include "polymake/PowerSet.h"
#include "polymake/hash_map"
#include "polymake/hash_set"

namespace polymake { namespace topaz {

namespace gp {
   
// this is based on
// https://www.fluentcpp.com/2016/12/08/strong-types-for-strong-interfaces/   
template <typename T, typename tag>
class NamedType {
public:

   using base_type = T;
   using value_type = T;
   
   explicit NamedType() {}

   explicit NamedType(T const& value) : value_(value) {}

   template<typename T_ = T>
   explicit NamedType(T&& value,
                      typename std::enable_if<!std::is_reference<T_>{},
                      std::nullptr_t>::type = nullptr)
      : value_(std::move(value)) {}

   template<typename T_ = T>
   std::enable_if_t<!std::is_integral<T_>::value, Int>
   size() const {
      return value_.size();
   }
   
   T& get() { return value_; }
   T const& get() const { return value_; }

   operator T const&() const { return value_; }
   operator T&() { return value_; }

   friend bool operator==(const NamedType<T, tag>& a,
                          const NamedType<T, tag>& b) {
      return a.get() == b.get();
   }

   template<typename T_ = T>
   std::enable_if_t<std::is_integral<T_>::value, NamedType<T,tag>>
   operator-(const NamedType<T,tag>& x) {
      return NamedType<T,tag>(-x.value_);
   }

   template<typename Output>
   friend
   Output& operator<< (GenericOutput<Output>& outs, const NamedType<T, tag>& x) {
      return outs.top() << x.value_;
   }
   
protected:
   T value_;
};

struct TreeIndexTag {};
using TreeIndex = NamedType<Int, TreeIndexTag>;

struct CubeIndexTag {};
using CubeIndex = NamedType<Int, CubeIndexTag>;
   
/*
  From c++17 on, the following defines can be
  inline constexpr Int max_n_vertices(sizeof(Int)*4-1); 
  inline constexpr Int first_cube_index(Int(1) << (2*max_n_vertices));
*/
#define max_n_vertices ((Int) sizeof(Int)*4-1)
#define first_cube_index (CubeIndex(Int(1) << (2*max_n_vertices)))
#define MAGIC_VERTEX_MULTIPLE 256
   
      
struct PhiOrCubeIndexTag {};   
class PhiOrCubeIndex : public NamedType<Int, PhiOrCubeIndexTag> {
public:
   explicit PhiOrCubeIndex() {}

   explicit PhiOrCubeIndex(Int const& value) : NamedType<Int, PhiOrCubeIndexTag>(value) {}
   
   bool is_cube_index() const { return CubeIndex(value_) >= first_cube_index; }

   CubeIndex cube_index() const {
      if (!is_cube_index())
         throw std::runtime_error("PhiOrCubeIndex: not a cube index");
      return CubeIndex(value_ - first_cube_index);
   }
};
   
   
} } }


namespace pm {

template<typename T, typename tag>
struct hash_func<polymake::topaz::gp::NamedType<T, tag>, pm::is_opaque> {
   Int operator()(polymake::topaz::gp::NamedType<T, tag> const& x) const {
      return hash_func<T>()(x);
   }
};

template<>
struct hash_func<polymake::topaz::gp::PhiOrCubeIndex, pm::is_opaque> {
   Int operator()(polymake::topaz::gp::PhiOrCubeIndex const& x) const {
      return hash_func<Int>()(x.get());
   }
};
   
} // end namespace pm

namespace std {

template<typename T, typename tag>   
struct hash<polymake::topaz::gp::NamedType<T, tag>> {
   size_t operator()(const polymake::topaz::gp::NamedType<T, tag>& nt) const {
      return std::hash<T>()(nt.get());
   }
};

template<typename T, typename tag>   
struct hash<std::vector<polymake::topaz::gp::NamedType<T, tag>>> {
   size_t operator()(const std::vector<polymake::topaz::gp::NamedType<T, tag>>& v) const {
      const size_t prime = 257;
      auto it = v.begin();
      size_t h { hash<T>()(it->get()) };
      ++it;
      while (it != v.end()) {
         h = h * prime + hash<T>()(it->get());
         ++it;
      }
      return h;
   }
};

   
} // end namespace std


namespace polymake { namespace topaz {
namespace gp {   

struct SignImplTag {};
using SignImpl = NamedType<Int, SignImplTag>;

// PluckerHashIndex = Phi   
struct PhiTag {};
using Phi = NamedType<Int, PhiTag>;

// SignedUndeterminedSolidHash = Sush
struct SushTag {};
using Sush = NamedType<Int, SushTag>;
   
struct FacetAsSetTag {};
using FacetAsSet = NamedType<Set<Int>, FacetAsSetTag>;
using IndexOfFacet = hash_map<FacetAsSet, Int>;
   
struct LabelData {
   Array<std::string> vertex_labels;
   std::size_t max_label_width;
};

using ExplicitGroup = Array<Array<Int>>;   
using PhiOrbit = hash_set<Phi>;   
   
struct SphereData {
   Array<FacetAsSet> facets;
   IndexOfFacet iof;
   Array<Int> orientation;
   LabelData label_data;
   ExplicitGroup vertex_symmetry_group;
   PhiOrbit seen_phis;
   Int n;
   Int d;
};

struct PluckerStats {
   Int total_processed = 0;
   Int n_duplicates = 0;
   Int n_two_adjacent_undetermineds = 0;
   Int n_determined_sign_negative = 0;
};

/*
  The following class stores a pair of sets I,J and a sign
  in an Int. 

  If max_n_vertices needs to be bigger that 31 at some point,
  replace this with a proper bitset.
 */
// assert((max_n_vertices) <= 31);

class PluckerHasher {
   Phi store;

public:
   PluckerHasher(const Phi _store)
      : store(_store)
   {}
   
   PluckerHasher(const Set<Int>& I,
                 const Set<Int>& J,
                 const SignImpl sign)
      : store(0)
   {
      assert(I.size());
      assert(J.size() == I.size() + 2);
      assert(I.back() < max_n_vertices);
      assert(J.back() < max_n_vertices);
      for (const Int i: I) 
         store |= (Int(1) << (i + max_n_vertices));

      for (const Int j: J) 
         store |= (Int(1) << j);

      if (sign < 0)
         store.get() = -store;
   }

   const Phi phi() const { return store; }

   void invert() { store.get() = -store; }

private:
   const Set<Int> constituent_set(const Int offset) const {
      if (store > first_cube_index)
         return Set<Int>();
      
      const Int abs_store(abs(store));
      Set<Int> I;
      for (Int i=0; i<max_n_vertices - 1; ++i)
         if (abs_store & (Int(1) << (i + offset)))
            I += i;
      return I;
   }
   
public:
   
   const Set<Int> I() const {
      return constituent_set(max_n_vertices);
   }

   const Set<Int> J() const {
      return constituent_set(0);
   }
};

struct SolidSetTag {};
using SolidSet = NamedType<Set<Int>, SolidSetTag>; 

struct CanonicalSolidTag {};
using CanonicalSolidArray = NamedType<Array<Int>, CanonicalSolidTag>; 
   
class UndeterminedSolidHasher {
   Sush store_;

public:
   UndeterminedSolidHasher(const Sush store)
      : store_(store)
   {}

   UndeterminedSolidHasher(const CanonicalSolidArray& solid,
                           const Int sign)
      : store_(0)
   {
      for (const Int i: solid.get()) {
         assert (i < max_n_vertices);
         store_ |= (Int(1) << i);
      }
      if (sign < 0)
         store_.get() = -store_;
   }

   const Sush hash() const { return store_; };

   const SolidSet solid() const {
      const Int abs_store(abs(store_.get()));
      SolidSet solid;
      for (Int i=0; i<max_n_vertices - 1; ++i)
         if (abs_store & (Int(1) << i))
            solid.get() += i;
      return solid;
   }
};
   

SignImpl
sgn(const Int j,
    const Set<Int>& I,
    const Set<Int>& J);

enum class SignDeterminedStatus {
   determined,
   undetermined
};
   
class MaybeUndeterminedSign {
   SignImpl _sign;
   SignDeterminedStatus _determined;
   
public:
   MaybeUndeterminedSign()
      : _sign(0)
      , _determined(SignDeterminedStatus::undetermined)
   {}


   MaybeUndeterminedSign(const SignImpl& sign_,
                         const SignDeterminedStatus& determined_)
      : _sign(sign_)
      , _determined(determined_)
   {}

   SignImpl sign_or_signature() const {
      return _sign;
   }

   bool determined() const {
      return _determined == SignDeterminedStatus::determined;
   }

   SignDeterminedStatus determined_status() const {
      return _determined;
   }
   
   void invert() {
      _sign.get() = -_sign;
   }
   
   MaybeUndeterminedSign operator*=(const SignImpl& s) {
      _sign *= s;
      return *this;
   }
};

MaybeUndeterminedSign operator*(const MaybeUndeterminedSign& s, const MaybeUndeterminedSign& t);
MaybeUndeterminedSign operator*(const MaybeUndeterminedSign& s, const SignImpl& t);
MaybeUndeterminedSign operator-(const MaybeUndeterminedSign& s);
bool operator==(const MaybeUndeterminedSign& s, const MaybeUndeterminedSign& t);
bool operator!=(const MaybeUndeterminedSign& s, const MaybeUndeterminedSign& t);

   
class PluckerRel;

struct PluckerData {
   std::vector<PluckerRel> plucker_rel_list;
   hash_set<Phi> seen_pluckers;
   PluckerStats stats;
};

struct IntParams {
   Int verbosity = 0;
   Int max_length_ct = 0;
   Int initial_undetermined_ct = 0;
   Int max_undetermined_ct = 0;
   Int abort_after = 0;
   Int use_cubes = 0;
   Int use_fixed_plucker_source = 0;
   Int cube_log_interval = 0;
   Int tree_log_interval = 0;
   Int debug = 0;
   Int output_ct = 0;
   Int iter_ct = 0;
};


enum class SolutionStatus {
   not_found,
   found_single_positive_pr,
   found_tree,
   found_cube,
   unsuccessful
};

  
   
using TreesWithSush = std::map<Sush, std::vector<TreeIndex>>;
using SushVector = std::vector<Sush>;
   
class GP_Tree;
class GP_Cube;   
   
struct SearchData {
   // a string to identify the currently running instance
   std::string id_string;
   
   // all trees
   std::vector<GP_Tree> tree_list;

   // which trees have exactly one undetermined solid, namely sush
   hash_map<Sush, TreeIndex> leaf_of_sush;

   // which trees with >= 2 undetermined solids have one of them being sush
   hash_map<Sush, std::vector<TreeIndex>> nonleaf_trees_of_sush;

   // which trees have we already seen
   hash_set<SushVector> seen_tree_sushes;

   TreeIndex next_tree_index = TreeIndex(0);

   /*
   // the cubes we found
   std::vector<GP_Cube> cube_list;

   CubeIndex next_cube_index = first_cube_index;
   */
};

   
// --------------------------------------------   
// Memoizers: base class, first instances
// --------------------------------------------   
   
template<typename CRTP, typename Key, typename Data>   
class Memoizer {
protected:
   struct Compare {
      bool operator()(const typename Key::base_type& lhs,
                      const typename Key::base_type& rhs) const {
         return operations::cmp()(lhs, rhs) < 0;
      }
   };
   
   hash_map<typename Key::base_type, Data> storage_;

   const Data& make_entry(const Key& key) {
      return storage_.insert({ key.get(),
                               static_cast<CRTP*>(this)->data_from(key) }).first->second;
   }

public:
   const Data&
   operator[](const Key& key) {
      const auto it = storage_.find(key);
      return (it != storage_.end())
         ? it->second
         : make_entry(key);
   }

   const Data&
   operator[](const Key& key) const {
      const auto it = storage_.find(key);
      assert (it != storage_.end());
      return it->second;
   }

   Int size() const { return storage_.size(); }

   const auto& storage() const { return storage_; }
};

// --------------------------------------------   
// CanonicalSolidRep
// --------------------------------------------   
   
      
class PermutationSignMemoizer   
   : public Memoizer<PermutationSignMemoizer, CanonicalSolidArray, SignImpl> {

public:
   const SignImpl
   data_from(const CanonicalSolidArray& p) {
      Int c=0;
      for (Int k=0; k<p.size()-1; ++k)
         for (Int l=k+1; l<p.size(); ++l)
            if (p.get()[k] > p.get()[l])
               ++c;
      return c % 2
         ? SignImpl(-1)
         : SignImpl(1);
   }
};


class CanonicalSolidRep {
   CanonicalSolidArray csa_;
   MaybeUndeterminedSign sign_;

public:

   CanonicalSolidRep(const CanonicalSolidArray& csa)
      : csa_(csa)
      , sign_(SignImpl(1), SignDeterminedStatus::undetermined)
   {}

   CanonicalSolidRep(const SolidSet& s,
                     const SphereData& sd,
                     PermutationSignMemoizer& psm)
      : csa_(Array<Int>(s.size(), entire(s.get())))
   {
      const Int d(s.size() - 1); // the solid has d+1 elements
      assert(d >= 2);
      FacetAsSet F(s.get());
      CanonicalSolidArray original_array = csa_;
      
      for (auto cit = entire(csa_.get()); !cit.at_end(); ++cit) {
         // By leaving out the element *cit, do we get a facet?
         // If not, continue to search
         F.get() -= *cit;
         if (!sd.iof.exists(F)) {
            F.get() += *cit;
            continue;
         }
         // We only get here if csa_ contains a facet

         // point to the last-but-one element
         auto eit = csa_.get().end();
         --eit;
         
         // swap the vertex that is not in the facet to the end
         std::swap(*cit, *eit);

         // order the facet
         std::sort(csa_.get().begin(), eit);
         
         // adjust for orientation by maybe interchanging the last
         // elements of the facet
         if (-1 == sd.orientation[sd.iof[F]])
            std::swap(csa_.get()[d-2], csa_.get()[d-1]);

         sign_ = MaybeUndeterminedSign(SignImpl(psm[csa_] * psm[original_array]),
                                       SignDeterminedStatus::determined);
         return;
      }
      std::sort(csa_.get().begin(), csa_.get().end());
      sign_ = MaybeUndeterminedSign(psm[original_array],
                                    SignDeterminedStatus::undetermined);
   }

   MaybeUndeterminedSign sign() const { return sign_; }

   const CanonicalSolidArray& representative() const {
      return csa_;
   }

   template<typename Output>
   friend
   Output& operator<< (GenericOutput<Output>& outs, const CanonicalSolidRep& r) {
      outs.top() << "[" << r.representative() << "]";
      if (!r.sign().determined())
         outs.top() << "?";
      return outs.top();
   }
};


// --------------------------------------
// GP_Term
// --------------------------------------

class CanonicalSolidMemoizer   
   : public Memoizer<CanonicalSolidMemoizer, SolidSet, CanonicalSolidRep> {
   SphereData& sd_;
   PermutationSignMemoizer& psm_;

public:
   
   const CanonicalSolidRep
   data_from(const SolidSet& f) {
      return CanonicalSolidRep(f, sd_, psm_);
   }

   CanonicalSolidMemoizer(SphereData& sd,
                          PermutationSignMemoizer& psm)
      : sd_(sd)
      , psm_(psm)
   {}
};

#if POLYMAKE_DEBUG   
extern CanonicalSolidMemoizer* global_csm;   // for debug printing
#endif
   
class GP_Term {
   CanonicalSolidRep r1_, r2_;
   MaybeUndeterminedSign sign_;

public:

   GP_Term(const SolidSet& s1,
           const SolidSet& s2,
           const SignImpl& sigma,
           CanonicalSolidMemoizer& csm)
      : r1_(csm[s1])
      , r2_(csm[s2])
      , sign_(r1_.sign() * r2_.sign() * sigma)
   {}

   const CanonicalSolidRep& r1() const { return r1_; }
   const CanonicalSolidRep& r2() const { return r2_; }
   const MaybeUndeterminedSign& sign() const { return sign_; }
   const bool two_undetermined_signs() const {
      return
         !r1_.sign().determined() &&
         !r2_.sign().determined();
   }
   void invert_sign() { sign_.invert(); };

   template<typename Output>
   friend
   Output& operator<< (GenericOutput<Output>& outs, const GP_Term& t) {
      Output& os = outs.top();
      os << "[" << t.r1() << "]";
      if (!t.r1().sign().determined())
         os << "?";
      os << "[" << t.r2() << "]";
      if (!t.r2().sign().determined())
         os << "?";
      return os;
   }
};

struct SushSetTag {};
using SushSet = NamedType<Set<Int>, SushSetTag>;
   
class PluckerRel {
   bool has_two_adjacent_undetermineds_;
   Phi phi_;
   std::vector<GP_Term> terms_;
   SushVector sush_vector_;

   void make_terms(const Set<Int>& I,
                   const Set<Int>& J,
                   const SignImpl sigma,
                   CanonicalSolidMemoizer& csm)
   {
      assert (I.size() + 2 == J.size());

      SolidSet Iplus, Jminus;
      for (const Int j: J) {
         if (I.contains(j))
            continue;
         Iplus.get() = I;  Iplus.get()  += j;
         Jminus.get() = J; Jminus.get() -= j;
         terms_.emplace_back(Iplus, Jminus, SignImpl(sigma * sgn(j, I, J)), csm);
         if (terms_.back().two_undetermined_signs()) {
            has_two_adjacent_undetermineds_ = true;
            return;
         }
      }
   }

   void
   make_sushes() {
      for (const auto& term: terms_) 
         if (!(term.sign().determined()))
            sush_vector_.push_back(UndeterminedSolidHasher((term.r1().sign().determined()
                                                              ? term.r2().representative()
                                                              : term.r1().representative()), 
                                                             term.sign().sign_or_signature()).hash());
      std::sort(sush_vector_.begin(), sush_vector_.end());
}

public:

   PluckerRel(const Set<Int>& I,
              const Set<Int>& J,
              const SignImpl sigma,
              CanonicalSolidMemoizer& csm)
      : has_two_adjacent_undetermineds_(false)
      , phi_(PluckerHasher(I, J, sigma).phi())
   {
      make_terms(I, J, sigma, csm);
      make_sushes();
   }

   PluckerRel(const Phi& phi,
              CanonicalSolidMemoizer& csm)
      : has_two_adjacent_undetermineds_(false)
      , phi_(phi)
   {
      PluckerHasher hasher_(phi);
      make_terms(hasher_.I(), hasher_.J(), SignImpl(sign(phi.get())), csm);
      make_sushes();
   }

   void invert_sign() {
      for (auto& term: terms_)
         term.invert_sign();
      phi_.get() = -phi_;
      for (auto& sush: sush_vector_)
         sush.get() = -sush;
      std::sort(sush_vector_.begin(), sush_vector_.end());
   }

   const SushVector&
   sush_vector() const { return sush_vector_; }

   const Int
   n_undetermineds() const { return sush_vector_.size(); }

   const bool
   has_two_adjacent_undetermineds() const { return has_two_adjacent_undetermineds_; }
   
   bool are_determined_signs_positive() const {
      for (const auto& term: terms_)
         if (term.sign().determined() &&
             -1 == term.sign().sign_or_signature())
            return false;
      return true;
   }
   
   bool has_sush(const Sush sush) const {
      return
         std::find(sush_vector_.begin(), sush_vector_.end(), sush)
         != sush_vector_.end();
   }
   
   const Phi&
   phi() const { return phi_; }

   const std::vector<GP_Term>&
   terms() const { return terms_; }

   template<typename Output>
   friend
   Output& operator<< (GenericOutput<Output>& outs, const PluckerRel& r) {
      Output& os = outs.top();
      for (const auto& term: r.terms()) {
         os << ( term.sign().sign_or_signature() == 1
                 ? "+"
                 : "-" )
            << term.r1() << term.r2();
      }
      return os;
   }

#if POLYMAKE_DEBUG
   void dump() const __attribute__((used)) { cerr << *this << std::flush; }
#endif
   
};      

// --------------------------------------
// more memoizers
// --------------------------------------
   
class PluckerRelationMemoizer
   : public Memoizer<PluckerRelationMemoizer, Phi, PluckerRel> {
   CanonicalSolidMemoizer& csm_;

public:
   
   const PluckerRel
   data_from(const Phi& phi) {
      return PluckerRel(phi, csm_);
   }

   PluckerRelationMemoizer(CanonicalSolidMemoizer& csm)
      : csm_(csm) {}

   void insert(const PluckerRel& pr) {
      // inserting a whole tree is costly, so we first check if it's needed
      if (storage_.find(pr.phi().get()) == storage_.end())
         storage_.insert({ pr.phi().get(), pr});
   }
};

   
std::vector<Set<Int>>
facets_containing_H_rests(const Set<Int>& H,
                          const Array<FacetAsSet>& facets);
   
class SelfTamingMemoizer {
   const SphereData& sphere_data_;
   Int n_access;
   Int n_duplicate;
   std::map<Set<Int>, std::vector<Set<Int>>> self_tamed_Js_complementing;

   bool
   is_self_taming(const Set<Int>& Jpart,
                  const Set<Int>& candidate,
                  const std::vector<Set<Int>>& fcHr)
   {
      Set<Int> J(Jpart);
      J += candidate;

      if (fcHr.size() &&
          fcHr.front().size() > J.size() - 1)
         return false;
      
      // TODO: Here is an opportunity for optimization.
      // Maybe there is a way of not having to go through all max-minus-one subsets of J
      for (auto rest_it = entire(all_subsets_less_1(J)); !rest_it.at_end(); ++rest_it) {
         const Set<Int> rest(*rest_it); // instantiate this so that the repeated incl()s below are faster
         bool contains_a_facet(false);
         for (const auto& facet: fcHr) {
            if (incl(facet, rest) <= 0) {
               contains_a_facet = true;
               break;
            }
         }
         if (!contains_a_facet)
            return false;
      }
      return true;
   }
   
   std::vector<Set<Int>>&
   make_key_entry(const Set<Int>& Jpart) {
      std::vector<Set<Int>> self_taming_Js;
      const std::vector<Set<Int>> fcHr = facets_containing_H_rests(Jpart, sphere_data_.facets);

      for (auto sit = entire(all_subsets_of_k(sequence(0,sphere_data_.n) - Jpart,
                                              sphere_data_.d + 2 - Jpart.size())); !sit.at_end(); ++sit) {
         const Set<Int> candidate(*sit);
         if (is_self_taming(Jpart, candidate, fcHr))
            self_taming_Js.emplace_back(candidate);
      }

      return self_tamed_Js_complementing.insert({Jpart, self_taming_Js}).first->second;
   }
   
public:
   SelfTamingMemoizer(const SphereData& sphere_data)
      : sphere_data_(sphere_data)
      , n_access(0)
      , n_duplicate(0)
   {}

   std::vector<Set<Int>>&
   possible_self_tamed_Js(const Set<Int>& Jpart) {
      ++n_access;
      auto it = self_tamed_Js_complementing.find(Jpart);
      if (it != self_tamed_Js_complementing.end()) {
         ++n_duplicate;
         return it->second;
      }
      return make_key_entry(Jpart);
   }

   const Int n_accessed() const { return n_access; }
   const Int n_duplicates() const { return n_duplicate; }
   const Int size() const { return self_tamed_Js_complementing.size(); }
};

Int
image_of(const Int bitset,
         const Array<Int>& g,
         const Int offset);

constexpr Int low_bitmask ( (Int(1) << max_n_vertices) - 1);
constexpr Int high_bitmask( low_bitmask << max_n_vertices);

Phi
image_of_abs(const Phi phi,
             const Array<Int>& g);
   
const std::vector<PluckerRel>
pluckers_containing_sush(SphereData& sphere_data,
                         CanonicalSolidMemoizer& csm,
                         PluckerRelationMemoizer& prm,
                         const Sush& sush,
                         SelfTamingMemoizer& stm,
                         IntParams& int_params);

   
class PluckersContainingSushMemoizer
   : public Memoizer<PluckersContainingSushMemoizer, Sush, Set<Phi>> {
   SphereData& sd_;
   CanonicalSolidMemoizer& csm_;
   PluckerRelationMemoizer& prm_;
   SelfTamingMemoizer& stm_;
   IntParams& ip_;

   void
   add_image(const Sush sush,
             const Phi phi,
             const Array<Int>& g)
   {
      const Sush sush_g(image_of(abs(sush.get()), g, 0));

      PluckerRel pr_g(image_of_abs(Phi(abs(phi.get())), g), csm_);
      if (pr_g.are_determined_signs_positive()) {
         storage_[sush_g.get()] += pr_g.phi();
         prm_.insert(pr_g);
      }
            
      pr_g.invert_sign();
      if (pr_g.are_determined_signs_positive()) {
         storage_[sush_g.get()] += pr_g.phi();
         prm_.insert(pr_g);
      }         
   }
   
public:

   const Set<Phi>
   data_from(const Sush& sush) {
      Set<Phi> solicited_data;
      for (const auto& pr: pluckers_containing_sush(sd_, csm_, prm_, sush, stm_, ip_)) {
         if (std::find(pr.sush_vector().begin(), pr.sush_vector().end(), sush) != pr.sush_vector().end())
            solicited_data += pr.phi();

         for (const Array<Int>& g: sd_.vertex_symmetry_group) 
            add_image(sush, pr.phi(), g);
      }
      
      return solicited_data;
   }

   PluckersContainingSushMemoizer(SphereData& sd,
                                  CanonicalSolidMemoizer& csm,
                                  PluckerRelationMemoizer& prm,
                                  SelfTamingMemoizer& stm,
                                  IntParams& ip)
      : sd_(sd)
      , csm_(csm)
      , prm_(prm)
      , stm_(stm)
      , ip_(ip)
   {}
   
};


// --------------------------------------------   
// GP_Tree
// --------------------------------------------   

// the sush is the one on the phi. The phi in upstream will have -sush.
using TreeConnector = std::pair<PhiOrCubeIndex, Sush>;

struct GP_Tree_Node {
   PhiOrCubeIndex self;
   std::vector<TreeConnector> upstream;

   GP_Tree_Node(const PhiOrCubeIndex i)
      : self(i)
      , upstream()
   {}

   // fake constructor for use in cubes
   GP_Tree_Node(const PhiOrCubeIndex i,
                const PhiOrCubeIndex upstream_index)
      : self(i)
      , upstream()
   {
      const TreeConnector tc(std::make_pair(upstream_index, Sush(0)));
      upstream.push_back(tc);
   }
};

using NodeContainer = std::vector<GP_Tree_Node>;
using HungrySushesAt = std::map<PhiOrCubeIndex, SushVector>;

   // for use with cube vertices later on   
struct VertexIdTag {};
using VertexId = NamedType<Int, VertexIdTag>;

bool
is_cube_vertex(const GP_Tree_Node& node);
   
   
class GP_Tree {
   TreeIndex index_;
   NodeContainer nodes_;
   hash_set<PhiOrCubeIndex> node_support_;
   HungrySushesAt hungry_sushes_at_;
   SushVector sush_vector_;
   hash_set<Sush> sushes_set_;

public:

   const TreeIndex index() const { return index_; }
   void set_index(const TreeIndex i) { index_ = i; }
   void modify_nodes(const PhiOrCubeIndex from, const PhiOrCubeIndex to);
   const PhiOrCubeIndex root() const { return nodes_.front().self; }
   const Int size() const { return nodes_.size(); }
   const NodeContainer& nodes() const { return nodes_; }
   const hash_set<PhiOrCubeIndex>& node_support() const { return node_support_; }
   const HungrySushesAt& hungry_sushes_at() const { return hungry_sushes_at_; }
   const SushVector& hungry_sushes_at(const PhiOrCubeIndex phi) const { return hungry_sushes_at_.at(phi); }
   const SushVector& sush_vector() const { return sush_vector_; }
   const hash_set<Sush>& sushes_set() const { return sushes_set_; }

   const PhiOrCubeIndex phi_containing_hungry_sush(const Sush sush) const {
      for (const auto& phi_sushes: hungry_sushes_at_)
         if (std::find(phi_sushes.second.begin(), phi_sushes.second.end(), sush) !=
             phi_sushes.second.end())
            return phi_sushes.first;
      throw std::runtime_error("phi_containing_hungry_sush: looked for nonexistent sush");
   }

   const VertexId cube_vertex_upstream_of(const PhiOrCubeIndex phi) const;
   
private:

   void
   incorporate_nodes(const GP_Tree& other,
                     const Sush common_sush,
                     const PhiOrCubeIndex this_phi,
                     const PhiOrCubeIndex other_phi);
   
   void remove_one_sush(const PhiOrCubeIndex phi,
                        const Sush sush);
   
   void 
   remove_sush_from_hungry_sushes_at(const PhiOrCubeIndex this_phi,
                                     const PhiOrCubeIndex other_phi,
                                     const Sush common_sush) {
      remove_one_sush(this_phi, common_sush);
      remove_one_sush(other_phi, Sush(-common_sush));
   }


   void remove_sush(const Sush sush);

   void
   complete_coupling(const GP_Tree& other,
                     const Sush common_sush,
                     const PhiOrCubeIndex this_phi,
                     const PhiOrCubeIndex other_phi);

   PhiOrCubeIndex
   first_hungry_phi_with_sush(const Sush sush) const {
      for (const auto& phi_sushes: hungry_sushes_at_)
         if (std::find(phi_sushes.second.begin(), phi_sushes.second.end(), sush) != phi_sushes.second.end())
            return phi_sushes.first;
      return PhiOrCubeIndex(-1);
   }

   
   std::pair<PhiOrCubeIndex, PhiOrCubeIndex>
   common_phis(const GP_Tree& other,
               const Sush sush) const {
      return { first_hungry_phi_with_sush( sush),
               other.first_hungry_phi_with_sush(Sush(-sush)) };
   }

   /*
   SolutionStatus
   prune_from(const CubeIndex cid,
              const VertexId vid,
              const PhiOrCubeIndex phi_to_keep,
              SearchData& sd,
              const IntParams& ip);
   */
   
public:

   GP_Tree() {}
   
   // make a tree with a single node and predetermined hungry sushes
   GP_Tree(const TreeIndex index,
           const PhiOrCubeIndex phi,
           const SushVector& sushes)
      : index_(index)
   {
      nodes_.emplace_back(phi);
      node_support_ += phi;
      hungry_sushes_at_[phi] = sushes;
      sush_vector_ = sushes;
      for (const auto sush: sushes)
         sushes_set_ += sush;
   }

   // make a tree with a single node and all hungry sushes
   GP_Tree(const TreeIndex index,
           const Phi phi,
           CanonicalSolidMemoizer& csm)
      : index_(index)
   {
      nodes_.emplace_back(PhiOrCubeIndex(phi.get()));
      node_support_ += PhiOrCubeIndex(phi.get());
      const PluckerRel pr(phi, csm);
      hungry_sushes_at_[PhiOrCubeIndex(phi.get())] = pr.sush_vector();
      sush_vector_ = pr.sush_vector();
      for (const auto sush: pr.sush_vector())
         sushes_set_ += sush;
   }

   // fake tree node inside a cube
   GP_Tree(const VertexId vertex_id,
           const CubeIndex cube_id);

   void flat_insert_from(const GP_Tree& other,
                         const SushVector& signing);

   void
   add_tree(const GP_Tree& other,
            const Sush common_sush,
            SearchData& sd,
            const IntParams& ip);

#if POLYMAKE_DEBUG
   
   template<typename Output>
   friend
   Output& operator<< (GenericOutput<Output>& outs, const GP_Tree& t)
   {
      Output& os = outs.top();

      os  << "---- tree with id " << t.index() << " and nodes:\n";
      Int ct(0);
      for (const auto& node: t.nodes()) {
         os << "  " << ct++ << " (" << node.self << "): ";
         if (node.self.get() < first_cube_index) {
            // it's a tree node
            if (is_cube_vertex(node)) {
               // it's a cube vertex node
               os << "connector node " << node.self;
            } else {
               // it's actually a Plucker node
               const PluckerHasher ph(Phi(node.self));
               os << ((node.self.get() < 0)
                      ? "-"
                      : "+")
                  << "(" << ph.I() << "|" << ph.J() << ") = "
                  << PluckerRel(Phi(node.self), *global_csm);
            }
         } else 
            os << "cube node " << node.self.get() - first_cube_index;
         os << endl;
      }
      os << "hungry_sushes_at { ";
      for (const auto& phi_sushes: t.hungry_sushes_at()) {
         os << phi_sushes.first << ": ";
         for (const auto sush: phi_sushes.second) {
            os << sush << "=";
            if (sush.get() < 0)
               os << "-";
            for (const Int i: UndeterminedSolidHasher(sush).solid().get())
               os << i << " ";
            os << "; ";
         }
      }
      os << "}\n"
         << "sushes: " << t.sush_vector() << "\n"
         << "* ..... connectivity:\n";
      for (const auto& node: t.nodes()) {
         os << node.self << ": " << node.upstream << endl;
      }
      return os << "---- done with tree " << t.index() << endl;
   }

   void dump() const __attribute__((used)) { cerr << *this << std::flush; }
#endif
};

   
// --------------------------------------------   
// function forward declarations
// --------------------------------------------   
SphereData
retrieve_sphere_data(BigObject s_in);

IntParams
retrieve_int_params(OptionSet& options,
                    const std::string& id_string);

// returns all plucker relations with at most int_params.max_undetermined_ct undetermined solids   
SolutionStatus
initialize_plucker_relations(SphereData& sphere_data,
                             PluckerData& pd,
                             CanonicalSolidMemoizer& csm,
                             PluckerRelationMemoizer& prm,
                             const IntParams& ip,
                             const std::string& id_string);

// for cubes   
SolutionStatus
re_initialize_plucker_relations(SphereData& sphere_data,
                                PluckerData& pd,
                                SearchData& sd,
                                CanonicalSolidMemoizer& csm,
                                PluckerRelationMemoizer& prm,
                                const IntParams& ip);
   
BigObject
make_solution(const SearchData& sd,
              CanonicalSolidMemoizer& csm);

SolutionStatus
initialize_tree_list(SearchData& sd,
                     const SphereData& sphere_data,
                     IntParams& ip,
                     const PluckerData& pd,
                     CanonicalSolidMemoizer& csm);


enum class TreeAddingAction {
   add_to_existing,
   dont_add_to_existing
};

enum class TreeCompletingAction {
   complete,
   dont_complete
};
   
SolutionStatus
process_tree(SearchData& sd,
             const IntParams& int_params,
             GP_Tree& t,
             const TreeAddingAction taa,
             const TreeCompletingAction tca);

bool
is_plucker_rel_acceptable(const PluckerRel& pr,
                          const IntParams& int_params,
                          PluckerStats& stats);

template<typename SushSignAcceptanceCriterion>   
SolutionStatus
process_one_plucker_rel(const PluckerRel& pr,
                        const IntParams& int_params,
                        PluckerData& local_pd,
                        PluckerRelationMemoizer& prm,
                        SushSignAcceptanceCriterion ssac)
{
   ++ local_pd.stats.total_processed;
   if (!is_plucker_rel_acceptable(pr, int_params, local_pd.stats) ||
       !ssac(pr))
      return SolutionStatus::not_found;

   local_pd.seen_pluckers += pr.phi();
   local_pd.plucker_rel_list.push_back(pr);
   prm.insert(pr);
   
   if (0 == pr.n_undetermineds())
      return SolutionStatus::found_single_positive_pr;

   return SolutionStatus::not_found;
}
                        
template<typename SushSignAcceptanceCriterion>
SolutionStatus
process_plucker_rel(const Set<Int>& I,
                    const Set<Int>& J,
                    CanonicalSolidMemoizer& csm,
                    PluckerRelationMemoizer& prm,
                    const IntParams& int_params,
                    PluckerData& local_pd,
                    SushSignAcceptanceCriterion ssac)
{
   PluckerRel pr(I, J, SignImpl(1), csm);
   const SolutionStatus ss =
      process_one_plucker_rel(pr, int_params, local_pd, prm, ssac);
   if (SolutionStatus::not_found != ss)
      return ss;

   pr.invert_sign();
   return
      process_one_plucker_rel(pr, int_params, local_pd, prm, ssac);
}

bool
already_in_orbit(const Set<Int>& I,
                 const Set<Int>& J,
                 const ExplicitGroup& G,
                 PhiOrbit& seen_phis);

IndexOfFacet
make_iof(const Array<FacetAsSet>& facets);

LabelData
make_labels(BigObject& s_in);

   
} // end namespace gp

} }

#endif // POLYMAKE_GRASS_PLUCKER_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
 
  

 
