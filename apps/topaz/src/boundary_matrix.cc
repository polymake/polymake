/* Copyright (c) 1997-2023
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
#include "polymake/topaz/complex_tools.h"
#include "polymake/hash_map"

namespace polymake { namespace topaz {

SparseMatrix<Integer> boundary_matrix(BigObject sc, Int d)
{
   const Lattice<BasicDecoration>& HD = sc.give("HASSE_DIAGRAM");
   const auto faces = HD.nodes_of_rank(d+1);
   const Int n_faces = faces.size();
   if (d==0) return ones_matrix<Integer>(n_faces, 1);

   const auto subfaces = HD.nodes_of_rank(d);
   const Int n_subfaces = subfaces.size();
   const Int dim = HD.rank()-2;
   if (d > dim) return zero_matrix<Integer>(1, n_subfaces);

   hash_map<Int, Int> face_index; //maps the index af a face to its number of occurence in the HD
   {
     Int i = 0;
     for (Int f : faces)
       face_index[f] = i++;
   }
   SparseMatrix<Integer> delta(n_faces,n_subfaces);

   Int c = 0;
   for (Int sf : subfaces) {
      auto subface = HD.face(sf);
      for (Int f : HD.out_adjacent_nodes(sf)) {
         auto face = HD.face(f);

         Int i = 0;   //find index of the vertex missing in current subface
         for (auto f_it = entire(face), sf_it = entire(subface); (*f_it)==(*sf_it) && !sf_it.at_end(); ++f_it, ++sf_it)
            ++i;

         delta(face_index[f],c) = pow(-1,i);
      }
      ++c;
   }
   return delta;

}

Function4perl(&boundary_matrix, "boundary_matrix_cpp(SimplicialComplex,Int)");

} }
