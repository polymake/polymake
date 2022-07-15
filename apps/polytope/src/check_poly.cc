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
#include "polymake/Array.h"
#include "polymake/PowerSet.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {

BigObject check_poly(const IncidenceMatrix<>& VIF, OptionSet options)
{
   // for all comments below we assume that we read VERTICES_IN_FACETS;
   // otherwise all notions have to be dualized

   const bool primal = !options["dual"];
   const bool verbose = options["verbose"];

   const Int n = primal ? VIF.rows() : VIF.cols(),       // number of facets
             v = primal ? VIF.cols() : VIF.rows();      // number of vertices

   Int d = 0,             // (co-)dimension
       max_d = n-1,       // upper bound for the dimension
       chi = 0,           // Euler characteristic
       chi_sgn = 1,
       simplicial,      // k-simplicity or k-simpleness
       neighborly;      // k-neighborliness or k-balance
  
   PowerSet<Int> F;     // all faces, we start with facets
   if (primal) {
      for (auto vlist=entire(rows(VIF)); !vlist.at_end(); ++vlist) {
         F += *vlist;
         max_d = std::min(vlist->size(), max_d);
      }
   } else {
      for (auto vlist=entire(cols(VIF)); !vlist.at_end(); ++vlist) {
         F += *vlist;
         max_d = std::min(vlist->size(), max_d);
      }
   }

   if (verbose) cout << "check_poly: face lattice analysis; dim <= " << max_d << endl;

   Array<Int> c(max_d+1); // for i>0:
   // if c[i] > 0 then each i-coface contains c[i] vertices
   c[0] = n;       // c[0] = #facets

   Array<Int> width(max_d+1);  // number of i-cofaces
   width[0] = 1;               // one polytope

   while (max_d > 0 && (width[d+1] = F.size()) > 1) {
      if (verbose) cout << "[ " << (primal ? -(d+1) : d) << " : " << F.size() << " ] " << std::flush;
      chi += chi_sgn * F.size(); chi_sgn = -chi_sgn;

      PowerSet<Int>::iterator f = F.begin(), fend = F.end();
      Int card = f->size();
      if (card < max_d) max_d = card;

      for (++f; f != fend; ++f) {
         Int l = f->size();
         if (card && l != card) card = 0;
         if (l < max_d) max_d = l;
      }
      c[d+1] = card;
      max_d--;
      if (verbose) cout << "\n" << F << "\n" << endl;

      F = ridges(F);
      d++;
   }

   if (primal && d%2 == 0)
      chi = -chi;
   if (verbose)
      cout << "chi = " << chi << "\n"
         "constant number of "
           << ( primal ? "vertices per face" : "facets through face" )
           << " of level = <" << select(c, range(0,d)) << ">\n"
         "number of faces per level = <"  << select(width, range(0,d)) << ">\n";
  
   for (simplicial = 0;
        simplicial < d && c[d-1-simplicial] == simplicial+2;
        simplicial++);

   for (neighborly = 0;
        neighborly < d && width[d-neighborly] == Integer::binom(v,neighborly+1);
        neighborly++);

   if (verbose)
      cout << simplicial << ( primal ? "-simplicial, " : "-simple, " )
           << neighborly << ( primal ? "-neighborly"   : "-balanced" );

   // if we have a polytope then
   //   the lattice is coatomic,
   //   the Euler characteristic is that of a (d-1)-sphere,
   //   we have sufficiently many edges or ridges

   if (!(width[d] == v
         && ((d%2 == 0 && chi == 0) || (d%2 != 0 && chi == 2))
         && simplicial > 0)) {
      if (verbose) cout << ", but not a polytope, last (co-)dimension " << d << endl;
      throw std::runtime_error("not a polytope");
   }

   BigObject p("Polytope"); //Combinatorial Polytope
   if (verbose) cout << " " << d << "-polytope" << endl;
   if (primal) {
      p.take("VERTICES_IN_FACETS")<<VIF;
      p.take("SIMPLICIAL") << (simplicial >= d-1);
      p.take("NEIGHBORLY") << (neighborly >= d/2);
      p.take("SIMPLICIALITY") << simplicial;
      p.take("NEIGHBORLINESS") << neighborly;
   } else {
      p.take("VERTICES_IN_FACETS")<<T(VIF);
      p.take("SIMPLE") << (simplicial >= d-1);
      p.take("BALANCED") << (neighborly >= d/2);
      p.take("SIMPLICITY") << simplicial;
      p.take("BALANCE") << neighborly;
   }
   return p;
}

UserFunction4perl("# @category Consistency check"
                  "# Try to check whether a given vertex-facet incidence matrix //VIF// defines a polytope."
                  "# Note that a successful certification by check_poly is **not sufficient**"
                  "# to determine whether an incidence matrix actually defines a polytope."
                  "# Think of it as a plausibility check." 
                  "# @param IncidenceMatrix VIF"
                  "# @option Bool dual transposes the incidence matrix"
                  "# @option Bool verbose prints information about the check."
                  "# @return Polytope the resulting polytope under the assumption that //VIF// actually defines a polytope",
                  &check_poly,"check_poly(IncidenceMatrix { dual => 0 , verbose => 0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
