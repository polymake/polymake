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

#ifndef __GROUP_ACTION_H
#define __GROUP_ACTION_H

#include "polymake/hash_map"
#include "polymake/Array.h"
#include <polymake/IncidenceMatrix.h>
#include <polymake/Polynomial.h>
#include <polymake/permutations.h>

namespace polymake { namespace group {


inline      
int degree(const Array<int>& a)
{
      return a.size();
}

template<typename Scalar>
int degree(const Matrix<Scalar>& a)
{
   assert(a.rows() == a.cols());
   return a.rows();
}

inline      
auto identity(int degree, const Array<int>&)
{
   return Array<int>(degree, entire(sequence(0, degree)));
}

template<typename Scalar>
auto identity(int degree, const Matrix<Scalar>&)
{
   return unit_matrix<Scalar>(degree);
}

inline      
auto inverse(const Array<int>& g)
{
   Array<int> inverse;
   inverse_permutation(g, inverse);
   return inverse;
}

template<typename Scalar>
auto inverse(const Matrix<Scalar>& g)
{
   return inv(g);
}

} }

namespace pm {
namespace operations {
namespace group {

struct on_container{};
struct on_nonhomog_container{};
struct on_elements{};
struct on_rows{};
struct on_cols{};
struct on_nonhomog_cols{};

template <typename OpRef, typename action_type, typename Perm, 
          typename op_tag=typename object_traits<typename deref<OpRef>::type>::generic_tag, 
          typename perm_tag=typename object_traits<Perm>::generic_tag,
          typename stores_ref=std::true_type,
          typename enabled=std::true_type>
struct action;

template <typename OpRef, typename action_type, typename Perm, 
          typename op_tag=typename object_traits<typename deref<OpRef>::type>::generic_tag, 
          typename perm_tag=typename object_traits<Perm>::generic_tag,
          typename stores_ref=std::true_type,
          typename enabled=std::true_type>
struct right_action;

template <typename OpRef, typename action_type, typename Perm, 
          typename op_tag=typename object_traits<typename deref<OpRef>::type>::generic_tag, 
          typename perm_tag=typename object_traits<Perm>::generic_tag,
          typename stores_ref=std::true_type>
struct conjugation_action;
   
// generic action on container with Array<int>
template <typename OpRef, typename Perm, typename op_tag, typename stores_ref>
struct action<OpRef, on_container, Perm, op_tag, is_container, stores_ref> {
   typedef OpRef argument_type;
   typedef typename deref<argument_type>::type result_type;

   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type perm;
  
   action(const Perm& p)
      : perm(p) {}
  
   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return permuted(x,perm);
   }      
}; 

// generic action by conjugation
template <typename OpRef, typename action_type, typename Perm, typename op_tag, typename perm_tag, typename stores_ref>
struct conjugation_action {
   typedef OpRef argument_type;
   typedef typename deref<argument_type>::type result_type;

         action<OpRef, action_type, Perm, op_tag, perm_tag, stores_ref>  left;
   right_action<OpRef, action_type, Perm, op_tag, perm_tag, stores_ref> right;
  
   conjugation_action(const Perm& p)
      : left(p), right(polymake::group::inverse(p)) {}
  
   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return left(right(x));
   }      
}; 
   
// generic non-homogeneous action on container with Array<int>
template <typename OpRef, typename Perm, typename op_tag>
struct action<OpRef, on_nonhomog_container, Perm, op_tag, is_container> {
  typedef OpRef argument_type;
  typedef typename deref<argument_type>::type result_type;

  const Perm perm;
  
  action(const Perm& p)
   : perm(concatenate(single_value_as_container(0), translate(p, 1))) {}
  
  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return permuted(x,perm);
  }      
}; 

// action on integers (anchor of the on_elements recursion)
template <typename Perm, typename stores_ref>
struct action<int&, on_elements, Perm, is_scalar, is_container, stores_ref> {
   typedef int& argument_type;
   typedef typename deref<argument_type>::type result_type;
  
   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type perm;

