/* Copyright (c) 1997-2021
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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
#include "polymake/matroid/check_axioms.h"

namespace polymake { namespace matroid {

bool check_basis_exchange_axiom(const Array<Set<Int>>& bases, OptionSet options)
{
   const bool verbose = options["verbose"];
   return check_basis_exchange_axiom_impl(bases, verbose);
}

bool check_hyperplane_axiom(const Array<Set<Int>>& matroid_hyperplanes, OptionSet options)
{
   const bool verbose = options["verbose"];
   return check_hyperplane_axiom_impl(matroid_hyperplanes, verbose);
}

bool check_flat_axiom(const Array<Set<Int>>& matroid_flats, OptionSet options)
{
   const bool verbose = options["verbose"];
   return check_flat_axiom_impl(matroid_flats, verbose);
}

UserFunction4perl("# @category Other"
                  "# Check if a given list of sets satisfies the axioms to be the bases of a matroid."
                  "# @param Array<Set> B a list of would-be bases of a matroid"
                  "# @option Bool verbose print a proof if the given sets do not form the set of bases of a matroid"
                  "# @return Bool are the given sets the bases of a matroid?",
                  &check_basis_exchange_axiom,
                  "check_basis_exchange_axiom(Array<Set> { verbose => 0 })");

UserFunction4perl("# @category Other"
                  "# Check if a given list of sets satisfies the axioms to be the hyperplanes of a matroid."
                  "# @param Array<Set> H a list of would-be hyperplanes of a matroid"
                  "# @option Bool verbose print a proof if the given sets do not form the set of hyperplanes of a matroid"
                  "# @return Bool are the given sets the hyperplanes of a matroid?",
                  &check_hyperplane_axiom,
                  "check_hyperplane_axiom(Array<Set> { verbose => 0 })");

UserFunction4perl("# @category Other"
                  "# Check if a given list of sets satisfies the axioms to be the flats of a matroid."
                  "# @param Array<Set> F a list of would-be flats of a matroid"
                  "# @option Bool verbose print a proof if the given sets do not form the set of flats of a matroid"
                  "# @return Bool are the given sets the flats of a matroid?",
                  &check_flat_axiom,
                  "check_flat_axiom(Array<Set> { verbose => 0 })");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
