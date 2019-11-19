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

#include "polymake/client.h"
#include "polymake/group/permlib.h"
#include "polymake/IncidenceMatrix.h"

#include "polymake/group/permlib_helpers.h"


namespace polymake { namespace group {

namespace {

void perl_action_from_group_impl(const PermlibGroup& permlib_group, 
                                 perl::Object action,
                                 const std::string& name,
                                 const std::string& description) 
{
   using namespace permlib::exports;
   BSGSSchreierExport exp;
   BSGSSchreierData* data = exp.exportData(*permlib_group.get_permlib_group());
      
   Array<Array<int>> transversals = arrays2PolymakeArray(data->transversals, data->baseSize, data->n);
   Array<Array<int>> sgs = arrays2PolymakeArray(data->sgs, data->sgsSize, data->n);
   Array<int> base = array2PolymakeArray(data->base, data->baseSize);
   delete data;

   action.take("STRONG_GENERATORS") << sgs;
   action.take("BASE") << base;
   action.take("TRANSVERSALS") << transversals;

   if (name.length())        action.set_name(name);
   if (description.length()) action.set_description() << description;
}

} // end anonymous namespace

Array<Array<int>>
generators_from_permlib_group(const PermlibGroup& permlib_group)
{
   using namespace permlib::exports;
   BSGSSchreierExport exp;
   BSGSSchreierData* data = exp.exportData(*permlib_group.get_permlib_group());
   Array<Array<int>> sgs = arrays2PolymakeArray(data->sgs, data->sgsSize, data->n);
   delete data;
   return sgs;
}

perl::Object perl_action_from_group(const PermlibGroup& permlib_group,
                                    const std::string& name,
                                    const std::string& description)
{
   perl::Object pa("group::PermutationAction");
   perl_action_from_group_impl(permlib_group, pa, name, description);
   return pa;
}

void perl_action_from_generators(const Array<Array<int>>& generators,
                                 perl::Object action,
                                 perl::OptionSet options)
{
   const std::string name = options["name"];
   const std::string description = options["description"];
   perl_action_from_group_impl(PermlibGroup(generators), action, name, description);
}


perl::Object perl_group_from_group(const PermlibGroup& permlib_group,
                                   const std::string& name,
                                   const std::string& description)
{
   perl::Object G("group::Group");
   G.take("PERMUTATION_ACTION") << perl_action_from_group(permlib_group, name, description);
   return G;
}
  


PermlibGroup group_from_perl_action(perl::Object action)
{
   using namespace permlib::exports;

   Array<int> base;
   Array<Array<int>> sgs;
   Array<Array<int>> transversals;
   int n = 0;
      
   // If group has a BSGS in perl, use it for C++.
   // Otherwise, compute BSGS from scratch (ie. from GENERATORS).
   if ((action.lookup("BASE") >> base) && 
       (action.lookup("STRONG_GENERATORS") >> sgs) &&
       (action.lookup("TRANSVERSALS") >> transversals))
      {
         if ( !(action.lookup("DEGREE") >> n) ) {
            if (sgs.size() > 0) {
               n = sgs[0].size();
            } else {
               throw std::runtime_error("group_from_perl_action: could not compute DEGREE for trivial group");
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
         Array<Array<int>> generators;
         action.give("STRONG_GENERATORS | GENERATORS") >> generators;
         return PermlibGroup(generators);
      }
}


/* helpers */  

perl::Object group_from_permlib_cyclic_notation(const Array<std::string>& cyc_not, int degree)
{
   Array<Array<int>> parsed_generators;
   const PermlibGroup permlib_group = PermlibGroup::permgroup_from_cyclic_notation(cyc_not, degree, parsed_generators);
   perl::Object action(perl_action_from_group(permlib_group));
   action.take("GENERATORS") << parsed_generators;      
   action.take("DEGREE") << degree;
   perl::Object G("Group");
   G.take("PERMUTATION_ACTION") << action;
   return G;
}
    
std::string action_to_cyclic_notation(perl::Object action)
{
   Array<Array<int>> generators;
   action.give("STRONG_GENERATORS | GENERATORS") >> generators;
   std::stringstream ss;
   int count = generators.size()-1;
   for (auto perm = entire(generators); !perm.at_end(); ++perm) {
      const permlib::Permutation gen(perm->begin(), perm->end());
      ss << gen;
      if (count > 0)
         ss << ",\n";
      --count;
   }
   if (generators.size() == 0) {
      ss << "()";
   }
   return ss.str();
}



/*orbit computations*/    

   //action on DOMAIN
   //constructor for permlib::PermutationGroup(Transversals,Base,SG) would be good!!!
Array<hash_set<int>> orbits_of_action(perl::Object action) {
   return group_from_perl_action(action).orbits();
}

namespace {  

/*
  The following function takes an action of a permutation group and a matrix as arguments, and 
  an action functional class as a template parameter.

  The permutation action acts on the columns of the matrix via the action functional class,
  and the following function calculates the orbits of the action that is induced on the rows.

  The main use cases are:

     DomainType =      Set<int>,
     DomainContainer = IncidenceMatrix<>,
     Action =          SetOfIndicesAction<permlib::Permutation>

  and

     DomainType =      Vector<Scalar>
     DomainContainer = GenericMatrix<MatrixTop, Scalar>
     Action =          CoordinateAction<permlib::Permutation,Scalar>
*/    
template<typename DomainType, typename DomainContainer, typename Action>
Array<hash_set<int>>
orbits_of_induced_action_impl(perl::Object action, const DomainContainer& container) {
   const PermlibGroup group_of_cone = group_from_perl_action(action);
   hash_map<DomainType, int> index_of;
   std::vector<DomainType> domain_list;
   domain_list.reserve(container.rows());
   int i(0);
   for (auto cit=entire(rows(container)); !cit.at_end(); ++cit, ++i) {	
      domain_list.push_back(*cit);
      index_of[*cit] = i;
   }

   const auto olist = permlib::orbits<DomainType, Action>(*(group_of_cone.get_permlib_group()), domain_list.begin(), domain_list.end());

   Array<hash_set<int>> induced_orbits(olist.size()); 
   auto ioit (entire(induced_orbits));
   for (auto orbit = olist.begin(); orbit != olist.end(); ++orbit, ++ioit) {
      hash_set<int> one_orbit;
      for (auto orb_it=(*orbit)->begin(); orb_it != (*orbit)->end(); ++orb_it) {
         one_orbit += index_of.at(*orb_it); // this will throw if *orb_it isn't in the orbit already, which means the action would be undefined
      }
      *ioit = one_orbit;
   }    

   return induced_orbits;      
}

} // end anonymous namespace


Array<hash_set<int>> orbits_of_induced_action_incidence(perl::Object action, const IncidenceMatrix<>& container) {
   return orbits_of_induced_action_impl<Set<int>, IncidenceMatrix<>, SetOfIndicesAction<permlib::Permutation>>(action, container);
}

template<typename Scalar, typename MatrixTop>
Array<hash_set<int>> orbits_of_coordinate_action(perl::Object action, const GenericMatrix<MatrixTop, Scalar>& container) {
   return orbits_of_induced_action_impl<Vector<Scalar>, MatrixTop, CoordinateAction<permlib::Permutation, Scalar>>(action, container.top());
}


// wrapper for orbits_in_orbit_order_impl, returns ListReturn instead of Pair
template <typename MatrixTop, typename Scalar>
perl::ListReturn orbits_in_orbit_order(perl::Object coordinate_action, const GenericMatrix<MatrixTop, Scalar>& mat)
{
   const auto sub_result(orbits_in_orbit_order_impl(coordinate_action, mat));
   perl::ListReturn result;
   result << sub_result.first
          << sub_result.second;
   return result;  
}


// test whether one vector is in the orbit of another vector (coordinate action)
template <typename Scalar>
bool are_in_same_orbit(perl::Object action, const Vector<Scalar>& vec1, const Vector<Scalar>& vec2) {
   typedef permlib::OrbitSet<permlib::Permutation,Vector<Scalar>> VecOrbit;
   boost::shared_ptr<VecOrbit> o(new VecOrbit());

   PermlibGroup group_of_cone = group_from_perl_action(action);
   if (vec1.size() <= group_of_cone.degree() || vec2.size() <= group_of_cone.degree())
      throw std::runtime_error("are_in_same_orbit: the dimension of the vectors must be equal to the degree of the group!");

   //orbit computation
   o->orbit(vec2, group_of_cone.get_permlib_group()->S, CoordinateAction<permlib::Permutation,Scalar>());

   for (auto orb_it=o->begin(); orb_it!=o->end(); ++orb_it) {
      if (*orb_it == vec1) {
         return true;
      }
   }
   return false;
}


// compute all group elements
Array<Array<int>>
all_group_elements(perl::Object action) {
   return Array<Array<int>>(all_group_elements_impl(group_from_perl_action(action)));
} 


/* stabilizer computations */    

perl::Object stabilizer_of_set (perl::Object action, const Set<int>& set) {
   PermlibGroup permlib_group = group_from_perl_action(action);
   PermlibGroup permlib_stab(permlib_group.setwise_stabilizer(set));
   perl::Object stab = perl_group_from_group(permlib_stab);
   stab.set_name("set stabilizer");
   stab.set_description() << "Stabilizer of " << set << endl;
   return stab;
}

template<typename Scalar>
perl::Object stabilizer_of_vector (perl::Object action, const Vector<Scalar>& vec) {
   const int deg = action.give("DEGREE");
   if (deg != vec.size()-1) {
      throw std::runtime_error("stabilizer_of_vector: the dimension of the vector must be equal to the degree of the group!");
   }
   PermlibGroup permlib_group = group_from_perl_action(action);
   PermlibGroup permlib_stab(permlib_group.vector_stabilizer(vec));
   perl::Object stab = perl_group_from_group(permlib_stab);
   stab.set_name("vector stabilizer");
   stab.set_description() << "Stabilizer of " << vec << endl;
   return stab;
}


/****************************************************************
user functions
****************************************************************/

UserFunction4perl("# @category Utilities"
                  "# Computes groups with a permutation action with the basic properties [[PermutationAction::BASE|BASE]], "
                  "# [[PermutationAction::STRONG_GENERATORS|STRONG_GENERATORS]], and [[PermutationAction::TRANSVERSALS|TRANSVERSALS]]."
                  "# @param Array<Array<Int>> gens some generators of the group"
                  "# @param Group action the generated action",
                  &perl_action_from_generators, "action_from_generators(Array<Array<Int>>, PermutationAction, { name=>'', description=>'action defined from generators' })");


/*orbit computations*/  

UserFunction4perl("# @category Orbits"
                  "# Computes the orbits of the basic set under //a//. "
                  "# @param PermutationAction a a permutation action of a group"
                  "# @return Array<Set<Int>>",
                  &orbits_of_action, "orbits_of_action(PermutationAction)");


UserFunction4perl("# @category Orbits"
                  "# Computes the orbits of a set on which an action is induced."
                  "# The incidences between the domain elements and the elements"
                  "# in the set are given by an incidence matrix //inc//."
                  "# @param PermutationAction a an action of a group"
                  "# @param IncidenceMatrix I the incidences between domain elements and elements on which an action is induced"
                  "# @return Array<Set<Int>> an array of the orbits of the induced action",
                  &orbits_of_induced_action_incidence, "orbits_of_induced_action(PermutationAction, IncidenceMatrix)");


UserFunctionTemplate4perl("# @category Orbits"
            "# Computes the orbits of the vectors (homogenized) of the rows of a matrix //M// by"
            "# permuting the coordinates of the vectors (skipping the homogenizing coordinate)."
            "# The group must act on the set of vectors, and the rows of the matrix must contain the entire orbit."
            "# @param PermutationAction a an action of a group acting by permuting the coordinates"
            "# @param Matrix<Scalar> M a matrix on whose columns the group acts by "
            "#    coordinate permutation"
            "# @return Array<Set<Int>> an array of the orbits under the action on the coordinates",
            "orbits_of_coordinate_action<Scalar>(PermutationAction, Matrix<Scalar>)");


UserFunctionTemplate4perl("# @category Orbits"
            "# Computes the orbit of the rows of the matrix //mat//"
            "# under the permutation action on coordinates //action//."
            "# @param PermutationAction a an action of a group of coordinate permutations"
            "# @param Matrix M some input vectors"
            "# @return List( Matrix generated vectors in orbit order, Array orbits of generated vectors)",
            "orbits_in_orbit_order(PermutationAction, Matrix)");


UserFunction4perl("# @category Utilities"
            "# Compute all elements of a given group, expressed as a permutation action."
            "# @param PermutationAction a the action of a permutation group"
            "# @return Array<Array<Int>> all group elements ",
             &all_group_elements,
            "all_group_elements(PermutationAction)");


UserFunctionTemplate4perl("# @category Orbits"
            "# Checks whether vectors //v1// and //v2// are in the same orbit"
            "# with respect to the (coordinate) action of //group//." 
            "# @param PermutationAction a the permutation group acting on coordinates"
            "# @param Vector v1"
            "# @param Vector v2"
            "# @return Bool",
            "are_in_same_orbit(PermutationAction, Vector, Vector)");

/*stabilizer computations*/  

UserFunction4perl("# @category Producing a group"
                  "# Computes the subgroup of //group// which stabilizes"
                  "# the given set of indices //set//."
                  "# @param PermutationAction a the action of a permutation group"
                  "# @param Set S the set to be stabilized"
                  "# @return Group the stabilizer of //S// w.r.t. //a//",
                  &stabilizer_of_set,"stabilizer_of_set(PermutationAction, Set)");


UserFunctionTemplate4perl("# @category Producing a group"
            "# Computes the subgroup of //G// which stabilizes the given vector //v//."
            "# @param PermutationAction a the action of a permutation group"
            "# @param Vector v the vector to be stabilized"
            "# @return Group the stabilizer of //v// w.r.t. //a//",
            "stabilizer_of_vector(PermutationAction, Vector)");


/*group generation from cyclic notation*/  

// group_from_cyclic_notation is realized by a user function in perl

UserFunction4perl("# @category Producing a group"
                  "# Constructs a Group from generators given in permlib cyclic notation,"
                  "# i.e., indices separated by whitespace, generators separated by commas."
                  "# @param Array<String> gens generators of the permutation group in permlib cyclic notation"
                  "# @param Int degree the degree of the permutation group"
                  "# @return Group the group generated by //gens//",
                  &group_from_permlib_cyclic_notation, "group_from_permlib_cyclic_notation(Array $)");

UserFunction4perl("# @category Utilities"
                  "# Returns group generators in 1-based cyclic notation"
                  "# (GAP like, not permlib like notation)"
                  "# @param PermutationAction a the action of the permutation group"
                  "# @return String group generators, separated by newline and comma",
                  &action_to_cyclic_notation, "action_to_cyclic_notation(PermutationAction)");

}
}



// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