   action(const Perm& p)
      : perm(p) {}
  
   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return perm[x];
   }      
}; 

// generic action on elements in a container with Array<int>
template <typename OpRef, typename Perm, typename op_tag, typename stores_ref>
struct action<OpRef, on_elements, Perm, op_tag, is_container, stores_ref,
              typename std::enable_if<std::is_same<typename object_traits<typename deref<OpRef>::type>::model, is_container>::value
                                      && !std::is_same<op_tag, is_matrix>::value, std::true_type>::type
              > {
   typedef OpRef argument_type;
   typedef typename object_traits<typename deref<argument_type>::type>::persistent_type result_type;

   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type perm;

   action(const Perm& p)
      : perm(p) {}

   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return result_type(attach_operation(x,action<typename deref<argument_type>::type::value_type&, on_elements, Perm>(perm)));
   }      
}; 

// generic action on elements in a matrix with Array<int>
template <typename OpRef, typename Perm, typename stores_ref>
struct action<OpRef, on_elements, Perm, is_matrix, is_container, stores_ref> {
   typedef OpRef argument_type;
   typedef typename object_traits<typename deref<argument_type>::type>::persistent_type result_type;

   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type perm;

   action(const Perm& p)
      : perm(p) {}

   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return result_type(x.rows(),x.cols(),entire(attach_operation(concat_rows(x),action<typename deref<argument_type>::type::value_type&, on_elements, Perm>(perm))));
   }      
}; 

// action on rows of a matrix with Array<int>
template <typename OpRef, typename Perm, typename stores_ref>
struct action<OpRef, on_rows, Perm, is_matrix, is_container, stores_ref> {
   typedef OpRef argument_type;
   typedef typename deref<argument_type>::type result_type;

   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type perm;

   action(const Perm& p)
      : perm(p) {}

   result_type operator() (typename function_argument< argument_type >::const_type x) const
   {
      return permuted_rows(x,perm);
   }
};

// action on cols of a matrix with Array<int>
template <typename OpRef, typename Perm, typename stores_ref>
struct action<OpRef, on_cols, Perm, is_matrix, is_container, stores_ref> {
   typedef OpRef argument_type;
   typedef typename deref<argument_type>::type result_type;
    
   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type perm;

   action(const Perm& p)
      : perm(p) {}

   result_type operator() (typename function_argument< argument_type >::const_type x) const
   {
      return permuted_cols(x,perm);
   }      
}; 

// non-homogeneous action on the cols of a matrix with Array<int>
template <typename OpRef, typename Perm>
struct action<OpRef, on_nonhomog_cols, Perm, is_matrix, is_container> {
   typedef OpRef argument_type;
   typedef typename deref<argument_type>::type result_type;
    
   const Perm perm;

   action(const Perm& p)
      : perm(concatenate(single_value_as_container(0), translate(p, 1))) {}

   result_type operator() (typename function_argument< argument_type >::const_type x) const
   {
      return permuted_cols(x,perm);
   }      
}; 

// action on cols of an IncidenceMatrix with Array<int>
template <typename Perm, typename stores_ref>
struct action<IncidenceMatrix<>, on_container, Perm, is_incidence_matrix, is_container, stores_ref> {
   typedef IncidenceMatrix<> argument_type;
   typedef typename deref<argument_type>::type result_type;
    
   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type perm;

   action(const Perm& p)
      : perm(p) {}

   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return permuted_cols(x,perm);
   }      
}; 

// action on a vector under a matrix group
template <typename OpRef, typename Perm, typename stores_ref>
struct action<OpRef, on_elements, Perm, is_vector, is_matrix, stores_ref> {
   typedef OpRef argument_type;
   typedef typename deref<argument_type>::type result_type;
    
   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type mat;
    
   action(const Perm& m)
      : mat(m.top()) {}
    
   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return mat*x;
   }
};

// right action on a matrix under a matrix group
template <typename OpRef, typename Perm, typename stores_ref>
struct right_action<OpRef, on_elements, Perm, is_matrix, is_matrix, stores_ref> {
   typedef OpRef argument_type;
   typedef typename deref<argument_type>::type result_type;
    
   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type mat;
    
