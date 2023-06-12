/* Copyright (c) 1997-2023
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

#pragma once


// polymake includes
#include "polymake/client.h"
#include "polymake/Map.h"

#include "polymake/ideal/internal/singularInclude.h"

#include "polymake/ideal/internal/singularTermOrderData.h"
#include "polymake/ideal/internal/singularTermOrderMap.h"
#include "polymake/ideal/internal/singularConvertTypes.h"
#include "polymake/ideal/internal/singularUtils.h"
#include "polymake/ideal/singularInit.h"

namespace polymake { 
namespace ideal {
namespace singular {

   extern unsigned int ringidcounter;
   extern SingularTermOrderMap stom_new;

   // Check ring routines:

   idhdl check_ring(idhdl singularRinghdl);
   // idhdl check_ring(const Ring<> r, const Matrix<int> order);
   idhdl check_ring(const int n_vars);
    
   template <typename OrderType>
   idhdl check_ring(const int n_vars, SingularTermOrderData<OrderType> termOrder) {
      init_singular();
      std::pair<int, SingularTermOrderData<OrderType > > pair_to(n_vars, termOrder);
      if(!stom_new.exists(pair_to)){
           if(n_vars == 0) 
             throw std::runtime_error("Given ring is not a polynomial ring.");
           // Create variables:
           char **variableNames=(char**)omalloc(n_vars*sizeof(char*));
           for(int i=0; i<n_vars; i++)
           {
             variableNames[i] = omStrDup(("x_" + std::to_string(i)).c_str());
           }
           int ord_size = safe_cast(termOrder.get_ord_size());
           singular_order_type *ord = termOrder.get_ord();
           int *block0 = termOrder.get_block0();
           int *block1 = termOrder.get_block1();
           int **wvhdl = termOrder.get_wvhdl();
           ring singularRing = rDefault(0,n_vars,variableNames,ord_size,ord,block0,block1,wvhdl);
           // ceil(log_10(2))=3
           size_t idlen = 2+3*sizeof(unsigned int);
           char* ringid = (char*) malloc(idlen+1);
           snprintf(ringid, idlen+1, "R_%0u", ringidcounter++);
           // Create handle for ring:
           idhdl newRingHdl=enterid(ringid,0,RING_CMD,&IDROOT,FALSE);
           IDRING(newRingHdl)=singularRing;
           stom_new[pair_to] = newRingHdl;
           free(ringid);
      }
      rSetHdl(stom_new[pair_to]);
      return stom_new[pair_to];
   }


} // end namespace singular
} // end namespace ideal
} // end namespace polymake


