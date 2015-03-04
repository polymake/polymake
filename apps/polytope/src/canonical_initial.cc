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
template <typename Matrix> inline
void canonicalize_polytope_generators(GenericMatrix<Matrix>& M)
{
   typedef typename Matrix::element_type E;
   // collect the rays with non-zero first entry
   Set<int> pos, neg;
   Set< Vector<E> > zvecs;
   for (int i = 0; i < M.top().rows(); ++i) {
      if ( M.top()(i,0) > 0 ) {
         pos.push_back(i);
      } else if ( M.top()(i,0) < 0 ) {
         neg.push_back(i);
      } else {
         zvecs.insert(M.top().row(i));
      }
   }
   if ( pos.size() == 0 ) 
      //   throw std::runtime_error("not a polytope or illegal empty polytope");
      // the polytope is empty if no positive points are found: truncate matrix
      // initial rule will set FEASIBLE to false
      M.top().clear();
   else {

      // Fourier-Motzkin elimination: 
      // we add all pairs of a ray with a positive and negative first coordinate
      // note that this requires that the first coordinates are all 0/+1/-1
      // taking also care of duplicates
      for ( Entire<Set<int> >::const_iterator nit = entire(neg); !nit.at_end(); ++nit )
         for ( Entire<Set<int> >::const_iterator pit = entire(pos); !pit.at_end(); ++pit ){
            Vector<E> vec = M.top()[*nit] + M.top()[*pit];
            canonicalize_oriented(find_if(entire(vec.top()), operations::non_zero()));
            // only add it if it does not already exist and is not the zero vector
            if(!zvecs.exists(vec) && vec != zero_vector<E>(vec.dim())){
               M.top() /= vec;
               zvecs.insert(vec);
            }
         }
      // now remove the inequalities with negative first coordinates
      // we want to keep the order of the rays otherwise
      M.top() = M.top().minor(~neg,All);
   }
}

template <typename MatrixTop> inline
void add_extra_polytope_ineq(perl::Object p, GenericMatrix<MatrixTop>& M, const int d)
{
   typedef typename MatrixTop::element_type E;
   if (M.rows()==0) {
      M /= unit_vector<E>(d,0);
   } else {
      const Vector<E> extra_ineq(unit_vector<E>(d,0));
      for (typename Entire< Rows<MatrixTop> >::iterator r=entire(rows(M)); !r.at_end();  ++r)
         if (*r == extra_ineq) return;
      M /= extra_ineq;
   }
}


FunctionTemplate4perl("canonicalize_polytope_generators(Matrix&) : void");
FunctionTemplate4perl("add_extra_polytope_ineq(Polytope,Matrix&,$) : void");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