   right_action(const Perm& m)
      : mat(m.top()) {}
    
   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return x*mat;
   }
};

// right action on a permutation under a permutation group
template <typename OpRef, typename Perm, typename stores_ref>
struct right_action<OpRef, on_container, Perm, is_container, is_container, stores_ref> {
   typedef OpRef argument_type;
   typedef typename deref<argument_type>::type result_type;
    
   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type perm;
    
   right_action(const Perm& p)
      : perm(p) {}
    
   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return permuted(perm,x);
   }
};
   
// action on a matrix under a matrix group
template <typename OpRef, typename Perm, typename stores_ref>
struct action<OpRef, on_elements, Perm, is_matrix, is_matrix, stores_ref> {
   typedef OpRef argument_type;
   typedef typename deref<argument_type>::type result_type;
    
   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type mat;
    
   action(const Perm& m)
      : mat(m.top()) {}
    
   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return mat*x;
   }
};
   
// action on the variables of a polyomial with Array<int>
template <typename Perm, typename Coefficient, typename Exponent, typename stores_ref>
struct action<Polynomial<Coefficient,Exponent>&, on_container, Perm, is_polynomial, is_container, stores_ref> {
   typedef Polynomial<Coefficient, Exponent>& argument_type;
   typedef typename deref<argument_type>::type result_type;

   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type perm;

   action(const Perm& p)
      : perm(p) {}

   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return Polynomial<Coefficient, Exponent>(x.coefficients_as_vector(), action<Matrix<Exponent>, on_cols, Perm>(perm)(x.monomials_as_matrix()));
   }      
}; 

// generic action on both elements of a pair
template <typename E1, typename E2, typename action_type, typename Perm, typename perm_tag, typename stores_ref>
struct action<std::pair<E1,E2>&, action_type, Perm, is_composite, perm_tag, stores_ref> {
   typedef std::pair<E1,E2>& argument_type;
   typedef typename deref<argument_type>::type result_type;

   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type perm;

   action(const Perm& p)
      : perm(p) {}

   result_type operator() (typename function_argument<argument_type>::const_type x) const
   {
      return std::make_pair<E1,E2>(action<E1&, action_type, Perm>(perm)(x.first),action<E2&, action_type, Perm>(perm)(x.second));
   }      
}; 

template<typename TMap, typename Perm, typename stores_ref>
struct action<TMap&, on_container, Perm, is_map, is_container, stores_ref> {
   typedef TMap& argument_type;
   typedef typename deref<argument_type>::type result_type;

   typename std::conditional<stores_ref::value, const Perm&, const Perm>::type perm;

   action(const Perm& p)
      : perm(p) {}

   result_type operator() (typename function_argument<argument_type>::const_type m) const
   {
      result_type img;
      const action<typename TMap::key_type, on_container, Perm> a(perm);
      for (const auto e : m)
         img[a(e.first)] = e.second;
      return img;
   }
};

} //end namespace group
} //end namespace operations

} //end namespace pm

namespace polymake { namespace group {

using pm::operations::group::on_container;
using pm::operations::group::on_nonhomog_container;
using pm::operations::group::on_elements;
using pm::operations::group::on_rows;
using pm::operations::group::on_cols;
using pm::operations::group::on_nonhomog_cols;

/*
 * computes the action on something under one permutation element
 */
template <typename action_type, typename Perm, typename Element>
typename pm::object_traits<Element>::persistent_type
action(const Perm& perm, const Element& element) 
{
  return pm::operations::group::action<Element&, action_type, Perm>(perm)(element);
}

// a memory-efficient version of the same
template <typename Iterator, typename Permutation, typename SetType> inline
void permute_to(Iterator in_it,          // deliberately no reference, so we can increment it inside the function
                const Permutation& perm,
                SetType& out)
{
   out.clear();
   while (!in_it.at_end()) {
      out += perm[*in_it];
      ++in_it;
   }
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

} }

#endif // __GROUP_ACTION_H


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:


