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

#include "polymake/client.h"
#include "polymake/group/permlib.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/hash_map"

#include "polymake/group/permlib_helpers.h"

#include "permlib/export/bsgs_schreier_export.h"
#include "permlib/generator/bsgs_generator.h"

namespace polymake { namespace group {

   void perlgroup_from_group(const PermlibGroup& permlib_group, perl::Object group) {
      using namespace permlib::exports;
      BSGSSchreierExport exp;
      BSGSSchreierData* data = exp.exportData(*permlib_group.get_permlib_group());
      
      Array<Array<int> > transversals = arrays2PolymakeArray(data->transversals, data->baseSize, data->n);
      Array<Array<int> > sgs = arrays2PolymakeArray(data->sgs, data->sgsSize, data->n);
      Array<int> base = array2PolymakeArray(data->base, data->baseSize);
      delete data;
      
      group.take("STRONG_GENERATORS") << sgs;
      group.take("BASE") << base;
      group.take("TRANSVERSALS") << transversals;
   }
   
   void perlgroup_from_generators(const Array< Array<int> >& generators, perl::Object group) {
      PermlibGroup permlib_group(generators);
      perlgroup_from_group(permlib_group, group);
   }

   PermlibGroup group_from_perlgroup(perl::Object group) {
      using namespace permlib::exports;

      Array<int> base;
      Array<Array<int> > sgs;
      Array<Array<int> > transversals;
      int n = 0;
      
      // If group has a BSGS in perl, use it for C++.
      // Otherwise, compute BSGS from scratch (ie. from GENERATORS).
      if ((group.lookup("BASE") >> base) && 
          (group.lookup("STRONG_GENERATORS") >> sgs) &&
          (group.lookup("TRANSVERSALS") >> transversals))
      {
         if ( !(group.lookup("DEGREE") >> n) ) {
            if (sgs.size() > 0) {
               n = sgs[0].size();
            } else {
               throw std::runtime_error("group_from_perlgroup: could not compute DEGREE for trivial group");
            }
         }
         
         BSGSSchreierImport imp;
         BSGSSchreierData data;

         data.n = n;
         data.baseSize = base.size();
         data.sgsSize = sgs.size();
         
         data.base = polymakeArray2Array<permlib::dom_int>(base);
         data.sgs = polymakeArray2Arrays<permlib::dom_int>(sgs);
         data.transversals = polymakeArray2Arrays<int>(transversals);
         
         boost::shared_ptr<permlib::PermutationGroup> perm_group(imp.importData(&data));
         return PermlibGroup(perm_group);
      }
      else
      {
         Array<Array<int> > generators;
         group.give("GENERATORS") >> generators;
         return PermlibGroup(generators);
      }
   }


/* helpers */  

    perl::Object group_from_permlib_cyclic_notation(const Array<std::string>& cyc_not, int degree){
      Array< Array<int> > parsed_generators;
      PermlibGroup permlib_group = PermlibGroup::permgroup_from_cyclic_notation(cyc_not,degree,parsed_generators);
      perl::Object group("Group");
      perlgroup_from_group(permlib_group, group);
      group.take("GENERATORS") << parsed_generators;      
      group.take("DEGREE") << degree;
      return group;
    }
    
    std::string group_to_cyclic_notation(perl::Object group){
       Array<Array<int> > generators;
       group.give("GENERATORS") >> generators;
       std::stringstream ss;
       int count = generators.size()-1;
       for (Entire< Array< Array <int> > >::const_iterator perm = entire(generators); !perm.at_end(); ++perm){
          boost::scoped_ptr<permlib::Permutation> gen(new permlib::Permutation((*perm).begin(),(*perm).end()));
          ss << *gen;
          if (count > 0)
             ss << ",\n";
          --count;
       }
       if (generators.size() == 0) {
          ss << "()";
       }
       return ss.str();
    }

    perl::Object correct_group_from_permlib_group(perl::Object group, const PermlibGroup& permlib_group) {
      perl::Object correct_group(group.type());
      if (group.type().isa("group::GroupOfCone")||group.type().isa("group::GroupOfPolytope")){
         correct_group.take("DOMAIN") << group.give("DOMAIN");
      }
      perlgroup_from_group(permlib_group, correct_group);
      correct_group.take("GENERATORS") << permlib_group.strong_gens();
      correct_group.set_name(group.name());
      correct_group.set_description(group.description());
      return correct_group;
    }





/*orbit computations*/    

