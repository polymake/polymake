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
#include "polymake/polytope/lrs_interface.h"

namespace polymake { namespace polytope {

// find irredundant representation of rays
// only works for pointed objects
// as lrs does not remove generators in the lineality space from input
// find_vertices_among_points in cdd only works for POINTED objects
void lrs_eliminate_redundant_points(perl::Object p)
{
   lrs_interface::solver solver;
   Matrix<Rational> P=p.give("INPUT_RAYS"),
                    L=p.give("LINEALITY_SPACE");
   const bool isCone = !p.isa("Polytope");
   if (isCone) {
     if ( P.rows() )  // leave matrix empty otherwise
       P = zero_vector<Rational>()|P;
     if ( L.rows() )  // leave matrix empty otherwise
       L = zero_vector<Rational>()|L;
   }

   const lrs_interface::solver::non_redundant V=solver.find_irredundant_representation(P,L,false);
   if (isCone) {
     p.take("RAYS") << P.minor(V.first,~scalar2set(0));
     p.take("LINEAR_SPAN") << V.second.minor(~scalar2set(0),~scalar2set(0));
   } else {
     p.take("RAYS") << P.minor(V.first,All);
     p.take("LINEAR_SPAN") << V.second;
   }
   p.take("POINTED") << ( L.rows()>0 ? 0 : 1 );
}

void lrs_eliminate_redundant_ineqs(perl::Object p)
{
   lrs_interface::solver solver;
   Matrix<Rational> P=p.give("INEQUALITIES"),
                    L=p.give("LINEAR_SPAN");
   const bool isCone = !p.isa("Polytope");
   if (isCone) {
     if ( P.rows() )  // leave matrix empty otherwise
       P = zero_vector<Rational>()|P;
     if ( L.rows() )  // leave matrix empty otherwise
       L = zero_vector<Rational>()|L;
   }

   const lrs_interface::solver::non_redundant V=solver.find_irredundant_representation(P,L,true);
   if (isCone) {
     p.take("FACETS") << P.minor(V.first,~scalar2set(0));
     p.take("LINEALITY_SPACE") << V.second.minor(All,~scalar2set(0));
   } else {
     p.take("FACETS") << P.minor(V.first,All);
     p.take("LINEALITY_SPACE") << V.second;
   }
}

Function4perl(&lrs_eliminate_redundant_points, "lrs_eliminate_redundant_points(Cone<Rational>)");
Function4perl(&lrs_eliminate_redundant_ineqs, "lrs_eliminate_redundant_ineqs(Cone<Rational>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
