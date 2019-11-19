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

#include "polymake/client.h"
#include "polymake/topaz/morse_matching_tools.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace topaz {

typedef graph::Lattice<graph::lattice::BasicDecoration> Lattice;

// Compute the critical faces of a Morse matching.
// @param SimplicialComplex p a complex with a Morse matching
void morse_matching_critical_faces (perl::Object p)
{
   perl::Object HD_obj = p.give("HASSE_DIAGRAM");
   const Lattice M(HD_obj);
   const int d = M.rank() - 2;
   const HasseEdgeMap EM = p.give("MORSE_MATCHING.MATCHING");
   Bitset critical = collectCriticalFaces(M, EM);
   Array<int> numCritical(d+1);
   for (int k = 0; k <= d; ++k) {
      for (const auto f : M.nodes_of_rank(k+1)) {
         if (critical.contains(f)) {
            const int dim = M.rank(f)-1;
            assert( 0 <= dim && dim <= d );
            ++numCritical[dim];
         }
      }
   }

#if POLYMAKE_DEBUG
   const bool debug_print = perl::get_debug_level() > 1;
   if (debug_print) {
      cout << endl;
      for (int k = 0; k <= d; ++k)
         cout << "dimension: " << k << "\t# critical faces: " << numCritical[k] << endl;
   }
#endif

   // get critical faces
   //  Note: this is not merged with the above code, since this code should
   //  also work for arbitrary complexes in which we cannot deduce the dimension
   //  from the vertex sets of the faces (as they are used in the following):
    
   PowerSet<int> criticalFaces = findCriticalFaces(M, EM);

   // write vector
   p.take("MORSE_MATCHING.CRITICAL_FACE_VECTOR") << numCritical;
   // write faces
   p.take("MORSE_MATCHING.CRITICAL_FACES") << as_array(criticalFaces);
}

Function4perl(&morse_matching_critical_faces, "morse_matching_critical_faces($)");
                  
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
