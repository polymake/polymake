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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace polytope {

perl::Object max_GC_rank(int d)
{
   if (d < 2)
      throw std::runtime_error("max_GC_rank: dimension d >= 2 required");

   perl::Object p("Polytope<Rational>");
   p.set_description() << "polytope with maximal GC rank of dim " << d << endl;

   const int n_ineqs=(1<<d)+2*d;
   Matrix<int> Inequalities(n_ineqs,d+1);
   Rows< Matrix<int> >::iterator i=rows(Inequalities).begin();
   // 2d facets of 0/1-cube
   for (int j=1; j<=d; ++j, ++i)
     (*i)[j]=1;
   for (int j=1; j<=d; ++j, ++i) {
     (*i)[0]=1;
     (*i)[j]=-1;
   }
   // one more inequality per subset of [1..d]
   for (auto si = entire(all_subsets(range(1,d))); !si.at_end(); ++si, ++i) {
     (*i)[0] = d - (si->size()) - 1;
     i->slice(range(1,d)).fill(-1);
     i->slice(*si).fill(1);
   }

   p.take("CONE_AMBIENT_DIM") << d+1;
   p.take("INEQUALITIES") << Inequalities;
   p.take("VALID_POINT") << (1 | Vector<Rational>(d,Rational(1,2)));
   p.take("BOUNDED") << true;
   p.take("FEASIBLE") << true;

   // symmetric linear objective function
   perl::Object LP("LinearProgram<Rational>");
   LP.take("LINEAR_OBJECTIVE") << Vector<Rational>(0|ones_vector<Rational>(d));
   LP.attach("INTEGER_VARIABLES") << Array<bool>(d,true);
   p.take("LP") << LP;

   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce a //d//-dimensional polytope of maximal Gomory-Chvatal rank $ Omega( d/log(d) ) $ ,"
                  "# integrally infeasible."
                  "# With symmetric linear objective function (0,1,1..,1)."
                  "# Construction due to Pokutta and Schulz."
                  "# "
                  "# @param Int d the dimension"
                  "# @return Polytope",
                  &max_GC_rank, "max_GC_rank");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
