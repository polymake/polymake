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

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Polynomial.h"
#include "polymake/Matrix.h"

namespace polymake { namespace polytope {

template <typename Coefficient, typename Exponent>
perl::Object newton(const Polynomial<Coefficient,Exponent>& p)
{
   perl::Object np("Polytope<Rational>");
   np.set_description() << "Newton polytope of " << p << endl;

   const Matrix<Exponent> exps(p.template monomials_as_matrix< Matrix<Exponent> >());
   const int n=exps.rows();
   const int d=exps.cols();

   np.take("POINTS") << (same_element_vector(1,n) | exps);
   np.take("CONE_AMBIENT_DIM") << d+1;
   np.take("LATTICE") << true;
   np.take("BOUNDED") << true;

   return np;
}

UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                          "# Produce the Newton polytope of a polynomial //p//."
                          "# @param Polynomial p"
                          "# @return Polytope<Rational>"
                          "# @example Create the newton polytope of 1+x^2+y like so:"
                          "# > $r=new Ring(qw(x y));"
                          "# > ($x,$y)=$r->variables;"
                          "# > $p=1+($x^2)+$y;"
                          "# > $n = newton($p);"
                          "# > print $n->VERTICES;"
                          "# | 1 0 0"
                          "# | 1 0 1"
                          "# | 1 2 0",
                          "newton(Polynomial)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
