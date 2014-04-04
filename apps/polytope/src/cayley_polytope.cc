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
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Vector.h"

namespace polymake { namespace polytope {

perl::Object cayley_polytope(const Array<perl::Object> & pp, perl::OptionSet options)
{
   int d = pp.size();

   Entire< Array<perl::Object> >::const_iterator pp_it = entire(pp);

   bool pointed=pp_it->give("POINTED");
   if (!pointed)
      throw std::runtime_error("cayley_polytope: input polyhedra not pointed");

   if ( !pp_it->give("LATTICE") )
      throw std::runtime_error("cayley polytope construction only defined for lattice polytopes");
   int adim = pp_it->give("CONE_AMBIENT_DIM");

   perl::Object p_out(pp_it->type());
   std::string names = pp_it->name();
   ListMatrix< Vector<Integer> > U = pp_it->give("VERTICES | POINTS");
   U |= repeat_row(unit_vector<Integer>(d,0),U.rows());

   int i = 1;
   while ( !(++pp_it).at_end() ) {
   bool pointed=pp_it->give("POINTED");
      if (!pointed)
         throw std::runtime_error("cayley_polytope: input polyhedra not pointed");

      if ( !pp_it->give("LATTICE") )
         throw std::runtime_error("cayley polytope construction only defined for lattice polytopes");

      int adim2 = pp_it->give("CONE_AMBIENT_DIM");
      if ( adim != adim2 )
         throw std::runtime_error("cayley polytope construction only defined for polytopes with equal ambient dimension");

      Matrix<Integer> V = pp_it->give("VERTICES | POINTS");
      U /= V|repeat_row(unit_vector<Integer>(d,i++),V.rows());

      names += ", ";
      names += pp_it->name();
   }

   p_out.set_description() << "cayley polytope of the polytopes " << names << endl;
   if ( options["proj"] ) 
      p_out.take("POINTS") << U.minor(All,~range(U.cols()-1,U.cols()-1));
   else
      p_out.take("POINTS") << U;

   const Matrix<Rational> empty;
   p_out.take("INPUT_LINEALITY") << empty;

   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct the cayley polytope of a set of pointed lattice polytopes contained in //P_Array//"
                          "# which is the convex hull of P<sub>1</sub>&times;e<sub>1</sub>, ..., P<sub>k</sub>&times;e<sub>k</sub>"
                          "# where e<sub>1</sub>, ...,e<sub>k</sub> are the standard unit vectors in R<sup>k</sup>."
                          "# In this representation the last k coordinates always add up to 1."
                          "# The option //proj// projects onto the complement of the last coordinate."
                          "# @param Array<LatticePolytope> P_Array  an array containing the lattice polytopes P<sub>1</sub>,...,P<sub>k</sub>"
                          "# @option Bool proj"
                          "# @return Polytope",
                          "cayley_polytope(Polytope<Rational> +; {proj => 0} )");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
