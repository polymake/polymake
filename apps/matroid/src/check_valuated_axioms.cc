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
#include "polymake/matroid/check_valuated_axioms.h"



namespace polymake { namespace matroid {

  UserFunctionTemplate4perl("# @category Other"
			    "# Takes a list of sets and a vector of valuations and checks"
			    "# if they fulfill the valuated basis axioms"
			    "# @param Array<Set<Int> > bases"
			    "# @param Vector<TropicalNumber<Addition,Scalar> > valuation"
			    "# @option Bool verbose. Whether the function should output when"
			    "# some axiom is not fulfilled. False by default."
			    "# @return Bool. Whether this is a basis valuation",
			    "check_valuated_basis_axioms<Addition,Scalar>(Array<Set<Int> >, Vector<TropicalNumber<Addition,Scalar> >;{verbose=>0})");
  
  UserFunctionTemplate4perl("# @category Other"
			    "# Takes a matrix of TropicalNumbers and checks if the rows"
			    "# fulfill the valuated circuit axioms"
			    "# @param Matrix<TropicalNumber<Addition,Scalar> > M"
			    "# @option Bool verbose. Whether the function should output when"
			    "# some axiom is not fulfilled. False by default."
			    "# @return Bool. Whether the matrix is a circuit valuation",
			    "check_valuated_circuit_axioms<Addition,Scalar>(Matrix<TropicalNumber<Addition,Scalar> >;{verbose=>0})");
  
}}
