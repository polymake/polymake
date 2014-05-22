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
#include "polymake/ListMatrix.h"
#include "polymake/Map.h"
#include "polymake/tropical/arithmetic.h"

namespace polymake { namespace tropical {

template <typename Addition>
perl::Object hypersurface_union(perl::Object H1, perl::Object H2, bool internal)
{
  const Matrix<int> monoms1=H1.give("MONOMIALS");
  const Vector<Rational> coefs1=H1.give("COEFFICIENTS");
  const int n1=monoms1.rows(), d1=monoms1.cols();
  if (n1!=coefs1.size())
    throw std::runtime_error("Coefficient vector of first hyperplane has wrong dimension.");

  const Matrix<int> monoms2=H2.give("MONOMIALS");
  const Vector<Rational> coefs2=H2.give("COEFFICIENTS");
  const int n2=monoms2.rows(), d2=monoms2.cols();
  if (n2!=coefs2.size())
    throw std::runtime_error("Coefficient vector of second hyperplane has wrong dimension.");

  if (internal && d1!=d2)
     throw std::runtime_error("Hyperplanes in different dimensions.  Use 'internal=>0'?");
  
  perl::Object H12(perl::ObjectType::construct<Addition>("Hypersurface"));
  ListMatrix< Vector<int> > H12_monoms;
  Vector<Rational> H12_coefs;

  if (internal) {
     // multiply polynomials tropically
     typedef Map< Vector<int>, Rational > map_type;
     map_type p_map;
     for (int i=0; i<n1; ++i)
        for (int k=0; k<n2; ++k) {
           Vector<int> m(monoms1[i] + monoms2[k]);
           const Rational c(coefs1[i] + coefs2[k]);
           typename map_type::iterator p_it=p_map.find(m);
           if (p_it.at_end())
              p_map[m]=c;
           else
              p_it->second=Addition::apply(p_it->second,c);
        }
     const int n=p_map.size();
     H12_coefs.resize(n);
     int u=0;
     for (typename Entire<map_type>::iterator p_it=entire(p_map); !p_it.at_end(); ++p_it, ++u) {
        H12_monoms /= p_it->first;
        H12_coefs[u] = p_it->second;
     }
  } else {
     // concatenate
     const int n=n1*n2;
     H12_coefs.resize(n);
     int u=0;
     for (int i=0; i<n1; ++i)
        for (int k=0; k<n2; ++k, ++u) {
           H12_monoms /= (monoms1[i] | monoms2[k]);
           H12_coefs[u] = coefs1[i] + coefs2[k];
        }
  }

  H12.take("MONOMIALS") << H12_monoms;
  H12.take("COEFFICIENTS") << H12_coefs;
  
  return H12;
}

UserFunctionTemplate4perl("# @category Producing a tropical hypersurface"
                          "# @param Hypersurface H1"
                          "# @param Hypersurface H2"
                          "# @param Bool internal default 1: both input hyperplanes lie in the same space"
                          "# @return Hypersurface H1 cup H2",
                          "hypersurface_union<Addition>(Hypersurface<Addition> Hypersurface<Addition>; $=1)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
