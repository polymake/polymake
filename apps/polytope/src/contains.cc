/* Copyright (c) 1997-2020
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   But WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
--------------------------------------------------------------------------------
*/
#include "polymake/client.h"
#include "polymake/polytope/contains.h"
#include "polymake/polytope/optimal_contains.h"

namespace polymake { namespace polytope {
template <typename Scalar>
bool cone_contains(BigObject p_in, BigObject p_out){
   return contains<Scalar>(p_in,p_out);
}
FunctionTemplate4perl("cone_contains<Scalar> (Cone<Scalar>, Cone<Scalar>)");


FunctionTemplate4perl("polytope_contains_ball<Scalar> (Vector<Scalar>, Scalar, Polytope<Scalar>)");

FunctionTemplate4perl("polytope_contained_in_ball<Scalar> ( Polytope<Scalar>, Vector<Scalar>, Scalar)");


UserFunctionTemplate4perl("# @category Geometry"
                          "# Finds the minimal Ball which contains a"
                          "# given Polytope P_in with"
                          "# semidefinit programming. The ball "
                          "# is given by its center c and the "
                          "# square of it radius r."
                          "# @param Polytope P_in the inner Polytope"
                          "# @return Pair <Scalar r, Vector c>"
                          "# @example"
                          "# > $P_in = new Polytope(POINTS=>[[1,0],[1,4]]);"
                          "# > print minimal_ball($P_in);"
                          "# | 4 <1 2>",
                          "minimal_ball<Scalar>(Polytope<Scalar>)");

UserFunctionTemplate4perl("# @category Geometry"
                          "# Finds for a given inner Polytope //P_in// and a" 
                          "# given outer Polytope //P_out// a maximal a scalar" 
                          "# //s// and a vector //t//, such that //P_in// scaled with"
                          "# s and shifted by t is a subset of //P_out//."
                          "# @param Polytope P_in the inner Polytope"
                          "# @param Polytope P_out the outer Polytope"
                          "# @return Pair <Scalar s, Vector t> "
                          "# @example"
                          "# > $P_in = new Polytope(POINTS=>[[1,0],[1,1]]);"
                          "# > $P_out = new Polytope(POINTS=>[[1,2],[1,4]]);"
                          "# > print optimal_contains($P_in,$P_out);"
                          "# | 2 <1 2>",
                          "optimal_contains<Scalar>(Polytope<Scalar>, Polytope<Scalar>)");



}}
