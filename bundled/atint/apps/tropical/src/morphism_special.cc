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
	Copyright (c) 2016-2022
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Implements morphism_special.h
	*/

#include "polymake/tropical/morphism_special.h"

namespace polymake { namespace tropical {


	UserFunctionTemplate4perl("# @category Moduli of rational curves" 
			"# This creates the i-th evaluation function on $ M_{0,n}^{(lab)}(R^r,Delta) $"
			"# (which is actually realized as $ M_{0,(n+|Delta|)} \\times R^r $ )"
			"# and can be created via [[space_of_stable_maps]])."
			"# @param Int n The number of marked (contracted) points"
			"# @param Matrix<Rational> Delta The directions of the unbounded edges (given as row vectors "
			"# in tropical projective coordinates without leading coordinate, i.e. have r+1 columns)"
			"# @param Int i The index of the marked point that should be evaluated."
			"# Should lie in between 1 and n"
			"# Note that the i-th marked point is realized as the $ (|Delta|+i) $-th leaf in $ M_{0,n+|Delta|} $"
			"# and that the $ R^r $ - coordinate is interpreted as the position of the n-th leaf. "
			"# In particular, ev_n is just the projection to the R^r-coordinates"
			"# @tparam Addition Min or Max"
			"# @return Morphism<Addition> ev_i. Its domain is the ambient space of the moduli space "
			"# as created by [[space_of_stable_maps]]. The target space is the tropical projective"
			"# torus of dimension r",
			"evaluation_map<Addition>($,Matrix<Rational>,$)");

	FunctionTemplate4perl("evaluation_map_d<Addition>($,$,$,$)");

	InsertEmbeddedRule("# @category Moduli of rational curves"
			"# This creates the i-th evaluation function on $ M_{0,n}^{(lab)}(R^r,d) $"
			"# (which is actually realized as $ M_{0,n+d(r+1)} \\times R^r $ )"
			"# This is the same as calling the function"
			"# evaluation_map(Int,Int,Matrix<Rational>,Int) with the standard d-fold"
			"# degree as matrix (i.e. each (inverted) unit vector of $ R^{r+1} $ occurring d times)."
			"# @param Int n The number of marked (contracted) points"
			"# @param Int r The dimension of the target space"
			"# @param Int d The degree of the embedding. The direction matrix will be"
			"# the standard d-fold directions, i.e. each unit vector (inverted for Max),"
			"# occurring d times."
			"# @param Int i The index of the marked point that should be evaluated. i "
			"# should lie in between 1 and n"
			"# @tparam Addition Min or Max"
			"# @return Morphism<Addition> ev_i. Its domain is the ambient space of the moduli space "
			"# as created by [[space_of_stable_maps]]. The target space is the tropical projective"
			"# torus of dimension r\n"
			"user_function evaluation_map<Addition>($,$,$,$) {\n"
			"	my ($n,$r,$d,$i) = @_;\n"
			" 	return evaluation_map_d<Addition>($n,$r,$d,$i);\n"
			"}\n");

	UserFunctionTemplate4perl("# @category Creation function for specific morphisms and functions"
			"# This creates a linear projection from the projective torus of dimension n to a given set"
			"# of coordinates. "
			"# @param Int n The dimension of the projective torus which is the domain of the projection."
			"# @param Set<Int> s The set of coordinaes to which the map should project. Should be"
			"# a subset of (0,..,n)"
			"# @tparam Addition Min or Max"
			"# @return Morphism<Addition> The projection map.",
			"projection_map<Addition>($,Set<Int>)");

	FunctionTemplate4perl("projection_map_default<Addition>($,$)");

	InsertEmbeddedRule("# @category Creation function for specific morphisms and functions"
			"# This computes the projection from a projective torus of given dimension to a projective"
			"# torus of lower dimension which lives on the first coordinates"
			"# @param Int n The dimension of the larger torus"
			"# @param Int m The dimension of the smaller torus"
			"# @return Morphism The projection map\n"
			"user_function projection_map<Addition>($,$) {\n"
			"	my ($n,$m) = @_;\n"
			"	return projection_map_default<Addition>($n,$m);\n"
			"}\n");

	UserFunctionTemplate4perl("# @category Moduli of rational curves"
			"# This computes the forgetful map from the moduli space $ M_{0,n} $ to $ M_{0,n-|S|} $"
			"# @param Int n The number of leaves in the moduli space $ M_{0,n} $"
			"# @param Set<Int> S The set of leaves to be forgotten. Should be a subset of (1,..,n)"
			"# @tparam Addition Min or Max"
			"# @return Morphism The forgetful map. It will identify the remaining leaves "
			"# $ i_1,..,i_(n-|S|) $ with the leaves of $ M_{0,n-|S|} $ in canonical order."
			"# The domain of the morphism is the ambient space of the morphism in matroid coordinates,"
			"# as created by [[m0n]].",
			"forgetful_map<Addition>($,Set<Int>)");

}}
