/* Copyright (c) 1997-2022
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
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Vector.h"

namespace polymake { namespace polytope {

BigObject cayley_polytope(const Array<BigObject>& pp, OptionSet options)
{
   Int d = pp.size();

   auto pp_it = entire(pp);

   bool pointed=pp_it->give("POINTED");
   if (!pointed)
      throw std::runtime_error("cayley_polytope: input polyhedra not pointed");

   bool is_lattice=pp_it->give("LATTICE");
   if (!is_lattice)
      throw std::runtime_error("cayley polytope construction only defined for lattice polytopes");
   const Int adim = pp_it->give("CONE_AMBIENT_DIM");

   BigObject p_out(pp_it->type());
   std::string names = pp_it->name();
   ListMatrix< Vector<Integer> > U = pp_it->give("VERTICES | POINTS");
   U |= repeat_row(unit_vector<Integer>(d,0), U.rows());

   Int i = 1;
   while ( !(++pp_it).at_end() ) {
      pointed = pp_it->give("POINTED");
      if (!pointed)
         throw std::runtime_error("cayley_polytope: input polyhedra not pointed");
      is_lattice=pp_it->give("LATTICE");
      if (!is_lattice)
         throw std::runtime_error("cayley polytope construction only defined for lattice polytopes");

      Int adim2 = pp_it->give("CONE_AMBIENT_DIM");
      if (adim != adim2)
         throw std::runtime_error("cayley polytope construction only defined for polytopes with equal ambient dimension");

      Matrix<Integer> V = pp_it->give("VERTICES | POINTS");
      U /= V|repeat_row(unit_vector<Integer>(d,i++),V.rows());

      names += ", ";
      names += pp_it->name();
   }

   p_out.set_description() << "cayley polytope of the polytopes " << names << endl;
   if (options["proj"]) {
      p_out.take("POINTS") << U.minor(All, range(0, U.cols()-2));
      p_out.take("INPUT_LINEALITY") << Matrix<Rational>(0, U.cols()-1);
   } else {
      p_out.take("POINTS") << U;
      p_out.take("INPUT_LINEALITY") << Matrix<Rational>(0, U.cols());
   }

   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct the cayley polytope of a set of pointed lattice polytopes contained in //P_Array//"
                          "# which is the convex hull of P<sub>1</sub>&times;e<sub>1</sub>, ..., P<sub>k</sub>&times;e<sub>k</sub>"
                          "# where e<sub>1</sub>, ...,e<sub>k</sub> are the standard unit vectors in R<sup>k</sup>."
                          "# In this representation the last k coordinates always add up to 1."
                          "# The option //proj// projects onto the complement of the last coordinate."
                          "# @param Array<Polytope> P_Array  an array containing the lattice polytopes P<sub>1</sub>,...,P<sub>k</sub>"
                          "# @option Bool proj"
                          "# @return Polytope",
                          "cayley_polytope(Polytope<Rational> +; {proj => 0} )");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
