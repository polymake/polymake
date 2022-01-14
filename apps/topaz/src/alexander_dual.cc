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
#include "polymake/Set.h"
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {

BigObject alexander_dual(BigObject p_in, OptionSet options)
{
   IncidenceMatrix<> MNF = p_in.give("MINIMAL_NON_FACES");
   const Int n_vert = p_in.give("N_VERTICES");
   if (MNF.cols() < n_vert)
      MNF.resize(MNF.rows(),n_vert);

   MNF = ~MNF;
   Set<Int> V = accumulate(rows(MNF), operations::add());
   adj_numbering(rows(MNF),V);

   BigObject p_out("SimplicialComplex");
   p_out.set_description() << "Alexander dual of " << p_in.name() << endl;

   if (MNF.rows() == 0) MNF.resize(1,n_vert);      // add a single empty face
   p_out.take("FACETS") << Array<Set<Int>>(rows(MNF));
   p_out.take("VERTEX_INDICES") << V;

   if (!options["no_labels"]) {
      const Array<std::string> L = p_in.give("VERTEX_LABELS");
      const Array<std::string> new_L(V.size(), select(L,V).begin());
      p_out.take("VERTEX_LABELS") << new_L;
   }

   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Computes the Alexander dual complex, that is, the complements of all non-faces.\n"
                  "# The vertex labels are preserved unless the //no_labels// flag is specified.\n"
                  "# @param SimplicialComplex complex"
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @return SimplicialComplex"
		  "# @example The following creates the alexander dual of a particular graph on 5 vertices."
		  "# > $s = new SimplicialComplex(INPUT_FACES=>[[0, 1], [1, 2], [0, 2], [0, 3], [4]]);"
		  "# > $t = alexander_dual($s);"
	 	  "# > print $t -> F_VECTOR;"
		  "# | 5 10 6", 
                  &alexander_dual, "alexander_dual(SimplicialComplex { no_labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
