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
#include "polymake/tropical/arithmetic.h"

namespace polymake { namespace tropical {

PuiseuxPolynomial pointlift(const Vector<Rational>& p, const PuiseuxRing& r)
// REMARK: this requires the length of p to match the number of indeterminates
{
  const int n=p.size();
  Array< UniPolynomial<Rational,Rational> > coefs(n);
  for (int i=0; i<n; ++i) {
     coefs[i]=UniMonomial<Rational,Rational>(p[i]);
  }
  return PuiseuxPolynomial(unit_matrix<int>(n),coefs,r);
}

template <typename Addition>
TropicalPolynomial tropicalize(const PuiseuxPolynomial& p)
{
   const int n_terms=p.n_terms();
   Vector<Rational> exps(n_terms);
   Vector<Rational>::iterator e=exps.begin();
   for (Entire<PuiseuxPolynomial::term_hash>::const_iterator i=entire(p.get_terms()); !i.at_end(); ++i, ++e)
      *e=Addition::orientation()*i->second.lm_exp();

   return TropicalPolynomial(p.monomials_as_matrix< Matrix<int> >(), exps);
}

template <typename Addition>
perl::Object points2hypersurface(const Matrix<Rational>& points)
{
  const int n=points.cols();
  PuiseuxRing r(n);
  PuiseuxPolynomial poly(1, r);

  for (Entire< Rows< Matrix<Rational> > >::const_iterator i=entire(rows(points)); !i.at_end(); ++i)
     poly*=pointlift(-(*i),r);

  perl::Object h(perl::ObjectType::construct<typename Addition::dual>("Hypersurface"));
  const TropicalPolynomial tpoly=tropicalize<typename Addition::dual>(poly);
  h.take("MONOMIALS") << tpoly.first;
  h.take("COEFFICIENTS") << tpoly.second;

  return h;
}
    
UserFunctionTemplate4perl("# @category Other"
                          "# Constructs a tropical hypersurface defined by the linear"
                          "# hypersurfaces associated to the points."
                          "# If the points are part of a min-tropical polytope then the output is a"
                          "# max-tropical hypersurface, and conversely."
                          "# @param Matrix<Rational> points"
                          "# @return Hypersurface",
                          "points2hypersurface<Addition=Min>(Matrix)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