   //action on DOMAIN
   //constructor for permlib::PermutationGroup(Transversals,Base,SG) would be good!!!
   Array< Set<int> > orbits_of_domain(perl::Object group){
      PermlibGroup group_of_cone = group_from_perlgroup(group);
      return group_of_cone.orbits();
   }

   template<class PERM>
   struct SetOfIndicesAction {
      Set<int> operator()(const PERM& p, const Set<int>& s) {
         Set<int> ret;
         for( Entire< Set<int> >::const_iterator index = entire(s); !index.at_end(); ++index) {
            ret+=p.at(*index);
         }
         return ret;
      }
   };
  
    //action on set of domain indices
   Array< Set<int> > orbits_induced_action(perl::Object group,const IncidenceMatrix<>& inc_mat){
      PermlibGroup group_of_cone = group_from_perlgroup(group);

      hash_map<Set<int>, int> inc_mat_rows; //to look up the index in inc_mat belonging to the set orbit_set
      std::list<Set<int> > sets_of_indices;
      for (Entire< Rows< IncidenceMatrix<> > >::const_iterator set=entire(rows(inc_mat)); !set.at_end(); ++set){	
         sets_of_indices.push_back(*set);
         inc_mat_rows[*set]=set->index();
      }
      std::list<boost::shared_ptr<permlib::OrbitSet<permlib::Permutation,Set<int> > > > o = permlib::orbits<Set<int>, SetOfIndicesAction<permlib::Permutation> >(*(group_of_cone.get_permlib_group()), sets_of_indices.begin(), sets_of_indices.end());

      Array< Set<int> > induced_orbits(o.size()); 
      std::list<boost::shared_ptr<permlib::OrbitSet<permlib::Permutation,Set<int> > > >::const_iterator orbit;
      int count=0;
      for (orbit=o.begin(); orbit != o.end(); ++orbit){
         Set<int> one_orbit;
         permlib::OrbitSet<permlib::Permutation,Set<int> >::const_iterator orb_it; 
         for (orb_it=(*orbit)->begin();orb_it!=(*orbit)->end();++orb_it){
            Set<int> orbit_set=*orb_it; //one set contained in the orbit	  
            one_orbit+=inc_mat_rows[orbit_set];
         }
         induced_orbits[count]=one_orbit;
         ++count;
      }    

      return induced_orbits;      
   }



    //FIXME: almost the same as orbits_induced_action, maybe join to one function? too many templates...
   template <typename MatrixTop, typename Scalar>
   Array< Set<int> > orbit_coord_action(perl::Object group,const GenericMatrix<MatrixTop, Scalar>& mat){
      PermlibGroup group_of_cone = group_from_perlgroup(group);
      
      if (mat.cols() <= group_of_cone.degree())
         throw std::runtime_error("orbit_coord_action: group/matrix dimension mismatch: group degree greater than #(number of matrix columns)-1");

      hash_map<Vector<Scalar>, int> mat_rows; //to look up the index in mat belonging to the image of vec
      typename std::list< Vector<Scalar> > vectors;
      int i=0;
      for (typename Entire< Rows<MatrixTop> >::const_iterator vec=entire(rows(mat.top())); !vec.at_end(); ++vec){
         vectors.push_back(*vec);
         //mat_rows[*vec]=vec->index();
         mat_rows[*vec]=i;
         ++i;
      }
      typename std::list<boost::shared_ptr<permlib::OrbitSet<permlib::Permutation,Vector<Scalar> > > > o = permlib::orbits<Vector<Scalar>, CoordinateAction<permlib::Permutation,Scalar> >(*(group_of_cone.get_permlib_group()), vectors.begin(), vectors.end());
      Array< Set<int> > coord_act_orbits(o.size()); 
      typename std::list<boost::shared_ptr<permlib::OrbitSet<permlib::Permutation,Vector<Scalar> > > >::const_iterator orbit;
      int count=0;
      for (orbit=o.begin(); orbit != o.end(); ++orbit){
         Set<int> one_orbit;
         typename permlib::OrbitSet<permlib::Permutation,Vector<Scalar> >::const_iterator orb_it; 
         for (orb_it=(*orbit)->begin();orb_it!=(*orbit)->end();++orb_it){
            Vector<Scalar> vec_in_orbit=*orb_it; //one vector contained in the orbit	  
            one_orbit+=mat_rows[vec_in_orbit];
         }
         coord_act_orbits[count]=one_orbit;
         ++count;
      }    

      return coord_act_orbits;  
    }


