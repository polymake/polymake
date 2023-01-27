/* Copyright (c) 1997-2023
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
#undef DOMAIN
#include "polymake/Array.h"
#include "polymake/ListMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/hash_map"
#include "polymake/hash_set"
#include <list>

#include "polymake/group/permlib_tools.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#if !defined(__APPLE__) && __clang_major__ >= 13
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "permlib/permlib_api.h"
#include "permlib/generator/bsgs_generator.h"
#include "permlib/export/bsgs_schreier_export.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// TEMPORARY FIX:
// permlib uses std::set in OrbitSet where Vector can sneak in as a key
namespace std {

template <typename E>
struct less<pm::Vector<E>> : polymake::operations::lex_less { };

}

namespace polymake { namespace group {


class PermlibGroup {
protected:
   boost::shared_ptr<permlib::PermutationGroup> permlib_group;
  
public:
   PermlibGroup(){};
   PermlibGroup(boost::shared_ptr<permlib::PermutationGroup> perm_group) : permlib_group(perm_group){};

   PermlibGroup(const Array<Array<Int>>& _generators)
   {
      /*
        Issue #1194:

        For a non-empty list of generators, we translate them directly into permlib format 
        and return a pointer to a permlib::PermutationGroup object.
        This also works if the list of generators consists of only an identity permutation.

        However, if the list of generators is empty, paradoxically we have to do more work, because
        permlib does not deal gracefully with a trivial group specified by giving no generators.
        One issue that arises is that the degree (=cardinality of the ground set) of such a permutation is not known;
        another issue is that the implementation of the Schreier-Sims algorithm fails in this case.

        We work around this by substituting an empty list of generators 
        by the list consisting of the permutation (0), of degree 1.

        This has the disadvantage that the degree of the permutation group given by no generators
        is manually set to 1. 
        
        If this should turn out to be a problem (like it did for empty matrices, which turned out to 
        desperately want to know their number of rows and columns), the code needs to be reworked.
       */
      Array<Array<Int>> trivial_group_generators;
      if (is_zero(_generators.size())) {
         trivial_group_generators.append({{0}});
      }
      const Array<Array<Int>>& generators( is_zero(_generators.size())
                                           ? trivial_group_generators
                                           : _generators );
      
      std::list<boost::shared_ptr<permlib::Permutation>> permutations;
      for (const auto& perm : generators) {
         boost::shared_ptr<permlib::Permutation> gen(new permlib::Permutation(perm.begin(), perm.end()));
         permutations.push_back(gen);
      }
      permlib_group = construct(generators[0].size(), permutations.begin(), permutations.end());
   }

   Int degree() const
   {
      return permlib_group->n;
   }

   Int order() const
   {
      return permlib_group->order<Int>();
   }
  
   boost::shared_ptr<permlib::PermutationGroup> 
   get_permlib_group() const
   {
      return permlib_group;
   }

   Array<Array<Int>> strong_gens() const
   {
      Array<Array<Int>> permutations(permlib_group->S.size());
      Int count = 0;
      for (const auto& perm : permlib_group->S) {
         permutations[count]=perm2Array(perm);
         ++count;
      }    
      return permutations;
   }

   Array<Int> base() const
   {
      return Array<Int>(permlib_group->B.size(), entire(permlib_group->B));
   }

   Array<Array<Array<Int>>> transversals() const
   {
      const Int base_length = permlib_group->B.size();
      Array<Array<Array<Int>>> trans(base_length);
      for (Int i = 0; i < base_length; ++i) {
         Array<Array<Int>> single_trans(permlib_group->n);
         for (Int j = 0; j < Int(permlib_group->n); ++j) {
            boost::shared_ptr<permlib::Permutation> perm(permlib_group->U[i].at(j));
            single_trans[j] = perm2Array(perm);
         }
         trans[i] = single_trans;
      }
      return trans;
   }

   static Array<Int> perm2Array(const boost::shared_ptr<permlib::Permutation>& perm)
   {
      return (!perm)
         ? Array<Int>()
         : perm2Array(*perm.get());
   }

   static Array<Int> perm2Array(const permlib::Permutation& perm)
   {
      Array<Int> gen(perm.size());
      for (permlib::dom_int i = 0; i < perm.size(); ++i) {
         gen[i] = perm.at(i);
      }
      return gen;
   } 


   Array<hash_set<Int>> orbits() const
   {
      const auto orbit_ptr_list = permlib::orbits(*permlib_group);

      Array<hash_set<Int>> orbit_decomp(orbit_ptr_list.size());
      auto odec_it = orbit_decomp.begin();
      for (const auto& orbit_ptr : orbit_ptr_list) {
         hash_set<Int> orbit_set;
         for (const auto& orb_it : *orbit_ptr) {
            const unsigned long elem(orb_it);
            orbit_set += static_cast<Int>(elem);
         }
         *odec_it = orbit_set;
         ++odec_it;
      }
      return orbit_decomp;
   }

   template <typename Container>
   hash_set<Container> unordered_orbit(const Container& c) const
   {
      return orbit_impl(*this, c);
   }

   template <typename Container>
   Set<Container> orbit(const Container& c) const
   {
      return Set<Container>(pm::entire_range(orbit_impl(*this, c)));
   }
   

   template <typename SetType>
   PermlibGroup setwise_stabilizer(const SetType& set) const
   {
     return PermlibGroup(permlib::setStabilizer(*(permlib_group), set.begin(), set.end()));
   }


   template <typename Scalar >
   PermlibGroup vector_stabilizer(const Vector<Scalar>& vec) const
   {
      boost::shared_ptr<permlib::PermutationGroup> stab;

      std::list<permlib::dom_int> list;
      hash_map<Scalar, permlib::dom_int> hash;
      permlib::dom_int value = 0;
      for (Int i = 1; i < vec.size(); ++i) { //skip homogenizing entry
         if (hash.find(vec[i]) == hash.end()) {
            if (value == std::numeric_limits<permlib::dom_int>::max())
               throw std::runtime_error("input vector is too big for permlib");
            hash[vec[i]] = value;
            ++value;
         }
         list.push_back(hash[vec[i]]);
      }
 
      stab = permlib::vectorStabilizer(*permlib_group, list.begin(), list.end(), value-1);

      return PermlibGroup(stab);
   }
  

   static PermlibGroup permgroup_from_cyclic_notation(const Array<std::string>& cyc_not, Int degree_, Array<Array<Int>>& parsed_generators)
   {
      const permlib::dom_int degree = permlib::safe_to_dom_int(degree_);
      std::list<permlib::Permutation::ptr> gens;
      parsed_generators = Array<Array<Int>>(cyc_not.size());    
      for (Int i = 0; i < cyc_not.size(); ++i) {
         permlib::Permutation::ptr gen(new permlib::Permutation(degree, cyc_not[i]));
         gens.push_back(gen);
         //fill Array with parsed generators
         parsed_generators[i] = perm2Array(gen);
      }
      return PermlibGroup(permlib::construct(degree, gens.begin(), gens.end()));
   }


   permlib::dset lex_min_representative(const permlib::dset& dGamma) const
   {
      return smallestSetImage(*permlib_group, dGamma);
   }

   template<typename SetType>
   SetType lex_min_representative(const SetType& dGamma_in) const
   {
      permlib::dset dGamma(permlib_group->n);
      for (const auto s : dGamma_in)
         dGamma.set(s);
      permlib::dset image = smallestSetImage(*permlib_group, dGamma);
      SetType res(permlib_group->n);
      size_t index = image.find_first();
      while (index != permlib::dset::npos) {
         res += index;
         index = image.find_next(index);
      }
      return res;
   }

   Set<Int> lex_min_representative(const Set<Int>& input_rep) const
   {
      permlib::dset dGamma(permlib_group->n);
      for (const auto& s : input_rep)
         dGamma.set(s);
      Set<Int> rep;
      const permlib::dset dGammaLeast = smallestSetImage(*permlib_group, dGamma);
      for (Int i = 0; i < Int(permlib_group->n); ++i)
         if (dGammaLeast[i])
            rep += i;
      return rep;      
   }
};


