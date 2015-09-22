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

#ifndef ORBITS_H
#define ORBITS_H

#include <polymake/Array.h>
#include <polymake/Set.h>
#include <polymake/Bitset.h>
#include <polymake/Matrix.h>
#include <polymake/ListMatrix.h>
#include <polymake/Polynomial.h>
#include <polymake/permutations.h>
#include <queue>

namespace pm{
namespace operations{
namespace group{

struct on_container{};
struct on_elements{};
struct on_rows{};
struct on_cols{};

template <typename OpRef, typename action_type, typename PERM, 
typename op_tag=typename object_traits<typename deref<OpRef>::type>::generic_tag, 
typename perm_tag=typename object_traits<PERM>::generic_tag,
typename enabled=True
>
struct action;

// generic action on container with Array<int>
template <typename OpRef, typename PERM, typename op_tag>
struct action<OpRef, on_container, PERM, op_tag, is_container> {
  typedef OpRef argument_type;
  typedef typename deref<argument_type>::type result_type;

  const PERM& perm;
  
  action(const PERM& p)
   : perm(p) {}
  
  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return permuted(x,perm);
  }      
}; 

// action on integers (anchor of the on_elements recursion)
template <typename PERM>
struct action<int&, on_elements, PERM, is_scalar, is_container> {
  typedef int& argument_type;
  typedef typename deref<argument_type>::type result_type;
  
  const PERM& perm;
  
  action(const PERM& p)
    : perm(p) {}
  
  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return perm[x];
  }      
}; 

// generic action on elements in a container with Array<int>
template <typename OpRef, typename PERM, typename op_tag>
  struct action<OpRef, on_elements, PERM, op_tag, is_container,
  typename enable_if<True, (identical< typename object_traits<typename deref<OpRef>::type>::model, is_container>::value && !identical<op_tag, is_matrix>::value) >::type
  > {
  typedef OpRef argument_type;
  typedef typename object_traits<typename deref<argument_type>::type>::persistent_type result_type;


  const PERM& perm;

  action(const PERM& p)
   : perm(p) {}

  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return result_type(attach_operation(x,action<typename deref<argument_type>::type::value_type&, on_elements, PERM>(perm)));
  }      
}; 

// generic action on elements in a matrix with Array<int>
template <typename OpRef, typename PERM>
  struct action<OpRef, on_elements, PERM, is_matrix, is_container> {
  typedef OpRef argument_type;
  typedef typename object_traits<typename deref<argument_type>::type>::persistent_type result_type;


  const PERM& perm;

  action(const PERM& p)
   : perm(p) {}

  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return result_type(x.rows(),x.cols(),entire(attach_operation(concat_rows(x),action<typename deref<argument_type>::type::value_type&, on_elements, PERM>(perm))));
  }      
}; 

// action on rows of a matrix with Array<int>
template <typename OpRef, typename PERM>
struct action<OpRef, on_rows, PERM, is_matrix, is_container> {
  typedef OpRef argument_type;
  typedef typename deref<argument_type>::type result_type;

  const PERM& perm;

  action(const PERM& p)
   : perm(p) {}

  result_type operator() (typename function_argument< argument_type >::const_type x) const
  {
    return permuted_rows(x,perm);
  }
};

// action on cols of a matrix with Array<int>
template <typename OpRef, typename PERM>
struct action<OpRef, on_cols, PERM, is_matrix, is_container> {
  typedef OpRef argument_type;
  typedef typename deref<argument_type>::type result_type;
    
  const PERM& perm;

  action(const PERM& p)
   : perm(p) {}

  result_type operator() (typename function_argument< argument_type >::const_type x) const
  {
    return permuted_cols(x,perm);
  }      
}; 

// action on a vector under a matrix group
template <typename OpRef, typename PERM>
struct action<OpRef, on_elements, PERM, is_vector, is_matrix> {
  typedef OpRef argument_type;
  typedef typename deref<argument_type>::type result_type;
    
  const PERM& mat;
    
  action(const PERM& m)
   : mat(m.top()) {}
    
  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return mat*x;
  }
};

// action on the variables of a polyomial with Array<int>
template <typename PERM, typename Coefficient, typename Exponent>
struct action<Polynomial<Coefficient,Exponent>&, on_container, PERM, is_polynomial, is_container> {
  typedef Polynomial<Coefficient, Exponent>& argument_type;
  typedef typename deref<argument_type>::type result_type;

  const PERM& perm;

  action(const PERM& p)
   : perm(p) {}

  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return Polynomial<Coefficient, Exponent>(action<Matrix<Exponent>, on_cols, PERM>(perm)(x.monomials_as_matrix()), x.coefficients_as_vector(), x.get_ring());
  }      
}; 

// action on the variables of a monomial with Array<int>
template <typename PERM, typename Coefficient, typename Exponent>
struct action<Monomial<Coefficient,Exponent>&, on_container, PERM, is_opaque, is_container> {
  typedef Monomial<Coefficient,Exponent>& argument_type;
  typedef typename deref<argument_type>::type result_type;

  const PERM& perm;

  action(const PERM& p)
   : perm(p) {}

  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return Monomial<Coefficient,Exponent>(action<Vector<Exponent>, on_container, PERM>(perm)(x.get_value()), x.get_ring());
  }      
};

// generic action on both elements of a pair
template <typename E1, typename E2, typename action_type, typename PERM, typename perm_tag>
struct action<std::pair<E1,E2>&, action_type, PERM, is_composite, perm_tag> {
  typedef std::pair<E1,E2>& argument_type;
  typedef typename deref<argument_type>::type result_type;

  const PERM& perm;

  action(const PERM& p)
   : perm(p) {}

  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return std::make_pair<E1,E2>(action<E1&, action_type, PERM>(perm)(x.first),action<E2&, action_type, PERM>(perm)(x.second));
  }      
}; 

} //end namespace group
} //end namespace operations

} //end namespace pm

namespace polymake {
namespace group {

using pm::operations::group::on_container;
using pm::operations::group::on_elements;
using pm::operations::group::on_rows;
using pm::operations::group::on_cols;

/*
 * computes the action on something under one permutation element
 */
template <typename action_type, typename PERM, typename Element>
typename pm::object_traits<Element>::persistent_type
action(const PERM& perm, const Element& element) 
{
  return pm::operations::group::action<Element&,action_type,PERM>(perm)(element);
}

/*
 * computes the action on something under the inverse of a Array<int>
 */
template <typename action_type, typename Element>
typename pm::object_traits<Element>::persistent_type
action_inv(const Array<int>& perm, const Element& element) 
{
  Array<int> inv(perm.size());
  inverse_permutation(perm, inv);
  return pm::operations::group::action<Element&,action_type,Array<int> >(inv)(element);
}


/*
 * Comutes the orbit of element, where the group is spanned by generators
 */
template<typename action_type, typename PERM, typename Element>
Set< Element > orbit(const Array< PERM >& generators, const Element& element) {
  Set< Element > orbit;
  orbit += element;
  std::queue< Element > q;
  q.push(element);
  while(!q.empty()) {
    Element orbitElement = q.front();
    q.pop();
    for(typename Entire<Array< PERM > >::const_iterator generator = entire(generators); !generator.at_end(); ++generator) {
      Element next = action<action_type>(*generator,orbitElement);
      if(!orbit.collect(next)) {
        q.push(next);
      }
    }
  }
  return orbit;
}

}
}

#endif
