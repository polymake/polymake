/* Copyright (c) 1997-2021
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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/polytope/hasse_diagram.h"
#include "polymake/FacetList.h"
#include "polymake/list"

namespace polymake { namespace polytope {

// Maps indices of vertices of bounded complex to parent polytope vertex indices
template <typename Scalar>
Array<Int> find_bounded_mapping(const Matrix<Scalar>& bounded_matrix,
                                const Matrix<Scalar>& parent_matrix,
                                const Set<Int>& unbounded_rows)
{
   // this will throw an exception in the case of mismatch
   Array<Int> total_perm = find_permutation(rows(bounded_matrix), rows(parent_matrix.minor(~unbounded_rows, All))).value();
   Array<Int> bounded_order(sequence(0, parent_matrix.rows()) - unbounded_rows);
   return permuted(bounded_order, total_perm);
}

// Maps indices of parent polytope vertices to bounded complex vertex indices.
Array<Int> map_vertices_down(const Array<Int>& vertex_mapping, const Int n_parent_vertices)
{
   Array<Int> result(n_parent_vertices);
   const Set<Int> bounded_indices(vertex_mapping);
   copy_range(entire(sequence(0,vertex_mapping.size())), select(result, vertex_mapping).begin());
   copy_range(entire(sequence(vertex_mapping.size(), result.size() - vertex_mapping.size())),
              select(result, ~bounded_indices).begin());
   return result;
}

BigObject relabeled_bounded_hasse_diagram(const IncidenceMatrix<>& VIF, const Set<Int>& far_face, const Array<Int>& vertex_mapping)
{
   Lattice<BasicDecoration, Nonsequential> hd = bounded_hasse_diagram_computation(VIF, far_face);
   Array<Int> down_map = map_vertices_down(vertex_mapping, VIF.cols());
   Array<Int> inv_down_map(down_map.size());
   inverse_permutation(down_map, inv_down_map);
   hd.permute_faces(inv_down_map);
   return static_cast<BigObject>(hd);
}

FacetList
bounded_complex_from_face_lattice(BigObject HD_obj, const Set<Int>& far_face, const Array<Int>& vertex_mapping, const Int n_parent_vertices)
{
   Lattice<BasicDecoration, Sequential> HD(HD_obj);
   const Array<Int> down_map = map_vertices_down(vertex_mapping, n_parent_vertices);
   Array<Int> inv_down_map(down_map.size());
   inverse_permutation(down_map, inv_down_map);
   HD.permute_faces(inv_down_map);

   FacetList F(HD.nodes_of_rank(1).size());

   Set<Int> faces_to_visit;
   std::list<Int> Q;
   copy_range(entire(HD.nodes_of_rank(HD.rank()-1)), std::back_inserter(Q));

   while (!Q.empty()) {
      const Int f = Q.front(); Q.pop_front();
      if ((HD.face(f) * far_face).empty()) {
         F.insertMax(HD.face(f));
      } else {
         for (auto subf=HD.in_adjacent_nodes(f).begin();
              !subf.at_end(); ++subf)
            if (!faces_to_visit.collect(*subf))
               Q.push_back(*subf);
      }
   }

   F.squeeze();
   return F;
}

FunctionTemplate4perl("find_bounded_mapping(Matrix, Matrix, Set)");
Function4perl(&map_vertices_down, "map_vertices_down(Array<Int>, $)");
Function4perl(&relabeled_bounded_hasse_diagram, "relabeled_bounded_hasse_diagram(IncidenceMatrix, Set, Array<Int>)");
Function4perl(&bounded_complex_from_face_lattice, "bounded_complex(Lattice<BasicDecoration, Sequential> Set, Array<Int>, $)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
