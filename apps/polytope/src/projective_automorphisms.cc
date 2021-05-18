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
#include "polymake/linalg.h"
#include "polymake/matrix_linalg.h"
#include "polymake/group/action.h"
#include "polymake/group/named_groups.h"
#include "polymake/Graph.h"


namespace polymake { namespace polytope { 

// This implementation for finding the group of projective 
// automorphism of a cone follows the algorithm implizit 
// described in the paper:
//
// Computing symmetry groups of polyhedra, LMS J. Comput. Math.
// 17 (1) (2004)
// by David Bremner, Mathieu Dutour Sikiri'{c}, 
// Dmitrii V. Pasechnik, Thomas Rehn and Achill Sch"{u}rmann
// https://doi.org/10.1112/S1461157014000400
// 
// most of the functions defined here belong to a theorem of the
// paper.


template<typename Scalar>
auto  projective_symmetry_group_non_decomposable(BigObject C){
  // This function calculate the projective symmetry group of a
  // fulldimensional (by now) and non decomposable cone
  // It use theorem 10 of the paper 'Computing symmetry groups 
  // of polyhedra'
  Matrix<Scalar> Rays = C.give("RAYS");
  Rays  = T(Rays);
  Int p = Rays.cols(); // the number of Rays
  Int n = Rays.rows(); // the lenght of the Rays and size of a basis
  
  // get a basis of the rays
  Set<Int> basis_rows = basis(Rays).second;
  Array<Int> basis_array(n);
  Int counter = 0;
  for(Int i=0; counter <n; i++){
    if(basis_rows.contains(i)){
      basis_array[counter] = i;
      counter += 1;
    }
  }
  
  Matrix<Scalar> Basis_Rays = Rays.minor(All,basis_rows);
  
  Matrix<Scalar> Coef = solve_right(Basis_Rays,Rays);
  
  BigObject gro = call_function("group::automorphism_group", C.give("RAYS_IN_FACETS"), false); 
  //group::symmetric_group(p);
  BigObject perm = gro.give("PERMUTATION_ACTION");
  
  // load a larger group to find the projective symmetry group
  Array<Array<Int>> comb_group = perm.give("ALL_GROUP_ELEMENTS");
  Set<Array<Int>> proj_group;
  
  for(Int index= 0; index<comb_group.size(); ++index){
    Array<Int> sigma =  comb_group[index];
    Matrix<Scalar> Rays_sigma = group::action<group::on_cols>(sigma, Rays);
    Matrix<Scalar> Pos = zero_vector<Scalar>(p)|unit_matrix<Scalar>(p);
    
    
    // Constructing the equations
    Matrix<Scalar> U_sigma = Rays_sigma.minor(All,basis_rows);
    Matrix<Scalar> Equa = zero_matrix<Scalar>( p*n, p+1 );
    for(Int i = 0; i<n; i++){
      for(Int j = 0; j<p; j++){
        Vector<Scalar> v_1 = U_sigma.row(i);
        Vector<Scalar> v_2 = Coef.col(j);
        
        for(Int k = 0; k<n; ++k){
          Equa(i*p+j,k+1) = v_1[k]*v_2[k];
        }
        
        Equa(i*p+j,j+1) -= Rays_sigma(i,j); 
      }
    }
    
    BigObject poly1("Polytope", mlist<Scalar>());
    poly1.take("INEQUALITIES") << Pos;
    poly1.take("EQUATIONS") << Equa.minor(basis(Equa).first,All);
    
    if(!poly1.give("FEASIBLE")){
      continue;
    }
    
    // now check if there is a convex combination of the 
    // vertices strictly greater 0 and this exist if 
    // there is no index such that all vertices are zero 
    // in this entry
    bool none_zero_entry = true;
    Matrix<Scalar> Vertices = poly1.give("VERTICES");
    
    for(Int i=1; i<Vertices.cols(); i++){
      Int j=0;
      while(is_zero(Vertices(j,i))){
        j++;
        if(j>=Vertices.rows()){
          none_zero_entry = false;
          break;
        }
      }
      
      if(!none_zero_entry){
        break;
      }
    }
    
    if(none_zero_entry){
      proj_group += sigma;
    }
    
  }
  return proj_group;
  
}

template<typename Scalar>
auto compute_decomposition_projective_symmetry(BigObject C, bool full_dim){
  // compute a minimal decomposition of the cone in disjoined
  // subsets of the full dimensional cone C, such that rays 
  // such that the the linear hull of the cone is the direct 
  // sum of the linear hulls of the subsets
  // Here the bool full_dim say if the resulting cones should 
  // be project down to become full dimentional
  // It use Theorem 4 of the paper 'Computing symmetry groups 
  // of polyhedra'.
  
  // get the raise and finde a basis of them
  Matrix<Scalar> Rays = C.give("RAYS");
  Set<Int> basis_set = basis(Rays).first;
  Matrix<Scalar> Basis = Rays.minor(basis_set, All);
  
  // check if we are in a trivial case
  if(Rays.rows() == Basis.rows()){
    Int n = Rays.rows();
    Array<Int> Indices(n);
    Array<std::pair<BigObject,Array<Int>>> NewCones(1);
    NewCones[0].first = C;
    for(Int i = 0; i<n; ++i){
      Indices[i] = i;
    }
    NewCones[0].second = Indices;
    return NewCones;
  }
  
  // save the possitions of the basis rays in array
  Array<Int> basis_array(Rays.cols());
  Int l = 0;
  for(Int i=0; i<Rays.rows(); ++i){
    if(basis_set.contains(i)){
      basis_array[l] = i;
      l++;
    }
  }
  
  // define a graph over the rays
  Graph<Undirected> g(Rays.rows());
  
  // edges here are given the use of the basis rays in a 
  // description over the other rays
  Matrix<Scalar> Coeff = solve_left(Basis, Rays);
  for(Int i=0; i<Rays.rows(); ++i){
    // check if i-th ray is in the basis
    if(basis_set.contains(i)){continue;}
    // identify the basis rays that defines the i-th ray
    Vector<Scalar> lambda = Coeff.row(i);
    for(Int j = 0; j<basis_array.size(); ++j){
      if(!is_zero(lambda[j])){
        g.edge(i, basis_array[j]);
      }
    }
  }
  BigObject G("Graph<>");
  G.take("ADJACENCY") << g;
  
  // get the connected componets of the graph
  IncidenceMatrix<NonSymmetric> Components = G.give("CONNECTED_COMPONENTS");
  
  Int n_components = G.give("N_CONNECTED_COMPONENTS");
  
  Array<std::pair<BigObject,Array<Int>>> NewCones(n_components);
  
  // get the decomposition from the connected components
  for(Int i=0; i<n_components; ++i){
    Set<Int> new_rays = Components.row(i);
    Array<Int> new_ray_indices(Components.row(i));
    BigObject new_cone("Cone", mlist<Scalar>());
    new_cone.take("RAYS") << Rays.minor(new_rays,All);
    if( full_dim ){
      NewCones[i] = std::make_pair(call_function("project_full", new_cone), new_ray_indices);
    }else{
      NewCones[i] = std::make_pair(new_cone, new_ray_indices);
    }
  }
  return NewCones;
}

template<typename Scalar>
auto matrix_equation_feasible(Matrix<Scalar> A, Matrix<Scalar> B){
  // checks if there is a matrix X that solves the equation A * X = B
  
  if(A.rows() != B.rows()){
    return false;
  }
  
  Matrix<Scalar> Equa( B.rows()*B.cols(), A.cols()*B.cols() + 1);
  
  for(Int i=0; i<B.rows(); ++i){
    for(Int j=0; j<B.cols(); ++j){
      Equa( i*B.rows() + j ,0) = B(i,j);
      for(Int k=0; k<A.cols(); ++k){
        Equa( i*B.rows() + j , j*B.cols() + k + 1) = A(i,k);
      }
    }
  }
  
  Vector<Scalar> Inqu(A.cols()*B.cols() + 1);
  Inqu[0] = 1;
  
  BigObject p_new("Polytope", mlist<Scalar>());
  p_new.take("INEQUALITIES") << Inqu;
  p_new.take("EQUATIONS") << Equa.minor(basis(Equa).first,All);
  
  bool feasible = p_new.give("FEASIBLE"); // TODO returning the function value direcly
  return feasible;
  
}

template<typename Scalar>
auto projective_isomorphism(BigObject C1, BigObject C2){
  // This function checks if there is a bijective linear map 
  // between the two cones.
  
  Matrix<Scalar> Rays1 = C1.give("RAYS");
  Matrix<Scalar> Rays2 = C2.give("RAYS");
  
  Array<Int> empty_array;
  
  if(Rays1.rows() != Rays2.rows()){
    return std::make_pair(false, empty_array);
  }
  
  BigObject gro = group::symmetric_group(Rays1.rows());
  BigObject perm = gro.give("PERMUTATION_ACTION");
  
  Array<Array<Int>> symm_group = perm.give("ALL_GROUP_ELEMENTS");
  
  for(Int index= 0; index<symm_group.size(); ++index){
    Array<Int> sigma =  symm_group[index];
    
    Matrix<Scalar> Rays2_sigma = group::action<group::on_rows>( sigma, Rays2 );
    
    if(matrix_equation_feasible(Rays1,Rays2_sigma)){
      return std::make_pair(true, sigma);
    }
  }
  return std::make_pair(false, empty_array);
  
  
}

template<typename Scalar>
auto projective_symmetries(BigObject C){
  // This function calculates the projectiv automorphism group of
  // a full dimensional cone based of Theorem 7 from the paper
  // 'Computing symmetry groups of polyhedra'. 
  
  // get the decomposition
  Array< std::pair<BigObject, Array<Int> > > Decomposition = compute_decomposition_projective_symmetry<Scalar>(C, true);
  Int len = Decomposition.size();
  Int repr_nr = 0;
  
  // TODO find a better solution for the len
  Array< std::pair< BigObject, Set< Array<Int> > > > Represent(len);
  
  for(Int i=0; i<len; i++){
    
    // get the cones in the decomposition and assume that there of
    // a new type
    std::pair<BigObject, Array<Int> > SubC = Decomposition[i];
    bool newC = true;
    
    for(Int j=0; j<repr_nr; ++j){
      // check if the isomorphic type is known
      BigObject RepC = Represent[j].first;
      
      std::pair<bool, Array<Int> > isom2RepC = projective_isomorphism<Scalar>(SubC.first, RepC);
      if(isom2RepC.first){
        Represent[j].second += group::action<group::on_container>(isom2RepC.second, SubC.second);
        newC = false;
        break;
      }
    }
    if(newC){
      Set< Array<Int> > set;
      set += SubC.second;
      Represent[repr_nr] = std::make_pair(SubC.first, set);
      repr_nr += 1;
    }
  }
  
  // build an array that contains the index sets of all
  // pair< projective group, index sets cones >
  Array< std::pair< Array<Array<Int>>, Array<Array<Int>> > > small_proj_groups(repr_nr);
  
  for(Int i=0; i<repr_nr; ++i){
    small_proj_groups[i].second = Represent[i].second;
    small_proj_groups[i].first = projective_symmetry_group_non_decomposable<Scalar>(Represent[i].first);
  }
  
  // now we build our projective group
  Set<Array<Int>> projective_group;
  
  // define counter for the great merge
  Array<std::pair<Int, Array<Int>>> counter(small_proj_groups.size());
  
  Int amount = 1;
  
  // here we need the symmetric group on all projective isomorph sets
  Array< Array< Array<Int>>> symmertic_group(small_proj_groups.size());
  for(Int i=0; i<small_proj_groups.size(); ++i){
    BigObject gro = group::symmetric_group( small_proj_groups[i].second.size() );
    BigObject perm = gro.give("PERMUTATION_ACTION");
  
    Array<Array<Int>> this_group= perm.give("ALL_GROUP_ELEMENTS");
    symmertic_group[i] = this_group;
    
    amount *= symmertic_group[i].size();
    amount *= small_proj_groups[i].first.size();
    amount *= small_proj_groups[i].second.size();
    
    // set counter
    counter[i].first = 0;
    Array<Int> subcounter(small_proj_groups[i].second.size());
    for(Int j = 0; j<small_proj_groups[i].second.size(); ++j){
      subcounter[j] = 0;
    }
    counter[i].second = subcounter;
  }
  
  Int n = C.give("N_RAYS");
  Array<Array<Int>> proj_group(amount);
  
  for(Int index=0; index<amount; ++index){
    Array<Int> perm(n);
    // iterate over the groups of projective isomorphic cones
    for(Int i=0; i<repr_nr; ++i){
      // take an element from symmetry group
      Array<Int> sigma = symmertic_group[i][counter[i].first];
      // iterate over the projective isomorphic cones of the current group
      for(Int j=0; j<small_proj_groups[i].second.size(); ++j){
        // sigma say on which cone we project
        Array<Int> places = small_proj_groups[i].second[j];
        Array<Int> values = small_proj_groups[i].second[sigma[j]];
        values = group::action<group::on_container>(
        small_proj_groups[i].first[counter[i].second[j]],
        values);
        
        for(Int k=0; k<places.size(); ++k){
          perm[places[k]] = values[k];
        }
        
      }
    }
    
    proj_group[index]=perm;
    // update index (check if it need to be updated)
    bool updated=(index==amount-1);
    Int group_nr = 0;
    Int elem_nr = 0;
    while(!updated){
      if(counter[group_nr].second[elem_nr] < small_proj_groups[group_nr].first.size()-1){
        counter[group_nr].second[elem_nr] +=1;
        break;
      }else if(elem_nr < small_proj_groups[group_nr].second.size()-1){
        counter[group_nr].second[elem_nr] =0;
        elem_nr += 1;
      }else{
        counter[group_nr].second[elem_nr] =0;
        elem_nr = 0;
        group_nr +=1;
      }
    }
  }
  
  return proj_group;
  
}

/*
UserFunctionTemplate4perl("# @category Geometry"
                          "# Find the decomposition of a cone in  group of projective automorphisms of a"
                          "# Cone //C//."
                          "# This is an implementation of the algorithm described in a"
                          "# theorem of the paper \"Computing symmetry groups of polyhedra\""
                          "# LMS J. Comput. Math. 17 (1) (2004)"
                          "# by By David Bremner, Mathieu Dutour Sikiri'{c},"
                          "# Dmitrii V. Pasechnik, Thomas Rehn and Achill Sch\"{u}rmann."
                          "# @param Cone C"
                          "# @return Array<Pair <Cone<Scalar>, Array<Int>> >",
                          "compute_decomposition_projective_symmetry<Scalar>(Cone<Scalar>, Bool)");
*/
UserFunctionTemplate4perl("# @category Geometry"
                          "# Find the group of projective automorphisms of a"
                          "# Cone //C//. This is a group of all permutations on the"
                          "# rays of the cone (not necessarily there representatives),"
                          "# such that there is a invertible matrix //A// with"
                          "# A*Ray = Ray_sigma for all rays of the cone."
                          "# This is an implementation of the algorithm described in"
                          "# the paper \"Computing symmetry groups of polyhedra\""
                          "# LMS J. Comput. Math. 17 (1) (2004)"
                          "# by By David Bremner, Mathieu Dutour Sikiri'{c},"
                          "# Dmitrii V. Pasechnik, Thomas Rehn and Achill Sch\"{u}rmann."
                          "# @param Cone C"
                          "# @return Array<Array<Int>>"
                          "# @example"
                          "# > $C = cube(2);"
                          "# > print projective_symmetries($C);"
                          "# | 0 1 2 3"
                          "# | 0 2 1 3"
                          "# | 1 0 3 2"
                          "# | 1 3 0 2"
                          "# | 2 0 3 1"
                          "# | 2 3 0 1"
                          "# | 3 1 2 0"
                          "# | 3 2 1 0",
                          "projective_symmetries<Scalar>(Cone<Scalar>)");



} } // end namespaces
