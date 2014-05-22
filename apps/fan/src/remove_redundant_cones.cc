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
   f.take("MAXIMAL_CONES")<<max_cones;
}

Function4perl(&remove_redundant_cones,"remove_redundant_cones(PolyhedralFan) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
