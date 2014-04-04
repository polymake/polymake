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
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace fan {

template <typename Coord>
perl::Object face_fan(perl::Object p, Vector<Coord> v)
{
   perl::ObjectType t = perl::ObjectType::construct<Coord>("PolyhedralFan");
   perl::Object ff(t); 

   const Matrix<Coord> rays = p.give("VERTICES");
   ff.take("RAYS") << dehomogenize(rays-repeat_row(v,rays.rows()));

   const IncidenceMatrix<> vif = p.give("VERTICES_IN_FACETS");
   ff.take("MAXIMAL_CONES") << vif;

   ff.take("REGULAR") << true;
   bool fd = p.give("FULL_DIM");
   ff.take("COMPLETE") << (fd ? true : false);
   
   const Matrix<Coord> ls = p.give("LINEALITY_SPACE");
   if (ls.rows()) {
      ff.take("LINEALITY_SPACE") << dehomogenize(ls);
   } else {
      Matrix<Coord> empty(0,rays.cols()-1);
      ff.take("LINEALITY_SPACE") << empty;
   }

   int cdim = p.give("CONE_DIM");
   ff.take("FAN_DIM") << cdim-1;

   return ff;
}

template <typename Coord>
perl::Object face_fan(perl::Object p)
{
   bool cent = p.give("CENTERED");
   if ( !cent )
      throw std::runtime_error("face_fan: polytope is not centered. Please provide a relative interior point as a second argument");
   int cone_ambient_dim = p.give("CONE_AMBIENT_DIM");
   Vector<Coord> v = unit_vector<Coord>(cone_ambient_dim,0);
   return face_fan(p,v);
}

UserFunctionTemplate4perl("# @category Constructing a fan"
                          "# Computes the face fan of //p//."
                          "# @param Polytope p"
                          "# @param Vector v a relative interior point of the polytope"
                          "# @author Andreas Paffenholz",
                          "face_fan<Coord>(polytope::Polytope<Coord>, Vector<Coord>)");


UserFunctionTemplate4perl("# @category Constructing a fan"
                          "# Computes the face fan of //p//."
                          "# the polytope has to be //CENTERED//"
                          "# @param Polytope p"
                          "# @author Andreas Paffenholz",
                          "face_fan<Coord>(polytope::Polytope<Coord>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
