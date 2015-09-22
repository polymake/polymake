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

	Implements skeleton.h
	*/

#include "polymake/tropical/skeleton.h"

namespace polymake { namespace tropical {

	UserFunctionTemplate4perl("# @category Basic polyhedral operations"
			"# Takes a polyhedral complex and computes the k-skeleton. Will return an empty cycle, "
			"# if k is larger then the dimension of the given complex or smaller than 0."
			"# @param Cycle<Addition> C A polyhedral complex."
			"# @param Int k The dimension of the skeleton that should be computed"
			"# @param Bool preserveRays When true, the function assumes that all rays of the fan remain"
			"# in the k-skeleton, so it just copies the VERTICES, instead of computing an irredundant list."
			"# By default, this property is false."
			"# @return Cycle<Addition> The k-skeleton (without any weights, except if k is the dimension of C",
			"skeleton_complex<Addition>(Cycle<Addition>, $;$=0)");
}}
