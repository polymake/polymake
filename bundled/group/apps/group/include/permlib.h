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

#ifndef POLYMAKE_PERMLIB_H
#define POLYMAKE_PERMLIB_H
#undef DOMAIN
#include "polymake/Array.h"
#include "polymake/ListMatrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/hash_map"

#include "polymake/group/group_domain.h"

#include "permlib/permlib_api.h"
#include "polymake/group/permlib_tools.h"
#include <list>

namespace polymake { namespace group {


class PermlibGroup {
protected:
   boost::shared_ptr<permlib::PermutationGroup> permlib_group;
  
public:
   PermlibGroup(){};
   PermlibGroup(boost::shared_ptr<permlib::PermutationGroup> perm_group) : permlib_group(perm_group){};
   PermlibGroup(const Array< Array < int> >& generators){
      std::list<boost::shared_ptr<permlib::Permutation> > permutations;
      for(Entire< Array< Array <int> > >::const_iterator perm = entire(generators); !perm.at_end(); ++perm){
         boost::shared_ptr<permlib::Permutation> gen(new permlib::Permutation((*perm).begin(),(*perm).end()));
         permutations.push_back(gen);
      }
      permlib_group = construct(generators[0].size(), permutations.begin(), permutations.end());
   }

   int degree() const {
      return permlib_group->n;
   }

   int order() const {
      return permlib_group->order();
   }
  
   boost::shared_ptr<permlib::PermutationGroup> get_permlib_group() const{
      return permlib_group;
   }

   Array< Array<int> > strong_gens() const {
      Array< Array<int> > permutations(permlib_group->S.size());
      int count=0;
      for(std::list<boost::shared_ptr<permlib::Permutation> >::const_iterator perm = permlib_group->S.begin(); perm!=permlib_group->S.end(); ++perm){
         permutations[count]=perm2Array(*perm);
         ++count;
      }    
      return permutations;
   }

   Array<int> base() const {
      return Array<int>(permlib_group->B.size(),entire(permlib_group->B));
   }

   Array< Array < Array<int> > > transversals() const {
      int base_length=permlib_group->B.size();
      Array< Array < Array<int> > > trans(base_length);
      for (int i=0; i<base_length; ++i) {
         Array < Array<int> > single_trans(permlib_group->n);
         for (unsigned int j=0; j<permlib_group->n; ++j) {
            boost::shared_ptr<permlib::Permutation> perm(permlib_group->U[i].at(j));
            single_trans[j]=perm2Array(perm);
         }
         trans[i]=single_trans;
      }
      return trans;
   }

   static Array<int> perm2Array(const boost::shared_ptr<permlib::Permutation>& perm) {
      if(!perm){
         return Array<int>();
      } else {
        return perm2Array(*perm.get());
      }
   }

   static Array<int> perm2Array(const permlib::Permutation& perm) {
         Array<int> gen(perm.size());
         for (unsigned int i=0; i<perm.size(); i++) {
            gen[i]=perm.at(i);
         }
         return gen;
   } 

   /*
   template<typename SetType>
   static
   std::vector<int> dset2vector(const SetType& set) {
      std::vector<int> vec;
      vec.reserve(set.size());
      for (typename Entire<SetType>::const_iterator index = entire(set); !index.at_end(); ++index)
         vec.push_back(*index);
      return vec;
   }

   static
   std::vector<int> dset2vector(const permlib::dset& set) {
      std::vector<int> vec;
      vec.reserve(set.size());
      int pos = set.find_first();
      while (pos != permlib::dset::npos) {
         vec.push_back(pos);
         pos = set.find_next(pos);
      }
      return vec;
   }
   */

   Array< Set<int> > orbits() const{

      std::list<boost::shared_ptr<permlib::OrbitAsSet> > orbit_list = permlib::orbits(*permlib_group);

      Array< Set<int> > orbit_decomp(orbit_list.size());
      int count=0;
      for(std::list<boost::shared_ptr<permlib::OrbitAsSet> >::const_iterator orbit = orbit_list.begin(); orbit!=orbit_list.end(); ++orbit){
         Set<int> orbit_set;
         for(permlib::OrbitAsSet::const_iterator orb_it = (*orbit)->begin();orb_it!=(*orbit)->end();++orb_it){
            unsigned long elem=*orb_it;
            orbit_set+=static_cast<int>(elem);
         }
         orbit_decomp[count]=orbit_set;
         count++;
      }
      return orbit_decomp;
   }

   template <typename Container> inline
   Set<Container> orbit(const Container& c) const {
      return orbit_impl(*this, c);
   }


  template<typename SetType>
  PermlibGroup setwise_stabilizer(const SetType& set) const {
     return PermlibGroup(permlib::setStabilizer(*(permlib_group), set.begin(), set.end()));
   }


   template< typename Scalar >
   PermlibGroup vector_stabilizer(const Vector<Scalar>& vec) const {
      boost::shared_ptr<permlib::PermutationGroup> stab;

    std::list<int> list;
    hash_map<Scalar,int> hash;
    int value=0;
    for(int i=1; i<vec.size(); ++i){ //skip homogenizing entry
       if(hash.find(vec[i]) == hash.end()){
          hash[vec[i]]=value;
          value++;
       }
       list.push_back(hash[vec[i]]);
    }
 
    stab=permlib::vectorStabilizer(*(permlib_group),list.begin(),list.end(),value-1);

    return PermlibGroup(stab);
  }
  

