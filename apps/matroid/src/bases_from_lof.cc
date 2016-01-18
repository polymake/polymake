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
#include "polymake/Set.h"
#include "polymake/matroid/bases_from_lof.h"

namespace polymake { namespace matroid {

void bases_from_lof(perl::Object M)
{
   const graph::HasseDiagram LF = M.give("LATTICE_OF_FLATS");
   const int n = M.give("N_ELEMENTS");
   const Array<Set<int> > bases = bases_from_lof_impl(LF, n);
	int LF_dim = LF.dim();
	if(LF_dim == -1) LF_dim = 0;
   M.take("RANK") << LF_dim; //Hasse diagram has a shift for lattice with just one node
   M.take("BASES") << bases;
   M.take("N_BASES") << bases.size();
}

Function4perl(&bases_from_lof, "bases_from_lof(Matroid)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
