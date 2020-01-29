/* Copyright (c) 1997-2020
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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

#ifndef POLYMAKE_TROPICAL_DUAL_ADDITION_VERSION_H
#define POLYMAKE_TROPICAL_DUAL_ADDITION_VERSION_H

#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Vector.h"
#include "polymake/Polynomial.h"

namespace polymake { namespace tropical {

template <typename Addition, typename Scalar>
TropicalNumber<typename Addition::dual, Scalar>
dual_addition_version(const TropicalNumber<Addition, Scalar>& t, bool strong = true)
{
  return TropicalNumber<typename Addition::dual, Scalar>(strong ? -Scalar(t) : Scalar(t));
}

template <typename Addition, typename Scalar>
Vector<TropicalNumber<typename Addition::dual, Scalar>>
dual_addition_version(const Vector<TropicalNumber<Addition, Scalar>> &v, bool strong = true)
{
  Vector<TropicalNumber<typename Addition::dual, Scalar>> r(v.dim());
  for (Int i = 0; i < v.dim(); ++i) {
    r[i] = dual_addition_version(v[i], strong);
  }
  return r;
}

template <typename Addition, typename Scalar>
Matrix<TropicalNumber<typename Addition::dual, Scalar>>
dual_addition_version(const Matrix<TropicalNumber<Addition, Scalar>>& m, bool strong = true)
{
  Matrix<TropicalNumber<typename Addition::dual, Scalar>> r(m.rows(), m.cols());
  auto r_it = concat_rows(r).begin();
  for (auto m_it = entire(concat_rows(m)); !m_it.at_end(); ++m_it, ++r_it)
    *r_it = dual_addition_version(*m_it,strong);

  return r;
}

template <typename Addition, typename Scalar>
Polynomial<TropicalNumber<typename Addition::dual, Scalar>>
dual_addition_version(const Polynomial<TropicalNumber<Addition, Scalar>>& p, bool strong = true)
{
  return Polynomial<TropicalNumber<typename Addition::dual, Scalar>>(
                                        dual_addition_version(p.coefficients_as_vector(), strong),
					p.monomials_as_matrix());
}

} }

#endif

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
