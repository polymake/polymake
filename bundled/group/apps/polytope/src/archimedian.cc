/* Copyright (c) 1997-2015
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
#include "polymake/QuadraticExtension.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/group/group_domain.h"

/* This file gathers clients which produce Archimedian solids which cannot
   be produced from Wythoff's construction.  See apps/polytope/src/wythoff.cc.
*/

namespace polymake { namespace polytope {

typedef QuadraticExtension<Rational> QE;


perl::Object truncated_cuboctahedron()
{
   const QE oneplussqrt2(1,1,2);
   const QE oneplus2sqrt2(1,2,2);

   // eight out of the 48 vertices
   Matrix<QE> V(8,4);
   V.col(0) = ones_vector<QE>(V.rows());
   V(0,1) =  1; V(0,2) =  oneplussqrt2; V(0,3) =  oneplus2sqrt2;
   V(1,1) = -1; V(1,2) =  oneplussqrt2; V(1,3) =  oneplus2sqrt2;
   V(2,1) =  1; V(2,2) = -oneplussqrt2; V(2,3) =  oneplus2sqrt2;
   V(3,1) = -1; V(3,2) = -oneplussqrt2; V(3,3) =  oneplus2sqrt2;
   V(4,1) =  1; V(4,2) =  oneplussqrt2; V(4,3) = -oneplus2sqrt2;
   V(5,1) = -1; V(5,2) =  oneplussqrt2; V(5,3) = -oneplus2sqrt2;
   V(6,1) =  1; V(6,2) = -oneplussqrt2; V(6,3) = -oneplus2sqrt2;
   V(7,1) = -1; V(7,2) = -oneplussqrt2; V(7,3) = -oneplus2sqrt2;

   // let the symmetric group of degree 3 act on the columns
   perl::Object g("group::GroupOfPolytope");
   g.set_description() << "symmetric group of degree 3" << endl;
   g.set_name("fullGroupOnCoords");
   g.take("DOMAIN") << polymake::group::OnCoords;
   Array< Array< int > > gens(2);
   Array<int> gen(3);
   gen[0]=1; gen[1]=0; gen[2]=2; gens[0]=gen;
   gen[0]=0; gen[1]=2; gen[2]=1; gens[1]=gen;
   g.take("GENERATORS") << gens;
   
   // put everything together
   perl::Object p("SymmetricPolytope<QuadraticExtension<Rational>>");
   p.set_description() << "truncated cuboctahedron" << endl;
   p.take("GEN_POINTS") << V;
   p.take("GENERATING_GROUP") << g;
   p.take("BOUNDED") << true;
   p.take("POINTED") << true;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create truncated cuboctahedron.  An Archimedean solid."
                  "# This is actually a misnomer.  The actual truncation of a cuboctahedron"
                  "# is obtained as wythoff(""B3"",range(0,2)), which is rational and"
                  "# normally equivalent to this construction."
                  "# @return SymmetricPolytope",
                  &truncated_cuboctahedron, "truncated_cuboctahedron()");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
