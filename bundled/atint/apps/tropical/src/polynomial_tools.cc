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

	FIXME Most (or all) of these should at some time be implemented in 
	Polynomial.
	*/

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Polynomial.h"
#include "polymake/TropicalNumber.h"
#include "polymake/tropical/polynomial_tools.h"


namespace polymake { namespace tropical {

	FunctionTemplate4perl("evaluate_polynomial<Addition>(Polynomial<TropicalNumber<Addition>>,Vector)");
	FunctionTemplate4perl("polynomial_degree<Coefficient>(Polynomial<Coefficient>)");
	FunctionTemplate4perl("is_homogeneous<Coefficient>(Polynomial<Coefficient>)");
} }
