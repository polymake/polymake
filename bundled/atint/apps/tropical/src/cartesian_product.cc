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

	Implements cartesian_product.h
	*/

#include "polymake/tropical/cartesian_product.h"

namespace polymake { namespace tropical {

	UserFunctionTemplate4perl("# @category Basic polyhedral operations"
			"# Computes the cartesian product of a set of cycles. If any of them has weights, so will the product"
			"# (all non-weighted cycles will be treated as if they had constant weight 1)"
			"# @param cycles A list of Cycles"
			"# @return Cycle The cartesian product. "
			"# Note that the representation is noncanonical, as it identifies"
			"# the product of two projective tori of dimensions d and e with a projective torus "
			"# of dimension d+e by dehomogenizing and then later rehomogenizing after the first coordinate.",
			"cartesian_product<Addition>(Cycle<Addition>+)");

}}
