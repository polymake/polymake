/* Copyright (c) 1997-2018
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
#include "polymake/TropicalNumber.h"
#include "polymake/linalg.h"
#include "polymake/permutations.h"

namespace polymake { namespace matroid {

    template <typename Addition, typename Scalar, typename Iterator> inline
    void canonicalize_by_iterator(Iterator e) {
      if (!e.at_end() && (*e) != TropicalNumber<Addition,Scalar>::one()) {
	  const typename Iterator::value_type leading=*e;
	  do *e /= leading; while (!(++e).at_end());
      }
    }
  
    
    template <typename Vector, typename Addition, typename Scalar> inline
    void canonicalize_tropical_rays(GenericVector<Vector,TropicalNumber<Addition,Scalar> >& V)
    {
      canonicalize_by_iterator<Addition,Scalar>(find_in_range_if(entire(V.top()), operations::non_zero()));
    }

    template <typename Matrix, typename Addition, typename Scalar> inline
    void canonicalize_tropical_rays(GenericMatrix<Matrix,TropicalNumber<Addition,Scalar> >& M)
    {	
      for (typename Entire< Rows<Matrix> >::iterator r=entire(rows(M)); !r.at_end();  ++r)
	  canonicalize_tropical_rays(r->top());
    }
    



    
    
 
    FunctionTemplate4perl("canonicalize_tropical_rays(Vector&) : void");
    
    FunctionTemplate4perl("canonicalize_tropical_rays(Matrix&) : void");
    
 
}
}
