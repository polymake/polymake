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
#include "polymake/topaz/complex_tools.h"

namespace polymake { namespace topaz {
  
perl::Object star_deletion_complex(perl::Object p_in, const Set<int>& face, perl::OptionSet options)      
{
   const Array< Set<int> > C = p_in.give("FACETS");
   const int n_vert = p_in.give("N_VERTICES");
   if (face.empty())
      throw std::runtime_error("star_deletion: empty face specified");
   if (face.front()<0 || face.back()>n_vert-1)
      throw std::runtime_error("star_deletion: specified vertex indices out of range");
   
   std::list< Set<int> > Deletion;
   copy_range(entire(deletion(C,face)), std::back_inserter(Deletion));
   
   if (int(Deletion.size())==int(C.size())) {
      std::ostringstream e;
      wrap(e) << "star_deletion: " << face << " does not specify a face.";
      throw std::runtime_error(e.str());
   }

   const Set<int> V = accumulate(Deletion, operations::add());

   adj_numbering(Deletion,V);
   
   perl::Object p_out("SimplicialComplex");
   p_out.set_description() << "Deletion of the star of " << face << " in " << p_in.name() << ".\n";

   if (Deletion.empty())
      p_out.take("FACETS") << Array< Set<int> >(1);
   else
      p_out.take("FACETS") << as_array(Deletion);

   if (!options["no_labels"]) {
      const Array<std::string> L = p_in.give("VERTEX_LABELS");
      const Array<std::string> new_L(select(L,V));
      p_out.take("VERTEX_LABELS") << new_L;
   }
   
   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others"
                  "# Remove the star of a given //face//."
                  "# @param SimplicialComplex complex"
                  "# @param Set<Int> face specified by vertex indices."
                  "#  Please use [[labeled_vertices]] if you want to specify the face by vertex labels."
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @return SimplicialComplex",
                  &star_deletion_complex, "star_deletion(SimplicialComplex $ { no_labels => 0 } )");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
