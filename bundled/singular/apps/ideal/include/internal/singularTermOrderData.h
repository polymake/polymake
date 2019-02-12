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

#ifndef POLYMAKE_IDEAL_SINGULAR_TERM_ORDER_DATA_H
#define POLYMAKE_IDEAL_SINGULAR_TERM_ORDER_DATA_H

#include "polymake/client.h"
#include "polymake/Polynomial.h"

#include "polymake/ideal/internal/singularInclude.h"

namespace polymake { 
namespace ideal {
namespace singular {

// since singular commit 90f715a0b0 the type of the term order is rRingOrder_t* instead of int*
// in various functions (e.g. rDefault) and as member for ip_sring
// since there is no version number to determine this we use the type of the member here
typedef typename std::remove_pointer<decltype(ip_sring::order)>::type singular_order_type;

singular_order_type StringToSingularTermOrder(std::string ringOrderName);

template <typename OrderType>
class SingularTermOrderData_base {

protected:
   const OrderType orderData;
   const int n_vars;

public:
   // Constructor:
   SingularTermOrderData_base(const int n_vars_, const OrderType& order)
     : orderData(order)
     , n_vars(n_vars_)
   {
      if (n_vars == 0) 
         throw std::runtime_error("Given ring is not a polynomial ring.");
   }

   // Comparison:
   bool operator== (const SingularTermOrderData_base<OrderType>& other)
   {
      return orderData == other.orderData;
   }

   // Getters:
   // Not depending on template
   int get_ord_size() const;
   int* get_block0() const;
   int* get_block1() const;
   // Depending on template
   int** get_wvhdl() const;
   singular_order_type* get_ord() const;

  const OrderType& get_orderData() const { return orderData; }
};

template <typename OrderType> 
class SingularTermOrderData : public SingularTermOrderData_base<OrderType> {
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
   int* block1 = (int*)omalloc0((ord_size+1)*sizeof(int));
   block1[0]=n_vars;
   block1[1]=0;
   block1[2]=0;
   return block1;
}




// Methods that do depend on template


template <typename Scalar>
class SingularTermOrderData<Matrix<Scalar>> : public SingularTermOrderData_base<Matrix<Scalar>> {
public:
   using SingularTermOrderData_base<Matrix<Scalar>>::SingularTermOrderData_base;

   int get_ord_size() const {
     return this->orderData.rows()+1;
   }

   int* get_block0() const {
      int ord_size = this->get_ord_size();
      int* block0 = (int*)omalloc0((ord_size+2)*sizeof(int));
      for(int i = 0; i < ord_size; ++i) {
        block0[i] = 1;
      }
      block0[ord_size]=0;
      block0[ord_size+1]=0;
      return block0;
   }

   int* get_block1() const {
      int ord_size = this->get_ord_size();
      int nvars = this->n_vars;
      int* block1 = (int*)omalloc0((ord_size+2)*sizeof(int));
      for(int i = 0; i < ord_size; ++i) {
        block1[i] = nvars;
      }
      block1[ord_size]=0;
      block1[ord_size+1]=0;
      return block1;
   }

   singular_order_type* get_ord() const {
      int ord_size = this->get_ord_size();
      singular_order_type* ord=(singular_order_type*)omalloc0((ord_size+2)*sizeof(singular_order_type));
      for(int i = 0; i < ord_size-1; ++i) {
        ord[i] = ringorder_a;
      }
      ord[ord_size-1] = ringorder_lp;
      ord[ord_size] = ringorder_c;
      return ord;
   }

   int** get_wvhdl() const {
      int ord_size = this->get_ord_size();
      int nvars = this->n_vars;
      int** wvhdl=(int**)omalloc0((ord_size+2)*sizeof(int*));
      for(int i =0; i<ord_size-1; i++){
         wvhdl[i] = (int*)omalloc0(nvars*sizeof(int));
         for(int j = 0; j<nvars; j++){
            wvhdl[i][j] = (int)this->orderData(i,j);
         }
      }
      wvhdl[ord_size-1]=NULL;
      wvhdl[ord_size]=NULL;
      wvhdl[ord_size+1]=NULL;
      return wvhdl;
   }
};

template <typename Scalar>
class SingularTermOrderData<Vector<Scalar>> : public SingularTermOrderData_base<Vector<Scalar> > {
public:
   using SingularTermOrderData_base<Vector<Scalar>>::SingularTermOrderData_base;

   singular_order_type* get_ord() const {
      int ord_size = this->get_ord_size();
      singular_order_type* ord=(singular_order_type*)omalloc0((ord_size+1)*sizeof(singular_order_type));
      ord[1]=ringorder_c;
      ord[0] = ringorder_wp;
      return ord;
   }

   int** get_wvhdl() const {
      int ord_size = this->get_ord_size();
      int nvars = this->n_vars;
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
   using SingularTermOrderData_base<std::string>::SingularTermOrderData_base;

   singular_order_type* get_ord() const {
      int ord_size = this->get_ord_size();
      singular_order_type* ord=(singular_order_type*)omalloc0((ord_size+1)*sizeof(singular_order_type));
      ord[1]=ringorder_c;
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

namespace pm {
namespace operations {

// for use in Maps
template <typename OrderType>
struct cmp_opaque<polymake::ideal::singular::SingularTermOrderData<OrderType>, void> {
  typedef polymake::ideal::singular::SingularTermOrderData<OrderType> first_argument_type;
  typedef first_argument_type second_argument_type;
  typedef cmp_value result_type;

  result_type operator() (const first_argument_type& l, const second_argument_type& r) const
  {
    return operations::cmp()(l.get_orderData(), r.get_orderData());
  }
};

} }

#endif

