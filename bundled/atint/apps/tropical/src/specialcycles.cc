/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	---
	Copyright (c) 2016-2019
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	This file provides functionality to compute certain special tropical varieties
	*/


#include "polymake/client.h"
#include "polymake/tropical/specialcycles.h"

namespace polymake { namespace tropical {

	bool is_empty_cycle(perl::Object cycle) {
		int proj_ambient_dim = cycle.give("PROJECTIVE_AMBIENT_DIM");
		IncidenceMatrix<> mpol = cycle.give("MAXIMAL_POLYTOPES");
		return proj_ambient_dim < 0 || mpol.rows() == 0;
	}//END is_empty

	// PERL WRAPPER -------------------------------------------

UserFunctionTemplate4perl("# @category Creation functions for specific cycles"
			  "# Creates the empty cycle in a given ambient dimension"
			  "# (i.e. it will set the property [[PROJECTIVE_AMBIENT_DIM]]."
			  "# @param Int ambient_dim The ambient dimension"
			  "# @tparam Addition Max or Min"
			  "# @return Cycle The empty cycle",
			  "empty_cycle<Addition>($)");

UserFunction4perl("# @category Degeneracy tests"
		  "# This tests wheter a cycle is the empty cycle.",
		  &is_empty_cycle,"is_empty(Cycle)");

UserFunctionTemplate4perl("# @category Creation functions for specific cycles"
			  "# Creates a cycle consisting of a collection of points"
			  "# with given weights"
			  "# @param Matrix<Rational> points The points, in tropical homogeneous coordinates"
			  "# (though not with leading ones for vertices)."
			  "# @param Vector<Integer> weights The list of weights for the points"
			  "# @tparam Addition Max or Min"
			  "# @return Cycle The point collection.",
			  "point_collection<Addition>($,$)");

UserFunctionTemplate4perl("# @category Creation functions for specific cycles"
			  "# Creates the linear space of the uniform matroid of rank k+1 on n+1 variables."
			  "# @param Int n The ambient (projective) dimension."
			  "# @param Int k The (projective dimension of the fan."
			  "# @param Integer weight The global weight of the cycle. 1 by default."
			  "# @tparam Addition A The tropical addition (min or max)"
			  "# @return Cycle A tropical linear space.",
			  "uniform_linear_space<Addition>($,$;$=1)");       

UserFunctionTemplate4perl("# @category Creation functions for specific cycles"
			  "# Creates a subdivision of the tropical projective torus"
			  "# along an affine hyperplane into two halfspaces."
			  "# This hyperplane is defined by an equation gx = a"
			  "# @param Rational a The constant coefficient of the equation"
			  "# @param Vector<Rational> g The linear coefficients of the equation"
			  "# Note that the equation must be homogeneous in the sense that (1,..1)"
			  "# is in its kernel, i.e. all entries of g add up to 0."
			  "# @param Integer w The (constant) weight this cycle should have"
			  "# @tparam Addition Max or Min"
			  "# @return Cycle The halfspace subdivision",
			  "halfspace_subdivision<Addition>($,$,$)");

UserFunctionTemplate4perl("# @category Creation functions for specific cycles"
			  "# Creates the tropical projective torus of a given dimension."
			  "# In less fancy words, the cycle is the complete complex"
			  "# of given (tropical projective) dimension n, i.e. R<sup>n</sup>"
			  "# @param Int n The tropical projective dimension."
			  "# @param Integer w The weight of the cycle. Optional and 1 by default."
			  "# @tparam Addition Max or Min."
			  "# @return Cycle The tropical projective torus.",
			  "projective_torus<Addition>($;$=1)");

UserFunctionTemplate4perl("# @category Creation functions for specific cycles"
			  "# Creates the orthant subdivision around a given point on a given chart,"
			  "# i.e. the corresponding affine chart of this cycle consists of all 2^n fulldimensional orthants"
			  "# @param Vector<Rational> point The vertex of the subdivision. Should be given in tropical homogeneous coordinates with leading coordinate."
			  "# @param Int chart On which chart the cones should be orthants, 0 by default."
			  "# @param Integer weight The constant weight of the cycle, 1 by default."
			  "# @tparam Addition Min or Max",
			  "orthant_subdivision<Addition>($; $=0,$=1)");

UserFunctionTemplate4perl("# @category Creation functions for specific cycles"
			  "# This creates a true affine linear space."
			  "# @param Matrix<Rational> lineality (Row) generators of the lineality space, in tropical"
			  "# homogeneous coordinates, but without the leading zero"
			  "# @param Vector<Rational> translate Optional. The vertex of the space. By default this is"
			  "# the origin"
			  "# @param Integer weight Optional. The weight of the space. By default, this is 1."
			  "# @tparam Addition Min or Max"
			  "# @return Cycle<Addition>",
			  "affine_linear_space<Addition>($; $ = new Vector(), $=1)");

UserFunctionTemplate4perl("# @category Creation functions for specific cycles"
			  "# This creates the k-skeleton of the tropical variety dual to the cross polytope"
			  "# @param Int n The (projective) ambient dimension"
			  "# @param Int k The (projective) dimension of the variety."
			  "# @param Rational h Optional, 1 by default. It is a nonnegative number, describing the "
			  "# height of the one interior lattice point of the cross polytope. "
			  "# @param Integer weight Optional, 1 by default. The (global) weight of the variety"
			  "# @tparam Addition Min or Max"
			  "# @return Cycle<Addition> The k-skeleton of the tropical hypersurface dual to the cross"
			  "# polytope. It is a smooth (for weight 1), irreducible (for h > 0) variety, which is invariant under reflection.",
			  "cross_variety<Addition>($,$; $=1,$=1)");


}}
