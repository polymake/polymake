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
#include "polymake/Matrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/polytope/CubeFacets.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/polytope/cube_group.h"

namespace polymake { namespace polytope {
      
template <typename Scalar>
perl::Object cube(int d, Scalar x_up, Scalar x_low, perl::OptionSet options)
{
   if (d < 1)
      throw std::runtime_error("cube: dimension d >= 1 required");
   
   if (d > std::numeric_limits<int>::digits-1)
      throw std::runtime_error("cube: in this dimension the number of vertices exceeds the machine int size ");
   
   if (x_up==0 && x_low==0) {
      x_up=1;
   } else {
      if (x_up <= x_low)
         throw std::runtime_error("cube: x_up > x_low required");
      negate(x_low);
   }

   perl::Object p("Polytope", mlist<Scalar>());
   p.set_description() << "cube of dimension " << d << endl;

   SparseMatrix<Scalar> F(2*d,d+1);
   auto f=rows(F).begin();
   for (int i=1; i<=d; ++i) { // Facet 2*i and Facet 2*i+1 are parallel  
      (*f)[0]=x_low;
      (*f)[i]=1;
      ++f;
      (*f)[0]=x_up;
      (*f)[i]=-1;
      ++f;
   }

   IncidenceMatrix<> VIF(2*d, 1<<d, CubeFacets<int>(d).begin());
   
   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("CONE_DIM") << d+1;
   p.take("FACETS") << F;
   p.take("AFFINE_HULL") << Matrix<Scalar>(0, d+1);
   p.take("VERTICES_IN_FACETS") << VIF;
   p.take("BOUNDED") << true;

   const bool
      group_flag = options["group"],
      character_table_flag = options["character_table"];

   if (group_flag)
      add_group(p, d, "VERTICES_ACTION", "FACETS_ACTION", character_table_flag);

   return p;
}

UserFunctionTemplate4perl("# @category Producing regular polytopes and their generalizations"
                          "# Produce a //d//-dimensional cube."
                          "# Regular polytope corresponding to the Coxeter group of type B<sub>//d//-1</sub> = C<sub>//d//-1</sub>."
                          "# "
                          "# The bounding hyperplanes are x<sub>i</sub> <= //x_up// and x<sub>i</sub> >= //x_low//."
                          "# @tparam Scalar Coordinate type of the resulting polytope.  Unless specified explicitly, deduced from the type of bound values, defaults to Rational."
                          "# @param Int d the dimension"
                          "# @param Scalar x_up upper bound in each dimension"
                          "# @param Scalar x_low lower bound in each dimension"
                          "# @option Bool group add a symmetry group description to the resulting polytope"
                          "# @option Bool character_table add the character table to the symmetry group description, if 0<d<7; default 1"
                          "# @return Polytope<Scalar>"
                          "# @example This yields a +/-1 cube of dimension 3 and stores it in the variable $c."
                          "# > $c = cube(3);"
                          "# @example This stores a standard unit cube of dimension 3 in the variable $c."
                          "# > $c = cube(3,0);"
                          "# @example This prints the area of a square with side length 4 translated to have"
                          "# its vertex barycenter at [5,5]:"
                          "# > print cube(2,7,3)->VOLUME;"
                          "# | 16",
                          "cube<Scalar> [ is_ordered_field(type_upgrade<Scalar, Rational>) ]"
                          "    (Int; type_upgrade<Scalar>=1, type_upgrade<Scalar>=(-$_[-1]), { group => undef, character_table => 1 } )");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
