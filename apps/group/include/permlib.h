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

#ifndef POLYMAKE_GROUP_PERMLIB_H
#define POLYMAKE_GROUP_PERMLIB_H
#undef DOMAIN
#include "polymake/Array.h"
#include "polymake/ListMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/hash_map"
#include "polymake/hash_set"

#include "polymake/group/permlib_tools.h"

#include "permlib/permlib_api.h"
#include "permlib/generator/bsgs_generator.h"
#include "permlib/export/bsgs_schreier_export.h"
#include <list>

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

   PermlibGroup(const Array<Array <int>>& generators)
   {
      std::list<boost::shared_ptr<permlib::Permutation>> permutations;
      for (const auto& perm : generators) {
         boost::shared_ptr<permlib::Permutation> gen(new permlib::Permutation(perm.begin(), perm.end()));
         permutations.push_back(gen);
      }
      permlib_group = construct(generators[0].size(), permutations.begin(), permutations.end());
   }

   int degree() const
   {
      return permlib_group->n;
   }

   int order() const
   {
      return permlib_group->order();
   }
  
   boost::shared_ptr<permlib::PermutationGroup> 
   get_permlib_group() const
   {
      return permlib_group;
   }

   Array<Array<int>> strong_gens() const
   {
      Array<Array<int>> permutations(permlib_group->S.size());
      int count=0;
      for (const auto& perm : permlib_group->S) {
         permutations[count]=perm2Array(perm);
         ++count;
      }    
      return permutations;
   }

   Array<int> base() const
   {
      return Array<int>(permlib_group->B.size(), entire(permlib_group->B));
   }

   Array<Array<Array<int>>> transversals() const
   {
      int base_length=permlib_group->B.size();
      Array<Array<Array<int>>> trans(base_length);
      for (int i=0; i<base_length; ++i) {
         Array<Array<int>> single_trans(permlib_group->n);
         for (unsigned int j=0; j<permlib_group->n; ++j) {
            boost::shared_ptr<permlib::Permutation> perm(permlib_group->U[i].at(j));
            single_trans[j]=perm2Array(perm);
         }
         trans[i]=single_trans;
      }
      return trans;
   }

   static Array<int> perm2Array(const boost::shared_ptr<permlib::Permutation>& perm)
   {
      return (!perm)
         ? Array<int>()
         : perm2Array(*perm.get());
   }

   static Array<int> perm2Array(const permlib::Permutation& perm)
   {
      Array<int> gen(perm.size());
      for (unsigned int i=0; i<perm.size(); i++) {
         gen[i]=perm.at(i);
      }
      return gen;
   } 


   Array<hash_set<int>> orbits() const
   {
      const auto orbit_ptr_list = permlib::orbits(*permlib_group);

      Array<hash_set<int>> orbit_decomp(orbit_ptr_list.size());
      auto odec_it = orbit_decomp.begin();
      for (const auto& orbit_ptr : orbit_ptr_list) {
         hash_set<int> orbit_set;
         for (const auto& orb_it : *orbit_ptr) {
            const unsigned long elem(orb_it);
            orbit_set += static_cast<int>(elem);
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
   

   template<typename SetType>
   PermlibGroup setwise_stabilizer(const SetType& set) const
   {
     return PermlibGroup(permlib::setStabilizer(*(permlib_group), set.begin(), set.end()));
   }


   template<typename Scalar >
   PermlibGroup vector_stabilizer(const Vector<Scalar>& vec) const
   {
      boost::shared_ptr<permlib::PermutationGroup> stab;

      std::list<int> list;
      hash_map<Scalar,int> hash;
      int value=0;
      for (int i=1; i<vec.size(); ++i) { //skip homogenizing entry
         if (hash.find(vec[i]) == hash.end()) {
            hash[vec[i]]=value;
            ++value;
         }
         list.push_back(hash[vec[i]]);
      }
 
      stab=permlib::vectorStabilizer(*(permlib_group),list.begin(),list.end(),value-1);

      return PermlibGroup(stab);
   }
  

   static PermlibGroup permgroup_from_cyclic_notation(const Array<std::string>& cyc_not, int degree, Array<Array<int>>& parsed_generators)
   {
      std::list<permlib::Permutation::ptr> gens;
      parsed_generators = Array<Array<int>>(cyc_not.size());    
      for (int i=0; i<cyc_not.size(); ++i) {
         permlib::Permutation::ptr gen(new permlib::Permutation(degree, cyc_not[i]));
         gens.push_back(gen);
         //fill Array with parsed generators
         parsed_generators[i]=perm2Array(gen);
      }
      return PermlibGroup(permlib::construct(degree,gens.begin(),gens.end()));
   }


   permlib::dset lex_min_representative(const permlib::dset& dGamma) const
   {
      return smallestSetImage(*permlib_group, dGamma);
   }

   template<typename SetType>
   SetType lex_min_representative(const SetType& dGamma_in) const
   {
      permlib::dset dGamma(permlib_group->n);
      for (const auto& s : dGamma_in)
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

   Set<int> lex_min_representative(const Set<int>& input_rep) const
   {
      permlib::dset dGamma(permlib_group->n);
      for (const auto& s : input_rep)
         dGamma.set(s);
      Set<int> rep;
      const permlib::dset dGammaLeast = smallestSetImage(*permlib_group, dGamma);
      for (unsigned int i=0; i < permlib_group->n; ++i)
         if (dGammaLeast[i])
            rep += int(i);
      return rep;      
   }
};


Array<Array<int>> generators_from_permlib_group(const PermlibGroup& permlib_group);

perl::Object perl_action_from_group(const PermlibGroup& permlib_group,
                                    const std::string& name = "",
                                    const std::string& description = "action defined from permlib group");

PermlibGroup group_from_perl_action(perl::Object action);

perl::Object perl_group_from_group(const PermlibGroup& permlib_group,
                                   const std::string& name = "",
                                   const std::string& description = "group defined from permlib group");

void perl_action_from_generators(const Array<Array<int>>& generators,
                                 perl::Object action,
                                 perl::OptionSet options);

/*  
    Actions
*/
template<class PERM, typename Scalar>
struct CoordinateAction {
   Vector<Scalar> operator()(const PERM& p, const Vector<Scalar>& vec) {
      Vector<Scalar> ret(vec);
      for (int i=1;i<vec.size();++i){
         ret[i]=vec[p.at(i-1)+1];
      }
      return ret;
   }
};

template<class PERM>
struct SetOfIndicesAction {
   Set<int> operator()(const PERM& p, const Set<int>& s) {
      Set<int> ret;
      for (const auto& index : s)
         ret += p.at(index);
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
std::pair<ListMatrix<Vector<Scalar>> , Array<hash_set<int>> >
orbits_in_orbit_order_impl(perl::Object coordinate_action, const GenericMatrix<_Matrix, Scalar>& generating_vectors)
{
   typedef typename std::pair<ListMatrix<Vector<Scalar>>, Array<hash_set<int>>> ListMatrixOrbitPair;

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

   Array<hash_set<int>> orbit_indices(orbit_ptr_list.size()); 
   ListMatrix<Vector<Scalar>> vectors_in_orbit_order;
   int orbit_count=0;
   int vec_count=0;
   for (const auto& orbit_ptr : orbit_ptr_list) {
      hash_set<int> indices_in_one_orbit;
      for (const auto& orb_it : *orbit_ptr) {
         vectors_in_orbit_order /= orb_it; //vectors of one orbit form one block of the matrix all_orbit_vectors	  
         indices_in_one_orbit += vec_count++;
      }
      orbit_indices[orbit_count++] = indices_in_one_orbit;
   }
   return ListMatrixOrbitPair(vectors_in_orbit_order, orbit_indices);
}

inline
std::vector<Array<int>> 
all_group_elements_impl(const PermlibGroup& perm_group)
{
   std::vector<Array<int>> all_elements;
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

#endif // POLYMAKE_PERMLIB_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
