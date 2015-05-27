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

#ifndef POLYMAKE_MATROID_LATTICE_OF_FLATS_TOOLS_H
#define POLYMAKE_MATROID_LATTICE_OF_FLATS_TOOLS_H

#include "polymake/polytope/face_lattice_tools.h"

namespace polymake { namespace matroid { namespace flat_lattice {

using namespace polytope::face_lattice;

/// Compute the lattice of flats (modifies compute from 'face_lattice_tools.h')
template <typename MatrixTop, typename DiagrammFiller, bool dual>
void compute_lattice_of_flats(const GenericIncidenceMatrix<MatrixTop>& VIF, DiagrammFiller HD, bool2type<dual> Dual, int dim_upper_bound=-1)
{
   std::list< Set<int> > Q;    // queue of flats, which have been seen but who's flats above have not been computed yet.
   FaceMap<> Faces;            // flates in the matroid world

   // The bottom node: loops (or dual: whole matroid)
   const int R=VIF.rows(), C=VIF.cols();
   if (dual)
      HD.add_node(sequence(0,R));
   else if(C==0){
      HD.add_node(Set<int>());
      return;
   }

   if ( C == 0 )  // the empty matroid
      return; 

   HD.increase_dim();
   int n, end_this_dim=0, d=0;

   if (__builtin_expect(C>1, 1)) {
      if(!dual){
      // The level: rank = 0 (loops)
         const Set<int> loops=accumulate(rows(VIF), operations::mul());
         n=HD.add_node(loops);
         Q.push_back(loops);
         end_this_dim=1;
         HD.increase_dim();
      }else{
      // The first level: rank-1 (hyperplanes)
         copy(entire(all_subsets_of_1(sequence(0,C))), std::back_inserter(Q));
         n= HD.add_nodes(C, cols(VIF).begin());
         end_this_dim=n+C;

         HD.increase_dim(); ++d;
         for (int i=n; i<end_this_dim; ++i)
            add_edge(HD,0,i,Dual);      
      }
      int end_next_dim=end_this_dim;

      if (__builtin_expect(C>1 && dim_upper_bound, 1)) {
         int old_n=n;
         for (;;) {
            Set<int> H = Q.front(); Q.pop_front();
            for (faces_one_above_iterator<Set<int>, MatrixTop> faces(H, VIF);  !faces.at_end();  ++faces) {
                  int &node_ref = Faces[c(faces->second, VIF)];
                  if (node_ref==-1) {
                     node_ref=HD.add_node(dual ? faces->first : faces->second);
                     Q.push_back(faces->second);
                     ++end_next_dim;
                  }
                  add_edge(HD,n,node_ref,Dual);
            }
            if (++n == end_this_dim) {
               if (__builtin_expect(Q.empty() || d == dim_upper_bound, 0)) {
                  // The top node: whole matroid (or dual: empty set)
                  if (end_this_dim == end_next_dim) {
                     if(dual && n==old_n+1) break; // there are loops 
                     n= dual ? HD.add_node(Set<int>()) : HD.add_node(sequence(0,C));
                     for (int i=old_n ; i<n; ++i)
                        add_edge(HD,i,n,Dual);
                  }
                  HD.increase_dim();
                  break;
               }else{
                  HD.increase_dim();
               }
               old_n=end_this_dim;
               ++d;  end_this_dim=end_next_dim;
            }
         }
      } else {
         end_this_dim=n;
      }
   }


}

}}} // end namespaces

#endif // POLYMAKE_MATROID_LATTICE_OF_FLATS_TOOLS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
