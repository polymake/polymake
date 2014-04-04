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

#include "polymake/ideal/singularTermOrderData.h"


namespace polymake {
namespace ideal {
namespace singular{


template<>
SingularTermOrderData<Matrix<int> >::SingularTermOrderData(const Ring<>& polymakeRing, const Matrix<int>& orderMatrix){
   initData(polymakeRing);
   OrderData = orderMatrix;
   int nvars = polymakeRing.n_vars();
   ord[0] = ringorder_M;
   wvhdl = (int**)omalloc0((ord_size+1)*sizeof(int*));
   wvhdl[0] = (int*)omalloc0(nvars*nvars*sizeof(int));
   for(int i =0; i<nvars; i++){
      for(int j = 0; j<nvars; j++){
         wvhdl[0][i*nvars+j] = orderMatrix(i,j);
      }
   }
}

template<>
SingularTermOrderData<Vector<int> >::SingularTermOrderData(const Ring<>& polymakeRing, const Vector<int>& weightVector){
   initData(polymakeRing);
   OrderData = weightVector;
   int nvars = polymakeRing.n_vars();
   ord[0]=ringorder_wp;
   wvhdl[0]=(int*)omalloc0(nvars*sizeof(int));
   for(int i =0; i<nvars; i++){
      wvhdl[0][i] = weightVector[i];
   }
   wvhdl[1]=NULL;
   wvhdl[2]=NULL;
}

template<>
SingularTermOrderData<std::string>::SingularTermOrderData(const Ring<>& polymakeRing, const std::string& singularOrder){
   initData(polymakeRing);
   cout << polymakeRing.n_vars() << " " << singularOrder << endl;
   OrderData = singularOrder;
   ord[0] = ringorder_dp;
}


int StringToSingularTermOrder(std::string ringOrderName){
   return ringorder_dp;
}

} // end namespace singular
} // end namespace ideal
} // end namespace polymake


