/* Copyright (c) 1997-2019
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

#include <string>
#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/vector"
#include "polymake/common/labels.h"

namespace polymake { namespace topaz {

perl::Object independence_complex(perl::Object matroid, perl::OptionSet options)
{
   const Array< Set<int> > bases = matroid.give("BASES");
   const bool no_labels = options["no_labels"];
   
   perl::Object complex("topaz::SimplicialComplex");
   complex.set_description() << "Independence complex of matroid " << matroid.name() << "." << endl;
   complex.take("FACETS") << bases;
   
   if (!no_labels) {
     const int n_elements=matroid.give("N_ELEMENTS");
     const std::vector<std::string> labels = common::read_labels(matroid, "LABELS", n_elements);
     complex.take("VERTEX_LABELS") << labels;
   }
   return complex;
}


InsertEmbeddedRule("REQUIRE_APPLICATION matroid\n\n");

UserFunction4perl("# @category Producing a simplicial complex from other objects\n"
                  "# Produce the __independence complex__ of a given matroid.\n"
                  "# If //no_labels// is set to 1, the labels are not copied.\n"
                  "# @param matroid::Matroid matroid"
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @return SimplicialComplex",
                  &independence_complex,"independence_complex(matroid::Matroid; { no_labels => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
