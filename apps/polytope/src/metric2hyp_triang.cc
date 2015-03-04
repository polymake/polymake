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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/polytope/hypersimplex.h"

namespace polymake { namespace polytope {

perl::Object metric2hyp_triang(perl::Object p_in)
{
   const IncidenceMatrix<> VIF=p_in.give("VERTICES_IN_FACETS");
   const int n_vertices=VIF.cols();
   const Set<int> far_face=p_in.give("FAR_FACE");
   const int n=far_face.size();
   
   //all simplexes of the triangulation
   PowerSet<int> triang;
   
   for (int i=0; i<n_vertices; ++i) 
      if (!far_face.contains(i)) {
         Set<int> simplex;
         bool stop=false;
         bool pstop=false;
         for (Entire< IncidenceMatrix<>::col_type >::const_iterator j=entire(VIF.col(i)); !j.at_end(); ++j) {
            if (*j>=n) simplex.insert(*j-n);
            else if (pstop || VIF.col(i).size()!=n+1) { stop=true; break; }
            else pstop=true;
         } 
         if (!stop) triang.insert(simplex);
      }
   
   perl::Object p_out=hypersimplex(2,n,perl::OptionSet());
   p_out.take("TRIANGULATION.FACETS") << triang;
   return p_out;
}

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Given a generic finite metric space //FMS//, construct the associated (i.e. dual) triangulation of the hypersimplex."
                  "# @param TightSpan FMS"
                  "# @return Polytope"
                  "# @author Sven Herrmann",
                  &metric2hyp_triang, "metric2hyp_triang(TightSpan)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
