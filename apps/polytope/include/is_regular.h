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

#pragma once

#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include <fstream>

namespace polymake { namespace polytope {

namespace {    

template<typename Scalar, typename SetInt, typename Matrix>
SparseVector<Scalar>
new_row(Int i,
        const GenericMatrix<Matrix,Scalar>& vertices,
        const SetInt& basis,
        Int basis_sign,
        Scalar basis_det)
{
   SparseVector<Scalar> new_row(vertices.rows());
   Int s = basis_sign;
   new_row[i] = s * basis_det;
   for (const auto& k: basis) {
      s = -s;
      new_row[k] = s * det(vertices[i] / vertices.minor(basis-scalar2set(k), All));
   }
   return new_row;
}	
  
} // end anonymous namespace

template<typename Scalar, typename TMatrix>
Matrix<Scalar>
full_dim_projection(const GenericMatrix<TMatrix, Scalar>& verts)
{
   const Int ambient_dim = verts.cols();
   const auto affine_hull = null_space(verts);
   const Int codim = affine_hull.rows();
   if (codim == 0)
      return verts;
   for (auto i = entire(all_subsets_of_k(sequence(0, ambient_dim),codim)); !i.at_end(); ++i)
      if (!is_zero(det(affine_hull.minor(All, *i))))
         return verts.minor(All, ~(Set<Int>(*i)));

   throw std::runtime_error("full_dim_projection: This shouldn't happen");
}
      
template<typename Scalar, typename SetInt, typename Matrix>
std::pair<const SparseMatrix<Scalar>, const SparseMatrix<Scalar>>
secondary_cone_ineq(const GenericMatrix<Matrix, Scalar>& full_dim_verts, const Array<SetInt>& subdiv, OptionSet options)
{
#if POLYMAKE_DEBUG
   if (rank(full_dim_verts) != full_dim_verts.cols())
      throw std::runtime_error("secondary_cone_ineq: need full-dimensional vertices. Use full_dim_projection on your vertices first.");
#endif

   const Int n_vertices  = full_dim_verts.rows();
   const Int ambient_dim = full_dim_verts.cols()-1;
   const Int n_facets    = subdiv.size();

   //compute the set of all points that is not used in any face
   SetInt not_used(sequence(0,n_vertices));
   for (const auto& sd: subdiv)
      not_used -= sd;

   // the equations and inequalities for the possible weight vectors
   // (without right hand side which will be 0)
   ListMatrix<SparseVector<Scalar>> equats(0,n_vertices);
   ListMatrix<SparseVector<Scalar>> inequs(0,n_vertices);

   SparseMatrix<Scalar> eqs;
   if (options["equations"] >> eqs)
      equats /= eqs;

   Set<Int> tozero = options["lift_to_zero"];
   Int face;
   if (!equats.rows() && tozero.empty() && options["lift_face_to_zero"]>>face)
      tozero += subdiv[face];

   for (const auto& j: tozero)
      equats /= unit_vector<Scalar>(n_vertices,j);


   // generate the equation and inequalities
   for (Int i = 0; i < n_facets; ++i) {
      const SetInt b(basis_rows(full_dim_verts.minor(subdiv[i], All)));

      // we have to map the numbers the right way:
      SetInt basis;
      {
         Int k = 0;
         auto l = entire(subdiv[i]);
         for (auto j = entire(b); !j.at_end(); ++j, ++k, ++l) {
            while (k < *j) {
               ++k;
               ++l;
            }
            basis.push_back(*l);
         }
      }
      const Scalar basis_det  = det(full_dim_verts.minor(basis, All));
      const Int    basis_sign = basis_det > 0 ? 1 : -1;
      const SetInt non_basis  = subdiv[i]-basis;

      // for each maximal face F, all points have to be lifted to the same facet
      for (const auto& j: non_basis)
         equats /= new_row(j, full_dim_verts, basis, basis_sign, basis_det);

      // for all adjacent maximal faces, all vertices not contained in F have to be lifted
      // in the same direction
      for (Int f = i+1; f < n_facets; ++f)
         if (rank(full_dim_verts.minor(subdiv[i] * subdiv[f], All)) == ambient_dim)
            inequs /= new_row(*((subdiv[f]-subdiv[i]).begin()), full_dim_verts, basis, basis_sign, basis_det);

      // additional equations for the non-used points
      for (const auto& l: not_used)
         inequs /= new_row(l, full_dim_verts, basis, basis_sign, basis_det);

   }
   return std::pair<const SparseMatrix<Scalar>,const SparseMatrix<Scalar>>(remove_zero_rows(inequs), remove_zero_rows(equats));
}

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
