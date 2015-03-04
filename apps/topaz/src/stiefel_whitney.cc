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
#include "polymake/topaz/FiniteFields.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/topaz/SimplicialComplex_as_FaceMap.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include <cassert>

namespace polymake { namespace topaz {
  
namespace {

bool regular(const Set<int>& s, const Set<int>& t)
{
   Entire< Set<int> >::const_iterator i(entire(s));
   assert(!i.at_end());
   Set<int>::const_iterator j(t.begin());
   assert(!j.at_end());

   // check B_{-1}
   if (*j<*i) return false;
   assert(*j==*i);
   ++i;                      // now index odd

   // check B_m
   while (!i.at_end()) {
      while (*j<*i) ++j;     // terminates since s subset of t
      assert(*j==*i);

      ++i; ++j;              // now index even
      if (i.at_end()) {
         // check B_p, if p is odd
         if (!j.at_end()) return false;
         else return true;
      }

      assert(!j.at_end());
      if (*i!=*j) return false;
      
      ++i;                    // now index odd
   }

   return true;
}

} // end unnamed namespace

Array<PowerSet<int> > stiefel_whitney(const Array<Set<int> >& facets, perl::OptionSet options)
{
   const bool verbose=options["verbose"];

   typedef SimplicialComplex_as_FaceMap<> sc_type;
   sc_type SC (facets);
   
   const int d(SC.dim());
   int high_d;
   if(!(options["high_d"]>>high_d)) high_d=d;
   int low_d;
   if(!(options["low_d"]>>low_d)) low_d=0;
   
   if (low_d<0) low_d+=d+1;
   if (high_d<0) high_d+=d+1;
   
   if (high_d < low_d || high_d > d) {
      std::ostringstream e;
      e << "stiefel_whitney: dim_low(" << low_d << "), dim_high("
        << high_d << ") out of bounds" << endl;
      throw std::runtime_error(e.str());
   }
   
   Array< PowerSet<int> > omega_cycle(high_d-low_d+1);
   Array< PowerSet<int> >::iterator omega_cycle_k=omega_cycle.begin();
   SC.complete_faces(d,low_d);
   
   for (int k=low_d; k<=high_d; ++k, ++omega_cycle_k) {
      const int size_k(SC.size_of_dim(k));
      Array< Set<int> > face(size_k);
      if (verbose)
         cout << "f_" << k << "=" << size_k << ", regular pairs:" << std::endl;
      SparseVector<GF2> omega(size_k);
      for (int l=k; l<=d; ++l)
         for (Entire<sc_type::Faces_of_Dim>::const_iterator t(entire(SC.faces_of_dim(l))); !t.at_end(); ++t) {
            for (Entire< Subsets_of_k<sc_type::reference> >::const_iterator s(entire(all_subsets_of_k(*t,k+1))); !s.at_end(); ++s) {
               if (regular(*s,*t)) {
                  if (verbose)
                     cout << " " << *s << *t << endl;
                  const int s_idx(SC[*s]);
                  if (face[s_idx].empty()) face[s_idx]=*s;
                  omega[s_idx]+=GF2(1);
               }
            }
         }
      if (k!=d) {
         SparseMatrix<GF2> delta_kk(SC.boundary_matrix<GF2>(k+1));
         omega = reduce(delta_kk,omega);
      }
      
      accumulate_in(entire(select(face,indices(omega))), operations::add(), *omega_cycle_k);
      if (verbose)
         cout << "omega_" << k << ":\n" << *omega_cycle_k << endl;
   }
   
   return omega_cycle;
}

UserFunction4perl("# @category Other"
                  "# Computes __Stiefel-Whitney classes__ of mod 2 Euler space (in particular, closed manifold).\n"
                  "# Use option //verbose// to show regular pairs and cycles.\n"
                  "# A narrower dimension range of interest can be specified.\n"
                  "# Negative values are treated as co-dimension - 1\n"
                  "# @param Array<Set<Int>> facets the facets of the simplicial complex"
                  "# @option Int high_dim"
                  "# @option Int low_dim"
                  "# @option Bool verbose"
                  "# @return Array<PowerSet<Int>>",
                  &stiefel_whitney,"stiefel_whitney(Array<Set<Int>> { high_dim => undef, low_dim => undef, verbose => 0})");
   
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
