/* Copyright (c) 1997-2023
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
#include "polymake/Rational.h"
#include "polymake/SparseMatrix.h"

namespace polymake { namespace polytope {

template<typename Scalar>
BigObject create_delpezzo(const Int d, const Scalar& s, const bool pseudo_flag)
{
   if (d < 1)
      throw std::runtime_error("del_pezzo : dimension d >= 1 required");

   if (size_t(d) >= sizeof(Int)*8-1)
      throw std::runtime_error("del_pezzo: in this dimension the number of facets exceeds the machine Int size");

   if (s <= zero_value<Scalar>())
      throw std::runtime_error("del_pezzo : scale > 0 required");

   const Int n_vertices = pseudo_flag ? 2*d+1 : 2*d+2;

   SparseMatrix<Scalar> V((s*unit_matrix<Scalar>(d)) / ((-s)*unit_matrix<Scalar>(d)) / (s*ones_vector<Scalar>(d)));
   if (!pseudo_flag) V /= -s*ones_vector<Scalar>(d);
   V = ones_vector<Scalar>() | V;

   BigObject p("Polytope", mlist<Scalar>(),
               "CONE_AMBIENT_DIM", d+1,
               "CONE_DIM", d+1,
               "N_VERTICES", n_vertices,
               "VERTICES", V,
               "BOUNDED", true,
               "CENTERED", true,
               "FEASIBLE", true);
   p.set_description() << "del-pezzo-polytope of dimension " << d << endl;
   return p;
}

template<typename Scalar>
BigObject delpezzo(const Int d, const Scalar& s)
{
   return create_delpezzo(d, s, false);
}

template<typename Scalar>
BigObject pseudo_delpezzo(const Int d, const Scalar& s)
{
   return create_delpezzo(d, s, true);
}

UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                          "# Produce a //d//-dimensional del-Pezzo polytope, which is the convex hull of"
                          "# the cross polytope together with the all-ones and minus all-ones vector."
                          "# "
                          "# All coordinates are +/- //scale// or 0."
                          "# @param Int d the dimension"
                          "# @param Scalar scale the absolute value of each non-zero vertex coordinate. Needs to be positive. The default value is 1."
                          "# @return Polytope<Scalar>",
                          "delpezzo<Scalar> [ is_ordered_field(type_upgrade<Scalar, Rational>) ] (Int; type_upgrade<Scalar>=1 )");


UserFunctionTemplate4perl("# @category Producing a polytope from scratch"
                          "# Produce a //d//-dimensional del-Pezzo polytope, which is the convex hull of"
                          "# the cross polytope together with the all-ones vector."
                          "# "
                          "# All coordinates are +/- //scale// or 0."
                          "# @param Int d the dimension"
                          "# @param Scalar scale the absolute value of each non-zero vertex coordinate. Needs to be positive. The default value is 1."
                          "# @return Polytope<Scalar>",
                          "pseudo_delpezzo<Scalar> [ is_ordered_field(type_upgrade<Scalar, Rational>) ] (Int; type_upgrade<Scalar>=1 )");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
