/* Copyright (c) 1997-2021
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
#include "polymake/Graph.h"
#include "polymake/Map.h"



namespace polymake { namespace polytope {

// This is a function to compute the linear symmetry group of a
// polytope or Matrix via the automorphisms group of a graph with
// edge colors.
//
// Computing symmetry groups of polyhedra, LMS J. Comput. Math.
// 17 (1) (2004)
// by David Bremner, Mathieu Dutour Sikiri'{c}, 
// Dmitrii V. Pasechnik, Thomas Rehn and Achill Sch"{u}rmann
// https://doi.org/10.1112/S1461157014000400
//
// To find this symmetry group we use a technic descripted in the 
// manual of nauty to compute it over the symmetry group of a vertex
// color graph.
//
template<typename Scalar>
auto linear_symmetries(const Matrix<Scalar>& Rays){
  // Take a matrix, which is maybe the VERTICES matrix of a Polytope,
  // als input.
  
  // catch cases with a single or non rows
  if(1>=Rays.rows()){
    Array<Array<Int>> generator(Rays.rows());
    
    if(1== Rays.rows()){
      generator[0] = {0};
    }
    
    // define the output
    BigObject Perm("group::PermutationAction");
    Perm.take("GENERATORS") << generator;
    BigObject Group("group::Group");
    Group.take("PERMUTATION_ACTION") << Perm;
    
    Group.set_name("LinAut");
    Group.set_description() << "linear symmetry group";
    
    return Group;
  }
  
  // At first we need the edge colors
  Matrix<Scalar> Q = zero_matrix<Scalar>( Rays.cols(), Rays.cols() );
  for(Int i=0; i<Rays.rows(); ++i){
    Set<Int> ray;
    ray += i;
    Matrix<Scalar> v = Rays.minor(ray, All);
    Q += T(v) * v;
  }
  
  Map<Scalar, Int> w2int;
  std::vector<std::vector<std::pair<Int,Int>>> reverse_w;
  
  // get the edge colors
  Matrix<Scalar> V = solve_left(Q,Rays);
  for(Int i=0; i<Rays.rows(); ++i){
    for(Int j=i+1; j<Rays.rows(); ++j){
      Vector<Scalar> v_j = Rays.row(j);
      Scalar w = V.row(i) * v_j;
      if(w2int.find(w) == w2int.end()){
        auto s = w2int.size();
        // they weights need to be strictly positive
        // and store the size first to avoid undefined behaviour
        w2int[w] = s+1;
        std::vector<std::pair<Int,Int>> Ray_pair;
	Ray_pair.push_back(std::make_pair(i, j));
	reverse_w.push_back(Ray_pair);
      }else{
        reverse_w[w2int[w]-1].push_back(std::make_pair(i, j));
      }
    }
  }
  
  // get the binary length of the colors
  Int len;
  if(w2int.size() == 1){
    len = 1;
  }else{
    len = (Int) ceil(log((double) w2int.size() )/log(2));
  }
  
  // define a superposition vertex color graph
  Graph<Undirected> g(Rays.rows()*len);
  
  //connect vertices of the graph that belong to the same ray
  for(Int k=0; k<Rays.rows(); k++){
    for(Int i=0; i<len; i++){
      for(Int j=i+1; j<len; j++){
        g.edge(k+i*Rays.rows(),k+j*Rays.rows());
      }
    }
  }
  
  // add the colored edges to the graphs which corresponding
  // to the ones in there binary description
  for(Int k = 1; k<= (Int)reverse_w.size(); k++){
    // iterate over the colors
    Int bin = 1;//TODO find a better solution
    for(Int i = 0; i<len; i++){
      // iterate over the bits of the color word
      if((k & bin) != 0){
        std::vector<std::pair<Int,Int>> vec = reverse_w[k-1];
        for(Int j=0; j< (Int) vec.size(); j++){
          g.edge(vec[j].first + i*Rays.rows(), vec[j].second + i*Rays.rows());
        }
      }
      bin*=2;
      if(bin > k){break;}
    }
  }
  
  // make vertex colors;
  Array<Int> colors(Rays.rows()*len);
  for(Int i = 0; i<Rays.rows()*len; i++){
    colors[i] = Int( i/Rays.rows() );
  }
  
  // get the linear group
  Array<Array<Int>> generator_long = call_function("graph::automorphisms", g, colors);
  Array<Array<Int>> generator(generator_long.size());
  
  // cut the stuff we don't need
  for(Int i=0; i<generator_long.size(); i++){
    Array<Int> elem(Rays.rows());
    for(Int j=0; j<Rays.rows(); j++){
      elem[j] = generator_long[i][j];
    }
    generator[i] = elem;
  }
  
  // define the output
  BigObject Perm("group::PermutationAction");
  Perm.take("GENERATORS") << generator;
  BigObject Group("group::Group");
  Group.take("PERMUTATION_ACTION") << Perm;
  
  Group.set_name("LinAut");
  Group.set_description() << "linear symmetry group";
  
  return Group;
  
}


UserFunctionTemplate4perl("# @category Symmetry"
                          "# Compute the linear symmetries of the rows of a rational matrix //M//."
                          "# This is an implementation of the algorithm described in"
                          "# the paper \"Computing symmetry groups of polyhedra\""
                          "# LMS J. Comput. Math. 17 (1) (2004)"
                          "# by By David Bremner, Mathieu Dutour Sikiri'{c},"
                          "# Dmitrii V. Pasechnik, Thomas Rehn and Achill Sch\"{u}rmann."
                          "# @param Matrix M"
                          "# @return Array<Array<Int>>"
                          "# @example [require bundled:bliss]"
                          "# > $ls = linear_symmetries(cube(2)->VERTICES);"
                          "# > print $ls->PERMUTATION_ACTION->GENERATORS;"
                          "# | 0 2 1 3"
                          "# | 1 0 3 2"
                          "# > print linear_symmetries(cube(3)->VERTICES)->PERMUTATION_ACTION->GENERATORS;"
                          "# | 0 1 4 5 2 3 6 7"
                          "# | 0 2 1 3 4 6 5 7"
                          "# | 1 0 3 2 5 4 7 6",
                          "linear_symmetries<Scalar>(Matrix<Scalar>)");



}}// end namespaces



