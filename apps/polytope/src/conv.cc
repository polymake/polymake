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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/ListMatrix.h"
#include "polymake/Array.h"

namespace polymake { namespace polytope {

template <typename Scalar>
perl::Object conv(const Array<perl::Object>& pp_in)
{
   auto p_in=entire(pp_in);

   if (p_in.at_end()) throw std::runtime_error("empty input");

   ListMatrix< Vector<Scalar> > Points=p_in->give("VERTICES | POINTS");
   ListMatrix< Vector<Scalar> > LinSpace=p_in->give("LINEALITY_SPACE");

   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   std::string descr_names=p_in->name();

   while (! (++p_in).at_end()) {
      const Matrix<Scalar> V=p_in->give("VERTICES | POINTS");
      const Matrix<Scalar> L=p_in->give("LINEALITY_SPACE");
      if (V.cols() == Points.cols())
         Points /= V;
      else
         throw std::runtime_error("conv - Points dimension mismatch");
      if (L.cols() == LinSpace.cols())
         LinSpace /= L;
      else
         throw std::runtime_error("conv - LinSpace dimension mismatch");
      descr_names+=", ";
      descr_names+=p_in->name();
   }

   p_out.set_description() << "Convex hull of polytopes " << descr_names << endl;
   p_out.take("INPUT_LINEALITY")<<LinSpace;
   p_out.take("POINTS") << Points;
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Construct a new polyhedron as the convex hull of the polyhedra"
                          "# given in //P_Array//."
                          "# @param Array<Polytope> P_Array"
                          "# @return PropagatedPolytope"
                          "# @example"
                          "# > $p = conv([cube(2,1,0),cube(2,6,5)]);"
                          "# > print $p->VERTICES;"
                          "# | 1 0 0"
                          "# | 1 1 0"
                          "# | 1 0 1"
                          "# | 1 6 5"
                          "# | 1 5 6"
                          "# | 1 6 6",
                          "conv<Scalar>(Polytope<type_upgrade<Scalar>> +)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
