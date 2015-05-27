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

#ifndef POLYMAKE_IDEAL_INTERNAL_SINGULAR_RING_MANAGER_H
#define POLYMAKE_IDEAL_INTERNAL_SINGULAR_RING_MANAGER_H


// polymake includes
#include "polymake/client.h"
#include "polymake/Map.h"

#include <Singular/libsingular.h>
#include "polymake/ideal/internal/singularTermOrderData.h"
#include "polymake/ideal/internal/singularTermOrderMap.h"
#include "polymake/ideal/internal/singularConvertTypes.h"
#include "polymake/ideal/singularInit.h"

namespace polymake { 
namespace ideal {
namespace singular {

   extern unsigned int ringidcounter;
   extern SingularTermOrderMap stom_new;

   // Check ring routines:

   idhdl check_ring(idhdl singularRinghdl);
   // idhdl check_ring(const Ring<> r, const Matrix<int> order);
   idhdl check_ring(const Ring<> r);
    
   template <typename OrderType>
   idhdl check_ring(const Ring<> polymakeRing, SingularTermOrderData<OrderType> termOrder) {
      init_singular();
      Ring<>::id_type id = polymakeRing.id();
      std::pair<Ring<>::id_type, SingularTermOrderData<OrderType > > pair_to(id, termOrder);
      if(!stom_new.exists(pair_to)){
           int nvars = polymakeRing.n_vars();
           if(nvars == 0) 
             throw std::runtime_error("Given ring is not a polynomial ring.");
           // Create variables:
           char **variableNames=(char**)omalloc(nvars*sizeof(char*));
           for(int i=0; i<nvars; i++)
           {
             variableNames[i] = omStrDup(polymakeRing.names()[i].c_str());
           }
           int ord_size = termOrder.get_ord_size();
           int *ord = termOrder.get_ord();
           int *block0 = termOrder.get_block0();
           int *block1 = termOrder.get_block1();
           int **wvhdl = termOrder.get_wvhdl();
           ring singularRing = rDefault(0,nvars,variableNames,ord_size,ord,block0,block1,wvhdl);
           // ceil(log_10(2))=3
           char* ringid = (char*) malloc(2+3*sizeof(unsigned int)+1);
           sprintf(ringid,"R_%0u",ringidcounter++); 
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

#endif

