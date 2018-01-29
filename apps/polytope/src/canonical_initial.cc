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
#include "polymake/linalg.h"
#include "polymake/permutations.h"
#include "polymake/polytope/canonicalize.h"

namespace polymake { namespace polytope {

// assumes that the first coordinate is in 0/+1/-1
// i.e. typically canonicalize_rays has been called before
// polytope model: the INPUT_RAYS define a cone, 
// and the polytope is the intersection with x_0=1
// the provided rays should be adjusted in a way that 
// they define the cone with the additional inequality (1,0,0,...)
// we do a step of Fourier-Motzkin elimination to achieve this
template <typename TMatrix> inline
void canonicalize_polytope_generators(GenericMatrix<TMatrix>& M)
{
   if (M.cols() == 0 && M.rows() != 0)
      throw std::runtime_error("canonicalize_polytope_generators - ambient dimension is 0");

   typedef typename TMatrix::element_type E;
   // collect the rays with non-zero first entry
   Set<int> pos, neg;
   Set< Vector<E> > zvecs;
   for (int i = 0; i < M.rows(); ++i) {
      if (M.top()(i,0) > 0) {
         pos.push_back(i);
      } else if (M.top()(i,0) < 0) {
         neg.push_back(i);
      } else {
         zvecs.insert(M.row(i));
      }
   }
   if (pos.size() == 0) {
      //   throw std::runtime_error("not a polytope or illegal empty polytope");
      // the polytope is empty if no positive points are found: truncate matrix
      // initial rule will set FEASIBLE to false
      rows(M).resize(0);
   } else {

      // Fourier-Motzkin elimination: 
      // we add all pairs of a ray with a positive and negative first coordinate
      // note that this requires that the first coordinates are all 0/+1/-1
      // taking also care of duplicates
      for (auto nit = entire(neg); !nit.at_end(); ++nit )
         for (auto pit = entire(pos); !pit.at_end(); ++pit ){
            Vector<E> vec = M.row(*nit) + M.row(*pit);
            canonicalize_oriented(find_in_range_if(entire(vec.top()), operations::non_zero()));
            // only add it if it does not already exist and is not the zero vector
            if (!zvecs.exists(vec) && !is_zero(vec)) {
               M /= vec;
               zvecs.insert(vec);
            }
         }
      // now remove the inequalities with negative first coordinates
      // we want to keep the order of the rays otherwise
      M = M.minor(~neg, All);
   }
}

template <typename TMatrix> inline
void add_extra_polytope_ineq(GenericMatrix<TMatrix>& M)
{
   if (M.cols() == 0) return;

   typedef typename TMatrix::element_type E;
   const auto extra_ineq = unit_vector<E>(M.cols(),0);
   for (auto r=entire(rows(M)); !r.at_end();  ++r)
      if (*r == extra_ineq) return;
   M /= extra_ineq;
}


FunctionTemplate4perl("canonicalize_polytope_generators(Matrix&) : void");
FunctionTemplate4perl("add_extra_polytope_ineq(Matrix&) : void");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