   static PermlibGroup permgroup_from_cyclic_notation(const Array<std::string>& cyc_not, int degree, Array< Array<int> >& parsed_generators) {
      std::list<permlib::Permutation::ptr> gens;
      parsed_generators = Array< Array<int> >(cyc_not.size());    
      for(int i=0; i<cyc_not.size(); ++i){
         permlib::Permutation::ptr gen(new permlib::Permutation(degree, cyc_not[i]));
         gens.push_back(gen);
         //fill Array with parsed generators
         parsed_generators[i]=perm2Array(gen);
      }
      boost::shared_ptr<permlib::PermutationGroup> group=permlib::construct(degree,gens.begin(),gens.end());
      return PermlibGroup(group);
   }


   permlib::dset lex_min_representative(const permlib::dset& dGamma) const
   {
      return smallestSetImage(*permlib_group, dGamma);
   }

   template<typename SetType>
   SetType lex_min_representative(const SetType& dGamma) const
   {
      return SetType(smallestSetImage(*permlib_group, dGamma.dset()));
   }

   Set<int> lex_min_representative(const Set<int>& input_rep) const
   {
      permlib::dset dGamma(permlib_group->n);
      for (Entire< Set<int> >::const_iterator sit = entire(input_rep); !sit.at_end(); ++sit)
         dGamma.set(*sit);
      Set<int> rep;
      const permlib::dset dGammaLeast = smallestSetImage(*permlib_group, dGamma);
      for (unsigned int i=0; i < permlib_group->n; ++i)
         if (dGammaLeast[i])
            rep += int(i);
      return rep;      
   }

};

  //template<typename Domain>
class PermlibGroupOfCone : public PermlibGroup {
public:
  PermlibGroupOfCone() 
   {};
  PermlibGroupOfCone(const Array<Array<int> >& generators) 
  : PermlibGroup(generators)
   {}

};


PermlibGroup group_from_perlgroup(perl::Object group);
perl::Object correct_group_from_permlib_group(perl::Object group, const PermlibGroup& perm_group);

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


template <typename Container> inline
Set<Container> orbit_impl (const PermlibGroup& sym_group, const Container& c)
{
   typedef permlib::Permutation PERM;
   permlib::OrbitSet<PERM, Container> orbit;
   orbit.orbit(c, sym_group.get_permlib_group()->S, permlib::ContainerAction<PERM, Container>());
   return Set<Container>(orbit.begin(), orbit.end());
}


template<typename _Matrix, typename Scalar>
std::pair< ListMatrix< Vector<Scalar> > , Array< Set<int> > >  orbits_coord_action_complete_sub(perl::Object group,const GenericMatrix<_Matrix, Scalar>& mat){
   PermlibGroup group_of_cone = group_from_perlgroup(group);
  
 
   if (mat.rows() == 0) 
       return std::pair< ListMatrix< Vector<Scalar> > , Array< Set<int> > >();

   if (mat.cols() <= group_of_cone.degree())
      throw std::runtime_error("orbits_coord_action_complete_sub: group/matrix dimension mismatch: group degree greather than #(number of matrix columns)-1");

   typename std::list< Vector<Scalar> > vectors;
   for (typename Entire< Rows< _Matrix > >::const_iterator vec=entire(rows(mat.top())); !vec.at_end(); ++vec){
      vectors.push_back(*vec);
   }

   typename std::list<boost::shared_ptr<permlib::OrbitSet<permlib::Permutation,Vector<Scalar> > > > o = permlib::orbits<Vector<Scalar>, CoordinateAction<permlib::Permutation,Scalar> >(*(group_of_cone.get_permlib_group()), vectors.begin(), vectors.end());

   Array< Set<int> > coord_act_orbits(o.size()); 
   ListMatrix< Vector<Scalar> > all_orbit_vectors;
   typename std::list<boost::shared_ptr<permlib::OrbitSet<permlib::Permutation,Vector<Scalar> > > >::const_iterator orbit;
   int orbit_count=0;
   int vec_count=0;
   for (orbit=o.begin(); orbit != o.end(); ++orbit){
      Set<int> one_orbit;
      typename permlib::OrbitSet<permlib::Permutation,Vector<Scalar> >::const_iterator orb_it; 
      for (orb_it=(*orbit)->begin();orb_it!=(*orbit)->end();++orb_it){
         Vector<Scalar> vec_in_orbit=*orb_it; //one vector contained in the orbit
         all_orbit_vectors/=vec_in_orbit; //vectors of one orbit form one block of the matrix all_orbit_vectors	  
         one_orbit+=vec_count;
         ++vec_count;
      }
      coord_act_orbits[orbit_count]=one_orbit;
      ++orbit_count;
   }
   return std::pair< ListMatrix< Vector<Scalar> > , Array< Set<int> > >(all_orbit_vectors,coord_act_orbits);
}


} // end namespace group
} // end namespace polymake

#endif // POLYMAKE_PERMLIB_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
