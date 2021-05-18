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
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/tropical/arithmetic.h"

namespace polymake { namespace tropical {
  
UserFunctionTemplate4perl("# @category Tropical operations"
                          "# The __tropical determinant__ of a matrix."
                          "# See also [[tdet_and_perm]] and [[second_tdet_and_perm]]."
                          "# @param Matrix<TropicalNumber> matrix"
                          "# @return TropicalNumber"
                          "# @example"
                          "# > print tdet(new Matrix<TropicalNumber<Max>>([[1,0,0],[0,1,0],[0,0,1]]));"
                          "# | 3" ,
                          "tdet(Matrix)");

UserFunctionTemplate4perl("# @category Tropical operations"
                          "# The __tropical determinant__ of a matrix and one optimal permutation."
                          "# @param Matrix<TropicalNumber> matrix"
                          "# @return Pair<TropicalNumber, Array<Int>>"
                          "# @example"
                          "# > print tdet_and_perm(new Matrix<TropicalNumber<Min>>([[1,0,0],[0,1,0],[0,0,1]]));"
                          "# | 0 <2 0 1>" ,
                          "tdet_and_perm(Matrix)");
      
UserFunctionTemplate4perl("# @category Tropical operations"
                          "# The __tropical determinant__ of a matrix and all optimal permutations."
                          "# @param Matrix<TropicalNumber> matrix"
                          "# @return Pair<TropicalNumber, Set<Array<Int>>>",
                          "tdet_and_perms(Matrix)");
      
UserFunctionTemplate4perl("# @category Tropical operations"
                          "# All __tropically optimal permutations__ of a matrix"
                          "# (i.e. those yielding the tropical determinant)."
                          "# @param Matrix<TropicalNumber> matrix"
                          "# @return Set<Array<Int>>"
                          "# @example"
                          "# > print optimal_permutations(new Matrix<TropicalNumber<Min>>([-1,-1,2],[0,-2,-2],[0,2,0]));"
                          "# | {<0 1 2> <1 2 0>}" ,
                          "optimal_permutations(Matrix)");
      
UserFunctionTemplate4perl("# @category Tropical operations"
                          "# The __second tropical optimum__ of a matrix and one corresponding permutation."
                          "# @param Matrix<TropicalNumber> matrix"
                          "# @return Pair<TropicalNumber, Array<Int>>"
                          "# @example"
                          "# > print second_tdet_and_perm(new Matrix<TropicalNumber<Min>>([[1,0,0],[0,1,0],[0,0,1]]));"
                          "# | 0 <1 2 0>" ,
                          "second_tdet_and_perm(Matrix)");      
      
UserFunctionTemplate4perl("# @category Tropical operations"
                          "# The __solution vector__ of an unsigned tropical matrix equation."
                          "# For more details and background see "
                          "Akian, Gaubert & Guterman: Tropical Cramer determinants revisited. "
                          "Tropical and idempotent mathematics and applications, Contemp. Math., 616, AMS, 2014 "
                          "# @param Matrix<TropicalNumber> matrix"
                          "# @return Vector<TropicalNumber>"
                          "# @example"
                          "# > print cramer(new Matrix<TropicalNumber<Min>>([[0,1,0],[0,4,1]]));"
                          "# | 2 0 1" ,
                          "cramer(Matrix)");

UserFunctionTemplate4perl("# @category Tropical operations"
                          "# __Tropical regularity__ of a matrix"
                          "# @param Matrix<TropicalNumber> matrix"
                          "# @return Bool"
                          "# @example A tropically regular matrix"
                          "# > print tregular(new Matrix<TropicalNumber<Min>>([0,-1,2],[0,-2,-2],[0,2,0]));"
                          "# | true"
                          "# @example A tropically singular matrix"
                          "# > print tregular(new Matrix<TropicalNumber<Min>>([-1,-1,2],[0,-2,-2],[0,2,0]));"
                          "# | false" ,
                          "tregular(Matrix)");
      
UserFunctionTemplate4perl("# @category Tropical operations"
                          "# Tropical distance function. This is a metric on the tropical projective torus."
                          "# The same for Min and Max."
                          "# @param Vector<TropicalNumber<Addition, Scalar>> v"
                          "# @param Vector<TropicalNumber<Addition, Scalar>> w"
                          "# @return Scalar"
                          "# @example"
                          "# > $v=new Vector<TropicalNumber<Min>>([1,0]);"
                          "# > $w=new Vector<TropicalNumber<Min>>([0,1]);"
                          "# > print tdist($v,$w);"
                          "# | 2" ,
                          "tdist<Addition,Scalar>(Vector<TropicalNumber<Addition, Scalar>>, Vector<TropicalNumber<Addition, Scalar>>)");

UserFunctionTemplate4perl("# @category Tropical operations"
                          "# Tropical diameter of a simplex, defined by the columns of a matrix."
                          "# This is the maximum over the pairwise tropical distances."
                          "# The same for Min and Max."
                          "# @param Matrix<TropicalNumber<Addition, Scalar>> matrix"
                          "# @return Scalar"
                          "# @example"
                          "# > print tdiam(new Matrix<TropicalNumber<Max>>([[1,0,0],[0,1,0],[0,0,1]]));"
                          "# | 2" ,
                          "tdiam<Addition,Scalar>(Matrix<TropicalNumber<Addition,Scalar>>)");

UserFunctionTemplate4perl("# @category Tropical operations"
                          "# Compute the solution of the tropical equation //A// * //x// = //b//."
                          "# If there is no solution, the return value is 'near' a solution. \n"
                          "# Cf. "
                          "# Butkovic 'Max-linear systems: theory and algorithms' (MR2681232), "
                          "# Theorem 3.1.1"
                          "# @param Matrix<TropicalNumber> A"
                          "# @param Vector<TropicalNumber> b"
                          "# @return Vector<TropicalNumber> x"
                          "# @example"
                          "# > $A = new Matrix<TropicalNumber<Min>>([[1,2],[3,4]]);"
                          "# > $b = new Vector<TropicalNumber<Min>>([5,6]);"
                          "# > print principal_solution($A, $b);"
                          "# | 4 3",
                          "principal_solution(Matrix,Vector)"); 

UserFunctionTemplate4perl("# @category Tropical operations"
                          "# __Signed tropical regularity__ of a matrix"
                          "# Returns, whether all optimal permutations have the same sign."
                          "# @param Matrix<TropicalNumber> matrix"
                          "# @return Bool"
                          "# @example A regular matrix is also sign-regular"
                          "# > print stregular(new Matrix<TropicalNumber<Min>>([0,-1,2],[0,-2,-2],[0,2,0]));"
                          "# | true"
                          "# @example A sign-singular matrix"
                          "# > print stregular(new Matrix<TropicalNumber<Min>>([0,1,0],[1,0,1],[0,1,0]));"
                          "# | false" ,
                          "stregular(Matrix)");
      
UserFunctionTemplate4perl("# @category Tropical operations"
                          "# The __sign__ of a tropical determinant."
                          "# Returns the sign of the unique optimal permutation if the matrix is tropically regular,"
                          "# otherwise 0."
                          "# @param Matrix<TropicalNumber> matrix"
                          "# @return Int",
                          "tsgn(Matrix)");
      


FunctionTemplate4perl("rel_coord(Vector, Vector)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
