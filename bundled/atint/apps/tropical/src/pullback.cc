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

	Implements pullback.h
	*/

#include "polymake/tropical/pullback.h"

namespace polymake { namespace tropical {

	UserFunctionTemplate4perl("# @category Intersection theory"
			"# This computes the pullback of a rational function via a morphism"
			"# Due to the implementation of composition of maps, the [[DOMAIN]] of the"
			"# rational function need not be contained in the image of the morphism"
			"# The pullback will be defined in the preimage of the domain."
			"# @param Morphism m A morphism."
			"# @param RationalFunction r A rational function."
			"# @return RationalFunction The pullback m*r.",
			"pullback<Addition>(Morphism<Addition>, RationalFunction<Addition>)");

}}
