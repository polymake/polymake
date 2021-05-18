/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	---
	Copyright (c) 2016-2021
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	FIXME Most (or all) of these should at some time be implemented in 
	Polynomial.
	*/

#pragma once

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/Polynomial.h"
#include "polymake/TropicalNumber.h"


namespace polymake { namespace tropical {

template <typename Addition>
Rational evaluate_polynomial(const Polynomial<TropicalNumber<Addition>> &p, const Vector<Rational> &v)
{
  Matrix<Rational> monoms(p.monomials_as_matrix());
  Vector<TropicalNumber<Addition>> coefs(p.coefficients_as_vector());

  TropicalNumber<Addition> result = TropicalNumber<Addition>::zero();
  for (Int m = 0; m < monoms.rows(); ++m) {
    result += (coefs[m] * TropicalNumber<Addition>(monoms.row(m)*v));
  }

  return Rational(result);
}

template <typename Coefficient>
Vector<Int> degree_vector(const Polynomial<Coefficient> &p)
{
  return accumulate(cols(p.monomials_as_matrix()), operations::add());
}

template <typename Coefficient>
Int polynomial_degree(const Polynomial<Coefficient>& p)
{
  if (p.monomials_as_matrix().rows() == 0) return -1;
  return accumulate(degree_vector(p), operations::max());
}

template <typename Coefficient>
bool is_homogeneous(const Polynomial<Coefficient>& p)
{
  if (p.monomials_as_matrix().rows() == 0) return true;
  const Vector<Int> dv = degree_vector(p);
  return dv == same_element_vector(dv[0], dv.dim());
}

} }