Array<Array<Int>> generators_from_permlib_group(const PermlibGroup& permlib_group);

BigObject perl_action_from_group(const PermlibGroup& permlib_group,
                                    const std::string& name = "",
                                    const std::string& description = "action defined from permlib group");

PermlibGroup group_from_perl_action(BigObject action);

BigObject perl_group_from_group(const PermlibGroup& permlib_group,
                                   const std::string& name = "",
                                   const std::string& description = "group defined from permlib group");

void perl_action_from_generators(const Array<Array<Int>>& generators,
                                 BigObject action,
                                 OptionSet options);

/*  
    Actions
*/
template<class PERM, typename Scalar>
struct CoordinateAction {
   Vector<Scalar> operator()(const PERM& p, const Vector<Scalar>& vec)
   {
      Vector<Scalar> ret(vec);
      for (Int i = 1; i < vec.size(); ++i) {
         ret[i] = vec[p.at(permlib::safe_to_dom_int(i-1))+1];
      }
      return ret;
   }
};

template <typename PERM>
struct SetOfIndicesAction {
   Set<Int> operator()(const PERM& p, const Set<Int>& s)
   {
      Set<Int> ret;
      for (const auto& index : s)
         ret += p.at(permlib::safe_to_dom_int(index));
      return ret;
   }
};


