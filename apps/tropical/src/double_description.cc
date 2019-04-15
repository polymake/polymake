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

#include "polymake/client.h"
#include "polymake/tropical/double_description.h"

namespace polymake { namespace tropical {

    FunctionTemplate4perl("monoextremals(Matrix, Matrix, Vector)");

    FunctionTemplate4perl("extremals_from_generators(Matrix)");

    UserFunctionTemplate4perl("# @category Tropical operations"
			      "# This computes the __extremal generators__ of a tropical cone "
			      "# given by generators //G// intersected with one inequality //a//x ~ //b//x."
			      "# Here, ~ is >= for min and <= for max."
			      "# @param Matrix<TropicalNumber<Addition, SCALAR> > G"
			      "# @param Vector<TropicalNumber<Addition, SCALAR> > a"
			      "# @param Vector<TropicalNumber<Addition, SCALAR> > b"
			      "# @return Matrix<TropicalNumber<Addition, SCALAR> > extrls"
			      "# @example"
			      "# > $G = new Matrix<TropicalNumber<Min>>([[0,0,2],[0,4,0],[0,3,1]]);"
			      "# > $a = new Vector<TropicalNumber<Min>>([0,-1,'inf']);"
			      "# > $b = new Vector<TropicalNumber<Min>>(['inf','inf',-2]);"
			      "# > print intersection_extremals($G,$a,$b);"
			      "# | 0 0 1"
			      "# | 0 4 0"
			      "# | 0 3 1",
			      "intersection_extremals(Matrix, Vector, Vector)");

    UserFunctionTemplate4perl("# @category Tropical operations"
			      "# compute the dual description of "
			      "# a monomial tropical cone. "
			      "# @param Matrix<SCALAR> monomial_generators"
			      "# @return Pair<Matrix<SCALAR>, IncidenceMatrix>",
			      "dual_description(Matrix)");


    UserFunctionTemplate4perl("# @category Tropical operations"
			      "# Reformulate the description of an "
			      "# inequality system given by two matrices"
			      "# to the description by apices and infeasible sectors " 
			      "# @param Matrix<TropicalNumber<Addition, SCALAR>> G"
			      "# @param Matrix<TropicalNumber<Addition, SCALAR>> A"
			      "# @return Pair<Matrix<TropicalNumber<Addition, SCALAR>>, Array<Set<Int>>> signed_apices",
			      "matrixPair2apexSet(Matrix, Matrix)");

    UserFunctionTemplate4perl("# @category Tropical operations"
			      "# Check if a point is contained in "
			      "# all tropical halfspaces given by "
			      "# their apices and the infeasible sectors " 
			      "# @param Matrix<TropicalNumber<Addition, SCALAR>> apices"
			      "# @param Array<Set<Int>> sectors"
			      "# @return Bool",
			      "is_contained(Vector, Matrix, Array)");

    
}}
