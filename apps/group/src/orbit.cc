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

#include "polymake/client.h"
#include "polymake/group/orbit.h"
#include "polymake/ApproximateSet.h"

namespace polymake { namespace group { 

template<typename Scalar>
auto
orbit(const Array<Matrix<Scalar>>& gens, const Vector<Scalar>& v)
{
  return orbit<on_elements, Matrix<Scalar>, Vector<Scalar>, hash_set<Vector<Scalar>>, pm::is_vector, pm::is_matrix>(gens, v);
}

template<>
auto
orbit(const Array<Matrix<double>>& gens, const Vector<double>& v)
{
  return orbit<on_elements, Matrix<double>, Vector<double>, ApproximateSet<Vector<double>>, pm::is_vector, pm::is_matrix>(gens, v);
}
    
UserFunctionTemplate4perl("# @category Utilities"
                         "# The image of an object //O// under a group element //g//.  The action_type defines how //g// acts on //O//."
			 "# For more information see the help function on the action types."
                         "# @param Any g Group element"
                         "# @param Any O Container type like Array, Set, Matrix or Vector"
                         "# @tparam action_type must be compatible with //O// by default [[on_container]]"
                         "# can also be [[on_elements]], [[on_rows]], [[on_cols]], [[on_nonhomog_cols]]"
                         "# @return Any the type depends on the type of //O//"
                         "# @example"
                         "# > $g = new Array<Int>([1,2,0]);"
                         "# > $O = new Matrix<Int>([[0,0],[1,1],[2,2]]);"
                         "# > print $O;"
                         "# | 0 0"
                         "# | 1 1"
                         "# | 2 2"
                         "# > print action<on_rows>($g, $O);"
                         "# | 1 1"
                         "# | 2 2"
                         "# | 0 0",
                         "action<action_type=on_container>(*, *)");

UserFunctionTemplate4perl("# @category Utilities"
                         "# The image of an object //O// under the inverse of a"
                         "# permutation //p//. The way it acts on //O// is given"
                         "# by the //action_type//."
                         "# @param Array<Int> p permutation"
                         "# @param Any O Container type like Array, Set, Matrix or Vector"
                         "# @tparam action_type must be compatible with //O//, by default [[on_container]]"
                         "# can also be [[on_elements]], [[on_rows]], [[on_cols]] or [[on_nonhomog_cols]]"
                         "# @return Any"
                         "# @example"
                         "# > $p = new Array<Int>([1,2,0]);"
                         "# > $O = new Array<Int>([2,3,1]);"
                         "# > print action_inv($p,$O);"
                         "# | 1 2 3",
                         "action_inv<action_type=on_container>(Array<Int>, *)");

UserFunctionTemplate4perl("# @category Orbits"
                         "# The orbit of an object //O// under a group generated by //G//. The action_type defines how the elements of the group work on //O//."
                         "# @param Array G Group generators"
                         "# @param Any O  (mostly Array, Vector, Matrix or Set)"
			                   "# @tparam action_type controls the"
			                   "# [[on_container]] the default. The group acts on the elements of a given container, which also could be container."
			                   "# [[on_elements]] for this the group will act on the deepest elements inside a given container with respect to the type of the group elements"
			                   "# [[on_rows]] For this //O// should be a matrix so that the group can act on the rows of //O//."
			                   "# [[on_cols]] like [[on_rows]] this require a matrix so that the group can act on its columns"
			                   "# [[on_nonhomog_cols]] also require that //O// is a matrix on whose columns the group generated by //G// acts. The 0-th column stay fixed."
                         "# @return Set<Any>"
                         "# @example"
                         "# > $permutationgroup = new Array<Array<Int>>([[1,2,3,0]]);"
                         "# > $set = new Set<Set<Int>>([[0,1],[2,3]]);"
                         "# > print orbit<on_elements>($permutationgroup, $set);"
                         "# | {{{0 1} {2 3}} {{0 3} {1 2}}}",
                         "orbit<action_type=on_container>(Array, *)");

InsertEmbeddedRule("# @category Orbits"
                  "# The orbit of a container //C// under a group //G//."
                  "# @param Group G"
                  "# @param Any C"
                  "# @tparam action_type by default [[on_container]]"
                  "# can also be [[on_elements]], [[on_rows]], [[on_cols]] or [[on_nonhomog_cols]]."
                  "# Call help \"<action_type>\"; for more information about this."
                  "# @return Set<Any>\n"
                   "user_function orbit<action_type=on_container>(PermutationAction, $) {\n"
                   "   orbit<action_type>($_[0]->GENERATORS, $_[1]);\n"
                   "}\n");

UserFunctionTemplate4perl("# @category Orbits\n"
			  "# The indices of one representative for each orbit under the group generated by //G//."
			  "# @param Array<GeneratorType> G Group generators"
			  "# @return Array<Int>",
			  "orbit_representatives<GeneratorType>(Array<GeneratorType>)");


UserFunctionTemplate4perl("# @category Orbits\n"
			  "# The orbit of a vector //V// under a group generated by //G//."
			  "# @param Array<Matrix<Scalar>> G Generators of the group"
			  "# @param Vector<Scalar> V"
			  "# @tparam Scalar S the number type"
			  "# @return Set",
			  "orbit<Scalar>(Array<Matrix<Scalar>>, Vector<Scalar>)");

} } // end namespaces
