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
	Copyright (c) 2016-2020
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Implements psi_classes.h
	*/

#include "polymake/tropical/psi_classes.h"

namespace polymake { namespace tropical {

	UserFunctionTemplate4perl("# @category Moduli of rational curves"
			"# Computes a product of psi classes psi_1^k_1 * ... * psi_n^k_n on the moduli space"
			"# of rational n-marked tropical curves M_0,n"
			"# @param Int n The number of leaves in M_0,n"
			"# @param Vector<Int> exponents The exponents of the psi classes k_1,..,k_n. If the "
			"# vector does not have length n or if some entries are negative, an error is thrown"
			"# @tparam Addition Min or Max"
			"# @return Cycle The corresponding psi class divisor",
			"psi_product<Addition>($, Vector<Int>)");

	UserFunctionTemplate4perl("# @category Moduli of rational curves" 
			"# Computes the i-th psi class in the moduli space of n-marked rational tropical curves"
			"# M_0,n"
			"# @param Int n The number of leaves in M_0,n"
			"# @param Int i The leaf for which we want to compute the psi class ( in 1,..,n )"
			"# @tparam Addition Min or Max"
			"# @return Cycle The corresponding psi class",
			"psi_class<Addition>($,$)");



}}
