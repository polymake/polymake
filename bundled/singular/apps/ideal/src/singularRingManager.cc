/* Copyright (c) 2012
   by authors as mentioned on:
   https://github.com/lkastner/polymake_algebra/wiki/Authors

   Project home:
   https://github.com/lkastner/polymake_algebra

   For licensing we cite the original Polymake code:

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version: http://www.gnu.org/licenses/gpl.txt.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <dlfcn.h>

#include "polymake/ideal/singularRingManager.h"

namespace polymake {
namespace ideal {
namespace singular{

    unsigned int ringidcounter = 0;
    SingularTermOrderMap stom_new;

// If no monomial ordering is given:
idhdl check_ring(const Ring<> r){
   std::string ord = "dp";
   SingularTermOrderData<std::string > TO = SingularTermOrderData<std::string >(r, ord); 
   return check_ring(r, TO);
}

idhdl check_ring(idhdl singRing) {
   init_singular();
   rSetHdl(singRing);
   return singRing;
}

} // end singular namespace
} // end namespace ideal
} // end namespace polymake


