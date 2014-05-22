/* Copyright (c) 1997-2014
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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/tropical/arithmetic.h"

namespace polymake { namespace tropical {

template <typename Addition>
Rational evaluate(const Matrix<int>& monoms, const Vector<Rational>& coefs, const Vector<Rational>& x) {
  const int n(monoms.rows());
  Rational val(coefs[0]+(x*monoms[0]));
  for (int i=1; i<n; ++i) {
    Rational v(coefs[i]+(x*monoms[i]));
    if (Addition::orientation()*(val-v)>0)
      val=v;
  }
  return val;
}

template <typename Addition>
Vector<Rational> evaluate(const Matrix<int>& monoms, const Vector<Rational>& coefs, const Matrix<Rational>& X) {
  const int m(X.rows());
  Vector<Rational> vals(m);
  for (int i=0; i<m; ++i) vals[i]=evaluate<Addition>(monoms,coefs,X[i]);
  return vals;
}

template <typename Addition>
Rational evaluate(perl::Object H, const Vector<Rational>& x) {
  const Matrix<int> monoms=H.give("MONOMIALS");
  const Vector<Rational> coefs=H.give("COEFFICIENTS");
  return evaluate<Addition>(monoms,coefs,x);
}

template <typename Addition>
Vector<Rational> evaluate(perl::Object H, const Matrix<Rational>& X) {
  const Matrix<int> monoms=H.give("MONOMIALS");
  const Vector<Rational> coefs=H.give("COEFFICIENTS");
  return evaluate<Addition>(monoms,coefs,X);
}

FunctionTemplate4perl("evaluate<Addition> (Matrix<Int> Vector<Rational> Vector<Rational>)");
FunctionTemplate4perl("evaluate<Addition> (Matrix<Int> Vector<Rational> Matrix<Rational>)");

UserFunctionTemplate4perl("# @category Basic functions"
			  "# Evaluate a tropical polynomial at a given point."
                          "# @param Hypersurface H"
                          "# @param Vector<Rational> x"
                          "# @tparam Addition [[Min]] or [[Max]]"
                          "# @return Rational",
                          "evaluate<Addition>(Hypersurface<Addition> Vector<Rational>)");

UserFunctionTemplate4perl("# @category Basic functions"
			  "# Evaluate a tropical polynomial at a collection of points."
                          "# @param Hypersurface H"
                          "# @param Matrix<Rational> X"
                          "# @tparam Addition [[Min]] or [[Max]]"
                          "# @return Vector<Rational>",
                          "evaluate<Addition>(Hypersurface<Addition> Matrix<Rational>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
