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
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/Rational.h"
#include <cmath>

namespace polymake { namespace polytope {

template <typename Scalar>
perl::Object spherize(perl::Object p_in)
{
   const bool bounded=p_in.give("BOUNDED"),
              centered=p_in.give("CENTERED");
   if (!bounded || !centered)
      throw std::runtime_error("spherize: input polytope must be bounded and centered\n");

   perl::Object p_out(perl::ObjectType::construct<Scalar>("Polytope"));
   p_out.set_description() << "Spherized polytope " << p_in.name() << endl;

   const Matrix<double> V=p_in.give("VERTICES | POINTS");
   const Matrix<double> points= ones_vector<double>(V.rows()) | normalized(V.minor(All,range(1,V.cols()-1)));

   p_out.take("POINTS") << points;
   p_out.take("BOUNDED") << true;
   p_out.take("CENTERED") << true;
   const Matrix<Rational> empty;
   p_out.take("INPUT_LINEALITY") << empty;
   return p_out;
}

UserFunctionTemplate4perl("# @category Producing a polytope from polytopes"
                          "# Project all vertices of a polyhedron //P// on the unit sphere."
                          "# //P// must be [[CENTERED]] and [[BOUNDED]]."
                          "# @param Polytope P"
                          "# @return Polytope",
                          "spherize<Scalar>(Polytope<Scalar>)");
   
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
