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
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include "polymake/polytope/to_interface.h"

namespace polymake { namespace polytope {

namespace {

template<typename Scalar>
bool is_origin_inside(const Matrix<Scalar>& V,
                      const Set<int>& indices)
{
   const Matrix<Scalar> 
      ineqs(zero_vector<Scalar>(indices.size()) | unit_matrix<Scalar>(indices.size())),
      t_eqs(-unit_vector<Scalar>(V.cols(), 0) | T(V.minor(indices, All)));

   // to_simplex doesn't like zero rows, so we have to remove them
   const Set<int> row_basis = basis_rows(t_eqs);

   // to_simplex doesn't like empty polytopes, either
   if (row_basis.size() == t_eqs.cols())
      return false;

   return to_interface::to_input_feasible_impl(ineqs, Matrix<Scalar>(t_eqs.minor(row_basis, All)));
}

// return value: true means to continue decomposing, false means we're done in dimension e
template<typename Scalar>
bool decompose_impl(int e,
                    const Matrix<Scalar>& V, 
                    Set<int>& active_indices,
                    std::vector<Set<int> >& summand_list)
{
   const Matrix<Scalar> linear_hull_of_active(null_space(V.minor(active_indices, All) /
                                                         unit_vector<Scalar>(V.cols(), 0)));
   const Array<int> original_vertex(active_indices);
   
   for (Entire< Subsets_of_k<const Set<int>&> >::const_iterator a=entire(all_subsets_of_k(active_indices, e+1)); !a.at_end(); ++a) {

      // the linear hull is the affine hull of the vertices and the origin
      const Matrix<Scalar> linear_hull_of_selected(null_space(V.minor(*a, All) / 
                                                              linear_hull_of_active /
                                                              unit_vector<Scalar>(V.cols(), 0)));

      // if the linear hull of the selected indices is full-dimensional, we don't have a summand
      if (!linear_hull_of_selected.rows())
         continue;

      // evaluate the equations of the linear hull on the active vertices
      const Matrix<Scalar> vals(V.minor(active_indices, All) * T(linear_hull_of_selected));

      // the putative summand indices are the vertices of P inside the linear hull of the selected subset
      // row iterators don't have an index() method, so do it directly
      Set<int> summand_indices;
      for (int i=0; i<vals.rows(); ++i)
         if (vals[i] == zero_vector<Scalar>(vals.cols()))
            summand_indices += original_vertex[i];

      // check if the origin lies in the convex hull of the putative summand indices
      if (summand_indices.size() <= 1 ||
          !is_origin_inside(V, summand_indices))
         continue;
      
      // if the ranks of the linear hulls of the selected vertices and the rest don't add up, it wasn't a summand at all
      const Set<int> rest_indices(active_indices - summand_indices);
      const Matrix<Scalar> linear_hull_of_rest(null_space(V.minor(rest_indices, All) / 
                                                          linear_hull_of_active /
                                                          unit_vector<Scalar>(V.cols(), 0)));
      if (linear_hull_of_selected.rows() + linear_hull_of_rest.rows() != V.cols() - 1 - linear_hull_of_active.rows())
         continue;

      // check if the origin lies in the convex hull of the putative remaining indices
      if (rest_indices.size() &&
          !is_origin_inside(V, rest_indices))
         continue;

      // ok, we found a summand
      summand_list.push_back(summand_indices);
      active_indices -= summand_indices;

      // now signal the conditions under which we want to continue decomposing
      return (e < active_indices.size() && // enough indices left so that the current dimension makes sense
              active_indices.size() > 1);  // enough indices left so that the origin could possibly be contained inside
   }

   return false;
}


} // end anonymous namespace

template<typename Scalar>
Array<Set<int> > direct_sum_decomposition_indices(perl::Object P)
{
   const Matrix<Scalar> V = P.give("VERTICES");
   const int d = P.give("COMBINATORIAL_DIM");

   const bool centered = P.give("CENTERED");
   if (!centered) throw std::runtime_error("direct_sum_decomposition: input polytope must be CENTERED");
   
   std::vector<Set<int> > summand_list;
   Set<int> active_vertices(sequence(0,V.rows()));
   for (int i=0; i<d && i < active_vertices.size(); ++i) {
      while(active_vertices.size() &&
            decompose_impl(i, V, active_vertices, summand_list))
         ;
   }

   if (active_vertices.size())
      summand_list.push_back(active_vertices);
   
   return Array<Set<int> >(summand_list.size(), entire(summand_list));
}


UserFunctionTemplate4perl("# @category Producing a polytope from polytopes\n"
                          "# Decompose a given polytope into the direct sum of smaller ones, and return the vertex indices of the summands"
                          "# @param Polytope P"
                          "# @return Array<Set>",
                          "direct_sum_decomposition_indices<Scalar>(Polytope<Scalar>)"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
