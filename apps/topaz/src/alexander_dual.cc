/* Copyright (c) 1997-2014
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
#include "polymake/Set.h"
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {

perl::Object alexander_dual(perl::Object p_in, perl::OptionSet options)
{
   Array< Set<int> > MNF = p_in.give("MINIMAL_NON_FACES");
   const int n_vert = p_in.give("N_VERTICES");

   for (Entire< Array< Set<int> > >::iterator nf=entire(MNF); !nf.at_end(); ++nf)
      *nf = range(0,n_vert-1) - *nf;

   Set<int> V = accumulate(MNF, operations::add());
   adj_numbering(MNF,V);

   perl::Object p_out("SimplicialComplex");
   p_out.set_description() << "Alexander dual of " << p_in.name() << endl;

   if (MNF.empty()) MNF.resize(1);      // add a single empty face
   p_out.take("FACETS") << MNF;

   if (!options["nol"]) {
      const Array<std::string> L = p_in.give("VERTEX_LABELS");
      const Array<std::string> new_L(V.size(), select(L,V).begin());
      p_out.take("VERTEX_LABELS") << new_L;
   }

   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Computes the Alexander dual complex, that is, the complements of all non-faces.\n"
                  "# The vertex labels are preserved unless the //nol// flag is specified.\n"
                  "# @param SimplicialComplex complex"
                  "# @option Bool nol"
                  "# @return SimplicialComplex",
                  &alexander_dual, "alexander_dual(SimplicialComplex { nol => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
