/* Copyright (c) 1997-2022
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
#include "polymake/topaz/complex_tools.h"
#include "polymake/list"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/FacetList.h"

namespace polymake { namespace topaz {
        
// Delete face from the list of facets C
// Returns 0 if face was not a subset of any of the facets of C (C remains unchanged),
// returns 1 otherwise
bool delete_face(FacetList &C, const Set<Int>& face)
{
   std::list<Set<Int>> deletedFaces;
   if (C.eraseSupersets(face, back_inserter(deletedFaces))) {
      do {
         // insert the parts of the boundary not containing the face
         const Set<Int> deleted = deletedFaces.front();  deletedFaces.pop_front();
         const bool exact_match = deleted.size() == face.size();
         for (auto boundaryIt=entire(all_subsets_less_1(deleted));  !boundaryIt.at_end(); ++boundaryIt) {
            if (!exact_match && (face - *boundaryIt).empty()) {
               // face is a proper subset of the boundary face too
               deletedFaces.push_back(*boundaryIt);
            } else {
               C.insertMax(*boundaryIt);
            }
         }
      } while (! deletedFaces.empty());
      return true;
   }
   return false;
}

BigObject deletion_complex(BigObject p_in, const Set<Int>& face, OptionSet options)      
{       
   FacetList facets = p_in.give("FACETS");      
   const Int n_vert = p_in.give("N_VERTICES");
   if (face.empty())
      throw std::runtime_error("deletion: empty face specified");
   if (face.front()<0 || face.back()>n_vert-1)
      throw std::runtime_error("deletion: specified vertex indices out of range");
   
   if ( !delete_face(facets,face) ) {
      std::ostringstream e;
      wrap(e) << "deletion: " << face << " does not specify a face.";
      throw std::runtime_error(e.str());
   }

   BigObject p_out("SimplicialComplex");
   p_out.set_description() << "Deletion of " << face << " in " << p_in.name() << ".\n";

   Set<Int> V;
   if (facets.empty()) {
      p_out.take("FACETS") << Array<Set<Int>>(1);
   } else {
      Array<Set<Int>> array_facets{ lex_ordered(facets) };
      V = accumulate(array_facets, operations::add());
      adj_numbering(array_facets,V);
      p_out.take("FACETS") << array_facets;
   }

   if (!options["no_labels"]) {
      const Array<std::string> L = p_in.give("VERTEX_LABELS");
      const Array<std::string> new_L(V.size(), select(L,V).begin());
      p_out.take("VERTEX_LABELS") << new_L;
   }

   return p_out;
}

UserFunction4perl("# @category Producing a new simplicial complex from others"
                  "# Remove the given //face// and all the faces containing it."
                  "# @param SimplicialComplex complex"
                  "# @param Set<Int> face specified by vertex indices."
                  "#  Please use [[labeled_vertices]] if you want to specify the face by vertex labels."
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @example Deleting any face of the 3-simplex yields a pure 2-dimensional complex with 3 facets:"
                  "# > $s = deletion(simplex(3),[0,1,2]);"
                  "# > print $s->PURE, ', ', $s->DIM, ', ', $s->N_FACETS;"
                  "# | true, 2, 3"
                  "# @return SimplicialComplex",
                  &deletion_complex, "deletion(SimplicialComplex $ { no_labels => 0 } )");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