    //wrapper for orbits_coord_action_complete_sub, returns ListReturn instead of Pair
    template <typename MatrixTop, typename Scalar>
    perl::ListReturn orbits_coord_action_complete(perl::Object group,const GenericMatrix<MatrixTop, Scalar>& mat)
    {
      const std::pair< ListMatrix< Vector<Scalar> > , Array< Set<int> > > sub_result(orbits_coord_action_complete_sub(group,mat));
      perl::ListReturn result;
      result << sub_result.first
             << sub_result.second;
      return result;  
    }


    //test whether one vector is in the orbit of another vector (coordinate action)
    template <typename Scalar>
    bool are_in_same_orbit(perl::Object group, const Vector<Scalar>& vec1, const Vector<Scalar>& vec2) {
      typedef permlib::OrbitSet<permlib::Permutation,Vector<Scalar> > VecOrbit;

      bool answer=false;
      PermlibGroup group_of_cone = group_from_perlgroup(group);
      boost::shared_ptr<VecOrbit> o(new VecOrbit());
      
      if (vec1.size() <= group_of_cone.degree() || vec2.size() <= group_of_cone.degree())
         throw std::runtime_error("are_in_same_orbit: the dimension of the vectors must be equal to the degree of the group!");

      //orbit computation
      o->orbit(vec2, group_of_cone.get_permlib_group()->S, CoordinateAction<permlib::Permutation,Scalar>());

      for (typename VecOrbit::const_iterator orb_it=o->begin(); orb_it!=o->end(); ++orb_it) {
         if (*orb_it == vec1){
            answer=true;
            break;
         }
      }
      return answer;
    }


    // compute all group elements
    Array<Array<int> > all_group_elements( perl::Object group ) {
      using namespace permlib;
      
      std::list< Array<int> > all_elements;
      PermlibGroup perm_group = group_from_perlgroup(group);
      BSGSGenerator<TRANSVERSAL> bsgsGen(perm_group.get_permlib_group()->U);
      while (bsgsGen.hasNext()) {
	const PERMUTATION p = bsgsGen.next();
	Array<int> perm = PermlibGroup::perm2Array(p);
	all_elements.push_back(perm);
      }
      Array<Array<int> > all_elements_array(all_elements);
      return all_elements_array;
    } 


/* stabilizer computations */    

    perl::Object stabilizer_of_set (perl::Object group, const Set<int>& set) {
      PermlibGroup permlib_group = group_from_perlgroup(group);
      PermlibGroup permlib_stab(permlib_group.setwise_stabilizer(set));
      perl::Object stab = correct_group_from_permlib_group(group,permlib_stab);
      stab.set_name("set stabilizer");
      stab.set_description() << "Stabilizer of " << set << endl;
      return stab;
    }

