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

#ifndef POLYMAKE_IDEAL_SINGULAR_TERM_ORDER_DATA_H
#define POLYMAKE_IDEAL_SINGULAR_TERM_ORDER_DATA_H

#include "polymake/client.h"
#include "polymake/Ring.h"
#include "polymake/Polynomial.h"

#include "Singular/libsingular.h"

namespace polymake { 
namespace ideal {
namespace singular {

int StringToSingularTermOrder(std::string ringOrderName);

template<typename OrderType>
class SingularTermOrderData_base{

protected:
   const OrderType orderData;
   const Ring<> polymakeRing;

public:
   // Constructor:
   SingularTermOrderData_base(const Ring<>& r, const OrderType& order) : orderData(order), polymakeRing(r){
      int nvars = polymakeRing.n_vars();
      if(nvars == 0) 
         throw std::runtime_error("Given ring is not a polynomial ring.");
   }
   // Comparison:
   friend bool operator==(const SingularTermOrderData_base<OrderType> &data1, const SingularTermOrderData_base<OrderType> &data2){
      //return true;
      return(data1.orderData == data2.orderData);
   }
   // Getters:
   // Not depending on template
   int get_ord_size() const;
   int* get_block0() const;
   int* get_block1() const;
   // Depending on template
   int** get_wvhdl() const;
   int* get_ord() const;

};

template <typename OrderType> 
class SingularTermOrderData : 
public SingularTermOrderData_base<OrderType> {
public:
//   SingularTermOrderData(const Ring<>& r, const OrderType& order) : SingularTermOrderData_base<OrderType>(r,order) {}
};

// Methods that do not depend on template
template<typename OrderType>
int SingularTermOrderData_base<OrderType>::get_ord_size() const {
   return 2;
}

template<typename OrderType>
int* SingularTermOrderData_base<OrderType>::get_block0() const {
   int ord_size = this->get_ord_size();
   int* block0 = (int*)omalloc0((ord_size+1)*sizeof(int));
   block0[0]=1;
   block0[1]=0;
   block0[2]=0;
   return block0;
}

template<typename OrderType>
int* SingularTermOrderData_base<OrderType>::get_block1() const {
   int ord_size = this->get_ord_size();
   int nvars = polymakeRing.n_vars();
   int* block1 = (int*)omalloc0((ord_size+1)*sizeof(int));
   block1[0]=nvars;
   block1[1]=0;
   block1[2]=0;
   return block1;
}




// Methods that do depend on template


template<typename Scalar> class SingularTermOrderData<Matrix<Scalar> > : public SingularTermOrderData_base<Matrix<Scalar> > {
public:
   SingularTermOrderData(const Ring<>& r, const Matrix<Scalar>& order) : SingularTermOrderData_base< Matrix<Scalar> >(r,order) {}

   int* get_ord() const {
      int ord_size = this->get_ord_size();
      int* ord=(int*)omalloc0((ord_size+1)*sizeof(int));
      ord[1]=ringorder_c;
      ord[2]=0;
      ord[0] = ringorder_M;
      return ord;
   }

   int** get_wvhdl() const {
      int ord_size = this->get_ord_size();
      int nvars = this->polymakeRing.n_vars();
      int** wvhdl=(int**)omalloc0((ord_size+1)*sizeof(int*));
      wvhdl = (int**)omalloc0((ord_size+1)*sizeof(int*));
      wvhdl[0] = (int*)omalloc0(nvars*nvars*sizeof(int));
      for(int i =0; i<nvars; i++){
         for(int j = 0; j<nvars; j++){
            wvhdl[0][i*nvars+j] = (int)this->orderData(i,j);
         }
      }
      return wvhdl;
   }
};

template<typename Scalar> class SingularTermOrderData<Vector<Scalar> > : public SingularTermOrderData_base<Vector<Scalar> > {
public:
   SingularTermOrderData(const Ring<>& r, const Vector<Scalar>& order) : SingularTermOrderData_base< Vector<Scalar> >(r,order) {}

   int* get_ord() const {
      int ord_size = this->get_ord_size();
      int* ord=(int*)omalloc0((ord_size+1)*sizeof(int));
      ord[1]=ringorder_c;
      ord[2]=0;
      ord[0] = ringorder_wp;
      return ord;
   }

   int** get_wvhdl() const {
      int ord_size = this->get_ord_size();
      int nvars = this->polymakeRing.n_vars();
      int** wvhdl=(int**)omalloc0((ord_size+1)*sizeof(int*));
      wvhdl[0]=(int*)omalloc0(nvars*sizeof(int));
      for(int i =0; i<nvars; i++){
         wvhdl[0][i] = (int) this->orderData[i];
      }
      wvhdl[1]=NULL;
      wvhdl[2]=NULL;
      return wvhdl;
   }
};

template<> class SingularTermOrderData<std::string> : public SingularTermOrderData_base<std::string> {
public:
   SingularTermOrderData(const Ring<>& r, const std::string& order) : SingularTermOrderData_base<std::string>(r,order) {}
   int* get_ord() const {
      int ord_size = this->get_ord_size();
      int* ord=(int*)omalloc0((ord_size+1)*sizeof(int));
      ord[1]=ringorder_c;
      ord[2]=0;
      ord[0] = StringToSingularTermOrder(orderData);
      return ord;
   }

   int** get_wvhdl() const {
      int ord_size = this->get_ord_size();
      int** wvhdl=(int**)omalloc0((ord_size+1)*sizeof(int*));
      return wvhdl;
   }
};


} // end namespace singular
} // end namespace ideal
} // end namespace polymake

#endif

