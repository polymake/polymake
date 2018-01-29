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

#ifndef POLYMAKE_FAN_FACE_LATTICE_TOOLS_H
#define POLYMAKE_FAN_FACE_LATTICE_TOOLS_H

#include "polymake/polytope/face_lattice_tools.h"

namespace polymake { namespace fan { namespace face_lattice {

/// Compute the lattice of a tight span, starting always from the vertices of the tight span (dual)
/// the excluded faces are in primal form
template <typename TMatrix, typename DiagrammFiller>
void compute_tight_span(const GenericIncidenceMatrix<TMatrix>& VIF,
                     const Set<Set<int> >& excluded_faces,
                     DiagrammFiller HD, int dim_upper_bound=-1)
{
   std::list< Set<int> > Q;    // queue of faces, which have been seen but who's faces above have not been computed yet.
   FaceMap<> Faces;
   
   // The bottom node: empty set
   const int C=VIF.cols();
   HD.add_node(Set<int>());
   HD.increase_dim();
   int end_this_dim=0, end_next_dim=0, d=0, max_faces_cnt=0;

   // The first level: vertices.
   const Set<int> vertices=sequence(0,C);

   if (__builtin_expect(C>1, 1)) {
      copy_range(entire(all_subsets_of_1(vertices)), std::back_inserter(Q));
      int n=HD.add_nodes(C, all_subsets_of_1(vertices).begin());
      end_next_dim=end_this_dim=n+C;
      HD.increase_dim(); ++d;
      for (int i=n; i<end_this_dim; ++i)
         HD.add_edge(0,i);

      if (__builtin_expect(dim_upper_bound, 1)) {
         for (;;) {
            Set<int> H = Q.front(); Q.pop_front();
            bool is_max_face=true;
            for (polytope::face_lattice::faces_one_above_iterator<Set<int>, TMatrix> faces(H, VIF);  !faces.at_end();  ++faces) {
               int &node_ref = Faces[polytope::face_lattice::c(faces->second, VIF)];
               if (node_ref==-1) {
                  bool excluded = false;
                  for( auto f = entire(excluded_faces); !f.at_end() ;++f )
                     if( incl(faces->first, *f) < 1 ){
                        excluded=true;
                        break;
                     }
                  if (!excluded) {
                     node_ref=HD.add_node(faces->second);
                     Q.push_back(faces->second);
                     ++end_next_dim;
                  } else {
                     node_ref=-2;
                     continue;
                  }
               } else if (node_ref==-2)
                  continue;
               HD.add_edge(n,node_ref);
               is_max_face=false;
            }
            if (is_max_face) ++max_faces_cnt;
            if (++n == end_this_dim) {
               if (__builtin_expect(Q.empty() || d == dim_upper_bound, 0)) break;
               HD.increase_dim();
               ++d;  end_this_dim=end_next_dim;
            }
         }
      }
   }

   if (max_faces_cnt + end_next_dim-end_this_dim > 1) {
      // The top node is connected to all inclusion-independent faces regardless of the dimension
      int n=HD.add_node(vertices);
      for (int i=0; i<n; ++i)
         if (HD.graph().out_degree(i)==0)
            HD.add_edge(i,n);
   }

}


} } // end namespace face_lattice

} // end namespace pm

#endif // POLYMAKE_FAN_FACE_LATTICE_TOOLS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