    template< typename Scalar >
    perl::Object stabilizer_of_vector (perl::Object group, const Vector<Scalar>& vec) {
      int deg=group.give("DEGREE");
      if(deg!=vec.size()-1){
         throw std::runtime_error("stabilizer_of_vector: the dimension of the vector must be equal to the degree of the group!");
      }
      PermlibGroup permlib_group = group_from_perlgroup(group);
      PermlibGroup permlib_stab(permlib_group.vector_stabilizer(vec));
      perl::Object stab = correct_group_from_permlib_group(group,permlib_stab);
      stab.set_name("vector stabilizer");
      stab.set_description() << "Stabilizer of " << vec << endl;
      return stab;
    }


/****************************************************************
user functions
****************************************************************/

UserFunction4perl("# @category Basics"
                  "# Computes the basic properties 'base', "
                  "# 'strong generators', and 'transversals' "
                  "# and stores the result in a Group object."
                  "# @param Array gens some generators of the group"
                  "# @param Group group to fill in the data",
                  &perlgroup_from_generators, "group_from_generators(Array Group)");


/*orbit computations*/  

UserFunction4perl("# @category Orbits"
                  "# Computes the orbits of the basic set under //group//. "
                  "# @param Group group a group of a cone"
                  "# @return Array ",
                  &orbits_of_domain,"orbits_of_domain(Group)");


UserFunction4perl("# @category Orbits"
                  "# Computes the orbits of a set on which an action is induced."
                  "# The incidences between the domain elements and the elements"
                  "# in the set are given by an incidence matrix //inc//."
                  "# @param Group group a group of a cone"
                  "# @param IncidenceMatrix inc the incidences between domain elements"
                  "#    and elements on which an action is induced"
                  "# @return Array an array of the orbits of the induced action",
                  &orbits_induced_action,"orbits_induced_action(Group,IncidenceMatrix)");


UserFunctionTemplate4perl("# @category Orbits"
            "# Computes the orbits of the vectors (homogenized) of a matrix //mat// by"
            "# permuting the coordinates of the vectors (skipping the homogenizing coordinate)."
            "# The group must act on the set of vectors. Choose the function"
            "# 'orbits_coord_action_complete' if your set is not complete."
            "# @param Group group a group acting on the cone by permuting the coordinates"
            "# @param Matrix<Scalar> mat a matrix with vectors on which the group acts by "
            "#    coordinate permutation"
            "# @return Array an array of the orbits under the action on the coordinates",
            "orbit_coord_action(Group,Matrix)");


UserFunctionTemplate4perl("# @category Orbits"
            "# Computes the orbit of the set of all vectors of the matrix //mat//"
            "# under //group//, which acts by permuting coordinates."
            "# The set of vectors does not have to be complete."
            "# @param Group group a group of coordinate permutations"
            "# @param Matrix<Scalar> mat some input vectors"
            "# @return ListReturn a matrix containing all generated vectors"
            "#    , and an array containing the orbits of generated vectors.",
            "orbits_coord_action_complete(Group,Matrix)");


UserFunctionTemplate4perl("# @category Basics"
            "# Compute all elements of a given //group//."
            "# @param Group group the permutation group"
            "# @return Array an array containing all group elements ",
            "all_group_elements(Group)");


UserFunctionTemplate4perl("# @category Orbits"
            "# Checks whether vector //vec1// and //vec2// are in the same orbit"
            "# with respect to the (coordinate) action of //group//." 
            "# @param Group group the permutation group acting on coordinates"
            "# @param Vector vec1 the first vector"
            "# @param Vector vec2 the second vector"
            "# @return bool true, if //vec1// and //vec2// are in the same orbit, false otherwise",
            "are_in_same_orbit(Group,Vector,Vector)");

/*stabilizer computations*/  

UserFunction4perl("# @category Stabilizers"
                  "# Computes the subgroup of //group// which stabilizes"
                  "# the given set of indices //set//."
                  "# @param Group group a permutation group"
                  "# @param Set set the set to be stabilized"
                  "# @return Group the stabilizer of //set// w.r.t. //group//",
                  &stabilizer_of_set,"stabilizer_of_set(Group,Set)");


UserFunctionTemplate4perl("# @category Stabilizers"
            "# Computes the subgroup of //group// which stabilizes"
            "# the given vector //vec//."
            "# @param Group group a permutation group"
            "# @param Vector vec the vector to be stabilized"
            "# @return Group the stabilizer of //vec// w.r.t. //group//",
            "stabilizer_of_vector(Group,Vector)");


/*group generation from cyclic notation*/  

// group_from_cyclic_notation is realized by a user function in perl

UserFunction4perl("# @category Utilities"
                  "# Constructs a Group from generators given in permlib cyclic notation."
                  "# (permlib cyc_not: indices separated by whitespace, generators separated by commas)"
                  "# @param Array gens generators of the permutation group in permlib cyclic notation"
                  "# @param Int degree the degree of the permutation group"
                  "# @return Group the group generated by //gens//",
                  &group_from_permlib_cyclic_notation,"group_from_permlib_cyclic_notation(Array $)");

UserFunction4perl("# @category Utilities"
                  "# Returns group generators in 1-based cyclic notation"
                  "# (GAP like, not permlib like notation)"
                  "# @param Group g the permutation group"
                  "# @return String group generators, separated by newline and comma",
                  &group_to_cyclic_notation,"group_to_cyclic_notation(Group)");

}
}


