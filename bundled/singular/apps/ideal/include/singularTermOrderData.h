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

#include "polymake/client.h"
#include "polymake/Ring.h"
#include "polymake/Polynomial.h"
//#include "polymake/internal/shared_object.h"

#include <Singular/libsingular.h>

namespace polymake { 
namespace ideal {
namespace singular {

template<typename OrderType>
class SingularTermOrderData{

    OrderType OrderData;
    int ord_size;
   int *ord;
   int *block0;
   int *block1;
   int **wvhdl;

private:
   void initData(const  Ring<>& polymakeRing){
      ord_size = 2;
      int nvars = polymakeRing.n_vars();
      if(nvars == 0) 
         throw std::runtime_error("Given ring is not a polynomial ring.");
      // Create variables:
      ord=(int*)omalloc0((ord_size+1)*sizeof(int));
      block0=(int*)omalloc0((ord_size+1)*sizeof(int));
      block1=(int*)omalloc0((ord_size+1)*sizeof(int));
      wvhdl=(int**)omalloc0((ord_size+1)*sizeof(int*));
      ord[1]=ringorder_c;
      ord[2]=0;
      block0[0]=1;
      block0[1]=0;
      block0[2]=0;
      block1[0]=nvars;
      block1[1]=0;
      block1[2]=0;
   };

public:
   // Constructor:
   SingularTermOrderData(const Ring<>& r, const OrderType& order);
   // Comparison:
   friend bool operator==(const SingularTermOrderData<OrderType> &data1, const SingularTermOrderData<OrderType> &data2){
      //return true;
      return(data1.OrderData == data2.OrderData);
   }
   // Getters:
   int get_ord_size() const {
      return ord_size;
   };
   int* get_ord() const {
      return ord;
   }
   int* get_block0() const {
      return block0;
   }
   int* get_block1() const {
      return block1;
   }
   int** get_wvhdl() const {
      return wvhdl;
   }

};

} // end namespace singular
} // end namespace ideal
} // end namespace polymake


