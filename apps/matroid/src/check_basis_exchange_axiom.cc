/* Copyright (c) 1997-2015
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
#include "polymake/Array.h"
#include "polymake/matroid/check_basis_exchange_axiom.h"

namespace polymake { namespace matroid {

int check_basis_exchange_axiom(const Array<Set<int> >& bases, perl::OptionSet options)
{
   const bool verbose = options["verbose"];
   return check_basis_exchange_axiom_impl(bases, verbose);
}

UserFunction4perl("# @category Other"
                  "# Check if a given list of sets satisfies the axioms to be the bases of a matroid."
                  "# @param Array<Set> a list of would-be bases of a matroid"
                  "# @option Bool verbose print a proof if the given sets do not form the set of bases of a matroid"
                  "# @return Int is_matroid are the given sets the bases of a matroid?",
                  &check_basis_exchange_axiom,
                  "check_basis_exchange_axiom(Array<Set> { verbose => 0 })");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
