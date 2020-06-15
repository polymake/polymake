/* Copyright (c) 1997-2020
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
#include "polymake/Set.h"
#include "polymake/Vector.h"
#include "polymake/Polynomial.h"
#include "polymake/Rational.h"


namespace polymake { namespace topaz {

BigObject stanley_reisner(BigObject C)
{
  const Array<Set<Int>> non_faces=C.give("MINIMAL_NON_FACES");
  const Int n_non_faces = non_faces.size();
  const Int n_vertices = C.give("N_VERTICES");

  Array<Polynomial<Rational, Int>> gens(n_non_faces);

  for (Int k = 0; k < n_non_faces; ++k) {
     gens[k] = Polynomial<Rational, Int>(1, same_element_sparse_vector<Int>(non_faces[k], n_vertices));
  }

  BigObject I("ideal::Ideal",
              "GENERATORS", gens,
              "MONOMIAL", true,
              "N_VARIABLES", n_vertices);
  I.set_description() << "Stanley-Reisner ideal of " << C.name();
  return I;
}

UserFunction4perl("# @category Other"
                  "# Creates the __Stanley-Reisner ideal__ of a simplicial complex."
                  "# @param  SimplicialComplex complex"
                  "# @return ideal::Ideal",
                  &stanley_reisner, "stanley_reisner(SimplicialComplex)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
