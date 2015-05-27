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
#include <queue>

namespace polymake {
namespace group {

template<typename PERM, typename T>
T action_on_container(const PERM& perm, const T& t);

// baseaction on int
inline int action_on_container(const Array<int>& p, int i) {
  return p[i];
}

// action on bitset
template<>
inline Bitset action_on_container(const Array<int>& p, const Bitset& set) {
  Bitset image;
  for(Entire< Bitset >::const_iterator it = entire(set); !it.at_end(); ++it) {
    image += action_on_container(p, *it);
  }
  return image;
}

// baseaction of matrix on vector
template<typename T>
inline Vector<T> action_on_container(const Matrix<T>& p, const Vector<T>& vector) {
  return p*vector;
}

// action on vector
template<typename PERM, typename T>
inline Vector<T> action_on_container(const PERM& p, const Vector<T>& vector) {
  int n = vector.size();
  Vector<T> image = zero_vector<T>(n);
  for(int i = 0; i < n; i++) {
    image[action_on_container(p,i)] = vector[i];
  }
  return image;
}

// action on matrix
template<typename PERM, typename T>
inline Matrix<T> action_on_container(const PERM& p, const Matrix<T>& matrix) {
  ListMatrix<Vector<T> > image;
  for(typename Entire<Rows<Matrix<T> > >::const_iterator row = entire(rows(matrix)); !row.at_end(); ++row) {
    image /= action_on_container(p, Vector<T>(*row));
  }
  return Matrix<T>(image);
}

// action on polynomial
template<typename PERM, typename Coefficient, typename Exponent>
inline Polynomial<Coefficient, Exponent> action_on_container(const PERM& p, const Polynomial<Coefficient, Exponent>& polynomial) {
  Matrix<Exponent> monoms = polynomial.template monomials_as_matrix<Matrix<Exponent> >();
  Vector<Coefficient> coefficients = polynomial.coefficients_as_vector();
  return Polynomial<Coefficient, Exponent>(action_on_container(p, monoms), coefficients, polynomial.get_ring());
}

// action on monomial
template<typename PERM, typename Coefficient, typename Exponent>
inline Monomial<Coefficient, Exponent> action_on_container(const PERM& p, const Monomial<Coefficient, Exponent>& monomial) {
  Vector<Exponent> monom(monomial.get_value());
  return Monomial<Coefficient, Exponent>(action_on_container(p, monom), monomial.get_ring());
}

// action on pair
template<typename PERM, typename T, typename U>
inline std::pair<T,U> action_on_container(const PERM& p, const std::pair<T,U>& pair) {
  return std::make_pair<T,U>(action_on_container(p,pair.first), action_on_container(p,pair.second));
}

// action on bitset
template<typename PERM, typename T>
inline Set<T> action_on_container(const PERM& p, const Set<T>& set) {
  Set<T> image;
  for(typename Entire< Set< T > >::const_iterator it = entire(set); !it.at_end(); ++it) {
    image += action_on_container(p, *it);
  }
  return image;
}

/*
 * Comutes the orbit of element, where the group is spanned by generators
 */
template<typename PERM, typename Element>
Set< Element > orbit(const Array< PERM >& generators, const Element& element) {
  Set< Element > orbit;
  orbit += element;
  std::queue< Element > q;
  q.push(element);
  while(!q.empty()) {
    Element orbitElement = q.front();
    q.pop();
    for(typename Entire<Array< PERM > >::const_iterator generator = entire(generators); !generator.at_end(); ++generator) {
      Element next = action_on_container(*generator, orbitElement);
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
