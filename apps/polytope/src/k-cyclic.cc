/* Copyright (c) 1997-2021
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

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/AccurateFloat.h"

namespace polymake { namespace polytope {

BigObject k_cyclic(Int n, Vector<Rational> s)
{
   const Int k = s.size();
   if (k < 1 || k >= n)
      throw std::runtime_error("k_cyclic: 1 <= k < n required");

   Matrix<Rational> vertices(n,2*k+1);
   auto v = concat_rows(vertices).begin();
   s *= 2; s /= n;

   AccurateFloat sinK, cosK;
   for (Int i = 0; i < n; ++i) {
      *v++ = 1;
      for (Int j = 0; j < k; ++j) {
         const Rational S=s[j]*i;
         // prevent silly values for multiples of pi/2
         if (denominator(S)==1) {
            *v++ = numerator(S).odd() ? -1 : 1;
            *v++ = 0;
         } else if (denominator(S)==2) {
            *v++ = 0;
            *v++ = numerator(S).bit(1) ? -1 : 1;
         } else {
            sin_cos(sinK, cosK, S*AccurateFloat::pi());
            *v++ = cosK;
            *v++ = sinK;
         }
      }
   }
   BigObject p("Polytope<Rational>",
               "CONE_AMBIENT_DIM", 2*k+1,
               "CONE_DIM", 2*k+1,
               "N_VERTICES", n,
               "VERTICES", vertices,
               "BOUNDED", true);
   p.set_description() << k << "-cyclic polytope" << endl;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a (rounded) 2*k-dimensional k-cyclic polytope with //n// points,"
                  "# where k is the length of the input vector //s//."
                  "# Special cases are the bicyclic (k=2) and tricyclic (k=3) polytopes."
                  "# Only possible in even dimensions."
                  "# "
                  "# The parameters s_i can be specified as integer, "
                  "# floating-point, or rational numbers."
                  "# The coordinates of the i-th point are taken as follows:"
                  "#\t cos(s_1 * 2&pi;i/n),"
                  "#\t sin(s_1 * 2&pi;i/n),"
                  "#\t ..."
                  "#\t cos(s_k * 2&pi;i/n),"
                  "#\t sin(s_k * 2&pi;i/n)"
                  "# "
                  "# Warning: Some of the k-cyclic polytopes are not simplicial."
                  "# Since the components are rounded, this function might output a polytope"
                  "# which is not a k-cyclic polytope!"
                  "# "
                  "# More information can be found in the following references:"
                  "#\t P. Schuchert: \"Matroid-Polytope und Einbettungen kombinatorischer Mannigfaltigkeiten\","
                  "#\t PhD thesis, TU Darmstadt, 1995."
                  "# "
                  "#\t Z. Smilansky: \"Bi-cyclic 4-polytopes\","
                  "#\t Isr. J. Math. 70, 1990, 82-92"
                  "# @param Int n the number of points"
                  "# @param Vector s s=(s_1,...,s_k)"
                  "# @return Polytope"
                  "# @example To produce a (not exactly) regular pentagon, type this:"
                  "# > $p = k_cyclic(5,[1]);",
                  &k_cyclic, "k_cyclic($ $)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
