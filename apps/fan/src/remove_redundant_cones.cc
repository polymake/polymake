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
#include "polymake/IncidenceMatrix.h"
#include "polymake/FacetList.h"

namespace polymake { namespace fan {
  
void remove_redundant_cones(perl::Object f)
{
   const IncidenceMatrix<> i_cones = f.give("INPUT_CONES");
   const int n_i_cones=i_cones.rows();
   FacetList max_cones;
    
   for(int i=0; i<n_i_cones;++i) 
   {
      max_cones.insertMax(i_cones.row(i));
   }

   // Take care of the empty fan and the fan containing only the origin
   if(max_cones.empty() && n_i_cones > 0)
      f.take("MAXIMAL_CONES")<< IncidenceMatrix<>(1,0);
   else
      f.take("MAXIMAL_CONES")<<max_cones;
}

Function4perl(&remove_redundant_cones,"remove_redundant_cones(PolyhedralFan)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
