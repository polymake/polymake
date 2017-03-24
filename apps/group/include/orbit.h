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
#include <polymake/hash_set>
#include <polymake/Matrix.h>
#include <polymake/ListMatrix.h>
#include <polymake/Polynomial.h>
#include <polymake/permutations.h>
#include <queue>

namespace pm{
namespace operations{
namespace group{

struct on_container{};
struct on_nonhomog_container{};
struct on_elements{};
struct on_rows{};
struct on_cols{};
struct on_nonhomog_cols{};

template <typename OpRef, typename action_type, typename Perm, 
          typename op_tag=typename object_traits<typename deref<OpRef>::type>::generic_tag, 
          typename perm_tag=typename object_traits<Perm>::generic_tag,
          typename enabled=std::true_type>
struct action;

// generic action on container with Array<int>
template <typename OpRef, typename Perm, typename op_tag>
struct action<OpRef, on_container, Perm, op_tag, is_container> {
  typedef OpRef argument_type;
  typedef typename deref<argument_type>::type result_type;

  const Perm& perm;
  
  action(const Perm& p)
   : perm(p) {}
  
  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return permuted(x,perm);
  }      
}; 

// generic non-homogeneous action on container with Array<int>
template <typename OpRef, typename Perm, typename op_tag>
struct action<OpRef, on_nonhomog_container, Perm, op_tag, is_container> {
  typedef OpRef argument_type;
  typedef typename deref<argument_type>::type result_type;

  const Perm perm;
  
  action(const Perm& p)
   : perm(concatenate(pm::single_value_container<int>(0), translate(p, 1))) {}
  
  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return permuted(x,perm);
  }      
}; 

// action on integers (anchor of the on_elements recursion)
template <typename Perm>
struct action<int&, on_elements, Perm, is_scalar, is_container> {
  typedef int& argument_type;
  typedef typename deref<argument_type>::type result_type;
  
  const Perm& perm;
  
  action(const Perm& p)
     : perm(p) {}
  
  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return perm[x];
  }      
}; 

// generic action on elements in a container with Array<int>
template <typename OpRef, typename Perm, typename op_tag>
struct action<OpRef, on_elements, Perm, op_tag, is_container,
              typename std::enable_if<std::is_same<typename object_traits<typename deref<OpRef>::type>::model, is_container>::value
                                      && !std::is_same<op_tag, is_matrix>::value, std::true_type>::type
              > {
  typedef OpRef argument_type;
  typedef typename object_traits<typename deref<argument_type>::type>::persistent_type result_type;


  const Perm& perm;

  action(const Perm& p)
     : perm(p) {}

  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return result_type(attach_operation(x,action<typename deref<argument_type>::type::value_type&, on_elements, Perm>(perm)));
  }      
}; 

// generic action on elements in a matrix with Array<int>
template <typename OpRef, typename Perm>
  struct action<OpRef, on_elements, Perm, is_matrix, is_container> {
  typedef OpRef argument_type;
  typedef typename object_traits<typename deref<argument_type>::type>::persistent_type result_type;


  const Perm& perm;

  action(const Perm& p)
   : perm(p) {}

  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return result_type(x.rows(),x.cols(),entire(attach_operation(concat_rows(x),action<typename deref<argument_type>::type::value_type&, on_elements, Perm>(perm))));
  }      
}; 

// action on rows of a matrix with Array<int>
template <typename OpRef, typename Perm>
struct action<OpRef, on_rows, Perm, is_matrix, is_container> {
  typedef OpRef argument_type;
  typedef typename deref<argument_type>::type result_type;

  const Perm& perm;

  action(const Perm& p)
     : perm(p) {}

  result_type operator() (typename function_argument< argument_type >::const_type x) const
  {
    return permuted_rows(x,perm);
  }
};

// action on cols of a matrix with Array<int>
template <typename OpRef, typename Perm>
struct action<OpRef, on_cols, Perm, is_matrix, is_container> {
  typedef OpRef argument_type;
  typedef typename deref<argument_type>::type result_type;
    
  const Perm& perm;

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
     : perm(concatenate(pm::single_value_container<int>(0), translate(p, 1))) {}

  result_type operator() (typename function_argument< argument_type >::const_type x) const
  {
     return permuted_cols(x,perm);
  }      
}; 

// action on a vector under a matrix group
template <typename OpRef, typename Perm>
struct action<OpRef, on_elements, Perm, is_vector, is_matrix> {
  typedef OpRef argument_type;
  typedef typename deref<argument_type>::type result_type;
    
  const Perm& mat;
    
  action(const Perm& m)
     : mat(m.top()) {}
    
  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return mat*x;
  }
};

// action on the variables of a polyomial with Array<int>
template <typename Perm, typename Coefficient, typename Exponent>
struct action<Polynomial<Coefficient,Exponent>&, on_container, Perm, is_polynomial, is_container> {
  typedef Polynomial<Coefficient, Exponent>& argument_type;
  typedef typename deref<argument_type>::type result_type;

  const Perm& perm;

  action(const Perm& p)
     : perm(p) {}

  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return Polynomial<Coefficient, Exponent>(x.coefficients_as_vector(), action<Matrix<Exponent>, on_cols, Perm>(perm)(x.monomials_as_matrix()));
  }      
}; 

// generic action on both elements of a pair
template <typename E1, typename E2, typename action_type, typename Perm, typename perm_tag>
struct action<std::pair<E1,E2>&, action_type, Perm, is_composite, perm_tag> {
  typedef std::pair<E1,E2>& argument_type;
  typedef typename deref<argument_type>::type result_type;

  const Perm& perm;

  action(const Perm& p)
   : perm(p) {}

  result_type operator() (typename function_argument<argument_type>::const_type x) const
  {
    return std::make_pair<E1,E2>(action<E1&, action_type, Perm>(perm)(x.first),action<E2&, action_type, Perm>(perm)(x.second));
  }      
}; 

template<typename TMap, typename Perm>
struct action<TMap&, on_container, Perm, is_map, is_container> {
   typedef TMap& argument_type;
   typedef typename deref<argument_type>::type result_type;

   const Perm& perm;

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

namespace polymake {
namespace group {

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


/*
 * Comutes the orbit of element, where the group is spanned by generators
 */
template<typename action_type, typename Perm, typename Element, typename Container=hash_set<Element>>
auto
orbit(const Array<Perm>& generators, 
      const Element& element) 
{
   Container orbit;
   orbit.insert(element);
   std::queue<Element> q;
   q.push(element);
   while (!q.empty()) {
      const Element orbitElement = q.front();
      q.pop();
      for (const auto& g : generators) {
         const Element next = action<action_type, Perm, Element>(g, orbitElement);
         if(!orbit.collect(next)) {
            q.push(next);
         }
      }
   }
   return orbit;
}

namespace {

inline
int next_not_in_set(const Set<int>& the_set,
		    int initial_value)
{
   if (!the_set.size() || initial_value >= *(the_set.rbegin())) return initial_value+1;
   while(the_set.contains(++initial_value));
   return initial_value;
}

}

/// Calculates a set of orbit representatives for a permutation action
template<typename GeneratorType>
Array<int>
orbit_representatives(const Array<GeneratorType>& generators) 
{
   const int degree = generators[0].size();
   Set<int> seen_elements;
   std::vector<int> reps;
   int rep(0);
   while (rep<degree) {
      reps.push_back(rep);
      seen_elements += orbit<on_elements, GeneratorType, int, Set<int>>(generators, rep);
      rep = next_not_in_set(seen_elements, rep);
   }
   return Array<int>{reps};
}

}
}

#endif

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
