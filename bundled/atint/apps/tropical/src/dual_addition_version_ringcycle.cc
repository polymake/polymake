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

	Contains functions to compute the affine transform of a cycle 
	*/

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace tropical {

template <typename Addition>
BigObject dual_addition_version(BigObject ring_cycle)
{
  const Int n = ring_cycle.give("N_ELEMENTS");
  const Int r = ring_cycle.give("RANK");
  Array<IncidenceMatrix<>> pres = ring_cycle.give("NESTED_PRESENTATIONS");
  Array<Int> coef = ring_cycle.give("NESTED_COEFFICIENTS");

  return BigObject("MatroidRingCycle", mlist<typename Addition::dual>(),
                   "N_ELEMENTS", n,
                   "RANK", r,
                   "NESTED_PRESENTATIONS", pres,
                   "NESTED_COEFFICIENTS", coef);
}
  
UserFunctionTemplate4perl("# @category Conversion of tropical addition"
                          "# Takes a MatroidRingCycle and converts it to the dual tropical addition"
                          "# @param MatroidRingCycle<Addition> M"
                          "# @return MatroidRingCycle",
                          "dual_addition_version<Addition>(MatroidRingCycle<Addition>)");

} }