template <typename Container>
hash_set<Container> orbit_impl (const PermlibGroup& sym_group, const Container& c)
{
   typedef permlib::Permutation PERM;
   permlib::OrbitSet<PERM, Container> orbit;
   orbit.orbit(c, sym_group.get_permlib_group()->S, permlib::ContainerAction<PERM, Container>());
   return hash_set<Container>(orbit.begin(), orbit.end());
}

/*
  @input PermutationAction coordinate_action a group action on coordinates
  @input Matrix generating_vectors a matrix whose rows are acted upon by the group
  @output Pair<ListMatrix<Vector>, Array<Set>> the orbits of generating_vectors, in orbit order, and the indices of the orbits
 */
template<typename _Matrix, typename Scalar>
std::pair<ListMatrix<Vector<Scalar>> , Array<hash_set<Int>> >
orbits_in_orbit_order_impl(BigObject coordinate_action, const GenericMatrix<_Matrix, Scalar>& generating_vectors)
{
   typedef typename std::pair<ListMatrix<Vector<Scalar>>, Array<hash_set<Int>>> ListMatrixOrbitPair;

   const PermlibGroup group_of_cone = group_from_perl_action(coordinate_action);
 
   if (generating_vectors.rows() == 0) 
      return ListMatrixOrbitPair();

   if (generating_vectors.cols() <= group_of_cone.degree())
      throw std::runtime_error("orbits_coord_action_complete_sub: group/matrix dimension mismatch: group degree greater than (number of matrix columns)-1");

   // convert the rows of the matrix to a vector of Vectors in order to feed it to permlib
   std::vector<Vector<Scalar>> permlib_generating_vectors;
   permlib_generating_vectors.reserve(generating_vectors.rows());
   for (auto vec=entire(rows(generating_vectors.top())); !vec.at_end(); ++vec) {
      permlib_generating_vectors.push_back(*vec);
   }

   // call permlib to get the list of orbits generated by the rows of the input matrix
   const auto orbit_ptr_list = permlib::orbits<Vector<Scalar>, CoordinateAction<permlib::Permutation,Scalar>>(*(group_of_cone.get_permlib_group()), permlib_generating_vectors.begin(), permlib_generating_vectors.end());

   Array<hash_set<Int>> orbit_indices(orbit_ptr_list.size()); 
   ListMatrix<Vector<Scalar>> vectors_in_orbit_order;
   Int orbit_count = 0;
   Int vec_count = 0;
   for (const auto& orbit_ptr : orbit_ptr_list) {
      hash_set<Int> indices_in_one_orbit;
      for (const auto& orb_it : *orbit_ptr) {
         vectors_in_orbit_order /= orb_it; //vectors of one orbit form one block of the matrix all_orbit_vectors	  
         indices_in_one_orbit += vec_count++;
      }
      orbit_indices[orbit_count++] = indices_in_one_orbit;
   }
   return ListMatrixOrbitPair(vectors_in_orbit_order, orbit_indices);
}

inline
std::vector<Array<Int>> 
all_group_elements_impl(const PermlibGroup& perm_group)
{
   std::vector<Array<Int>> all_elements;
   permlib::BSGSGenerator<permlib::TRANSVERSAL> bsgsGen(perm_group.get_permlib_group()->U);
   while (bsgsGen.hasNext()) {
      all_elements.push_back(PermlibGroup::perm2Array(bsgsGen.next()));
   }
   return all_elements;
}

} // end namespace group
} // end namespace polymake

namespace pm {
#if defined(__GLIBCXX__)

template <typename T>
struct iterator_cross_const_helper<std::_Rb_tree_iterator<T>, true> {
   typedef std::_Rb_tree_iterator<T> iterator;
   typedef std::_Rb_tree_const_iterator<T> const_iterator;
};
template <typename T>
struct iterator_cross_const_helper<std::_Rb_tree_const_iterator<T>, true> {
   typedef std::_Rb_tree_iterator<T> iterator;
   typedef std::_Rb_tree_const_iterator<T> const_iterator;
};

#elif defined(_LIBCPP_VERSION)

template <class _Tp, class _NodePtr, class _DiffType>
struct iterator_cross_const_helper<std::__tree_iterator<_Tp, _NodePtr, _DiffType>, true> {
   typedef std::__tree_iterator<_Tp, _NodePtr, _DiffType> iterator;
   typedef std::__tree_const_iterator<_Tp, _NodePtr, _DiffType> const_iterator;
};
template <class _Tp, class _NodePtr, class _DiffType>
struct iterator_cross_const_helper<std::__tree_const_iterator<_Tp, _NodePtr, _DiffType>, true> {
   typedef std::__tree_iterator<_Tp, _NodePtr, _DiffType> iterator;
   typedef std::__tree_const_iterator<_Tp, _NodePtr, _DiffType> const_iterator;
};

#endif
}


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
