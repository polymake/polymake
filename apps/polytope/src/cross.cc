/* Copyright (c) 1997-2018
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
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/polytope/CubeFacets.h"
#include "polymake/polytope/cube_group.h"

namespace polymake { namespace polytope {

template<typename Scalar>
perl::Object cross(int d, const Scalar& s, perl::OptionSet options)
{
   if (d < 1)
      throw std::runtime_error("cross : dimension d >= 1 required");

   if (d > std::numeric_limits<int>::digits-1)
      throw std::runtime_error("cross: in this dimension the number of facets exceeds the machine int size ");

   if (s <= zero_value<Scalar>())
      throw std::runtime_error("cross : scale >= 0 required");

   const int n_vertices=2*d;

   perl::Object p("Polytope", mlist<Scalar>());
   p.set_description() << "cross-polytope of dimension " << d << endl;

   SparseMatrix<Scalar> V(n_vertices,d+1);
   V.col(0).fill(1);
   int c=1;
   for (auto v=entire(rows(V)); !v.at_end(); ++v, ++c) {
      (*v)[c]=s;
      ++v;
      (*v)[c]=-s;
   }
   IncidenceMatrix<> VIF(1<<d, n_vertices);
   copy_range(CubeFacets<int>(d).begin(), cols(VIF).begin());

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CONE_DIM") << d+1;
   p.take("N_VERTICES") << n_vertices;
   p.take("VERTICES") << V;
   p.take("VERTICES_IN_FACETS") << VIF;
   p.take("BOUNDED") << true;
   p.take("CENTERED") << true;

   const bool
      group_flag = options["group"],
      character_table_flag = options["character_table"];

   if (group_flag)
      add_group(p, d, "FACETS_ACTION", "VERTICES_ACTION", character_table_flag);

   return p;
}

perl::Object octahedron() 
{
   const perl::OptionSet no_options;
   return cross(3,one_value<Rational>(),no_options);
}

UserFunctionTemplate4perl("# @category Producing regular polytopes and their generalizations"
                          "# Produce a //d//-dimensional cross polytope."
                          "# Regular polytope corresponding to the Coxeter group of type B<sub>//d//-1</sub> = C<sub>//d//-1</sub>."
                          "# "
                          "# All coordinates are +/- //scale// or 0."
                          "# @tparam Scalar Coordinate type of the resulting polytope.  Unless specified explicitly, deduced from the type of bound values, defaults to Rational."
                          "# @param Int d the dimension"
                          "# @param Scalar scale the absolute value of each non-zero vertex coordinate. Needs to be positive. The default value is 1."
                          "# @option Bool group add a symmetry group description to the resulting polytope"
                          "# @option Bool character_table add the character table to the symmetry group description, if 0<d<7; default 1"
                          "# @return Polytope<Scalar>"
                          "# @example To create the 3-dimensional cross polytope, type"
                          "# > $p = cross(3);"
                          "# Check out it's vertices and volume:"
                          "# > print $p->VERTICES;"
                          "# | 1 1 0 0"
                          "# | 1 -1 0 0"
                          "# | 1 0 1 0"
                          "# | 1 0 -1 0"
                          "# | 1 0 0 1"
                          "# | 1 0 0 -1"
                          "# > print cross(3)->VOLUME;"
                          "# | 4/3"
                          "# If you rather had a bigger one, type"
                          "# > $p_scaled = cross(3,2);"
                          "# > print $p_scaled->VOLUME;"
                          "# | 32/3"
                          "# To also calculate the symmetry group, do this:"
                          "# > $p = cross(3,group=>1);"
                          "# You can then print the generators of this group like this:"
                          "# > print $p->GROUP->RAYS_ACTION->GENERATORS;"
                          "# | 1 0 2 3 4 5"
                          "# | 2 3 0 1 4 5"
                          "# | 0 1 4 5 2 3",
                          "cross<Scalar> [ is_ordered_field(type_upgrade<Scalar, Rational>) ] (Int; type_upgrade<Scalar>=1, { group => undef, character_table => 1 } )");

UserFunction4perl("# @category Producing regular polytopes and their generalizations"
                  "# Produce a regular octahedron, which is the same as the 3-dimensional cross polytope."
                  "# @return Polytope",
                  &octahedron, "octahedron");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
