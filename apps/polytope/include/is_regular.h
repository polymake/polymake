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

#ifndef POLYMAKE_POLYTOPE_IS_REGULAR_H
#define POLYMAKE_POLYTOPE_IS_REGULAR_H

#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include <fstream>

namespace polymake { namespace polytope {

namespace {    

template<typename Scalar, typename SetInt>
Vector<Scalar>
new_row(int i,
        const Matrix<Scalar>& vertices,
        const SetInt& basis,
        int basis_sign,
        Scalar basis_det)
{
   Vector<Scalar> new_row(vertices.rows());
   int s = basis_sign;
   new_row[i] = s * basis_det;
   for (const auto& k: basis) {
      s=-s;
      new_row[k] = s * det(vertices[i] / vertices.minor(basis-scalar2set(k), All));
   }
   return new_row;
}	
  
} // end anonymous namespace
  
template<typename Scalar, typename SetInt>
std::pair<const Matrix<Scalar>, const Matrix<Scalar>>
secondary_cone_ineq(const Matrix<Scalar> &verts, const Array<SetInt>& subdiv, perl::OptionSet options)
{
   const int n_vertices  = verts.rows();
   const int ambient_dim = verts.cols()-1;
   const int n_facets    = subdiv.size();

   //compute the set of all points that is not used in any face
   SetInt not_used(sequence(0,n_vertices));
   for (const auto& sd: subdiv)
      not_used -= sd;

   //compute a full-dimensional orthogonal projection if verts is not full_dimensional
   const Matrix<Scalar> affine_hull = null_space(verts);
   const int codim = affine_hull.rows();
   SetInt coords;
   for (auto i=entire(all_subsets_of_k(sequence(0,ambient_dim),codim)); !i.at_end(); ++i) {
     if (!is_zero(det(affine_hull.minor(All, *i)))) {
       coords = *i;
       break;
     }
   }
   const Matrix<Scalar> vertices = verts.minor(All,~coords);  
   const int dim = vertices.cols()-1;

   // the equations and inequalities for the possible weight vectors
   // (without right hand side which will be 0)
   ListMatrix<Vector<Scalar>> equats(0,n_vertices);
   ListMatrix<Vector<Scalar>> inequs(0,n_vertices);

   Matrix<Scalar> eqs;
   if (options["equations"] >> eqs)
      equats /= eqs;

   Set<int> tozero = options["lift_to_zero"];
   int face;
   if (!equats.rows() && tozero.empty() && options["lift_face_to_zero"]>>face)
      tozero += subdiv[face];

   for (const auto& j: tozero)
      equats /= unit_vector<Scalar>(n_vertices,j);


   // generate the equation and inequalities
   for (int i=0; i<n_facets; ++i) {
      const SetInt b(basis_rows(vertices.minor(subdiv[i],All)));

      // we have to map the numbers the right way:
      SetInt basis;
      int k = 0;
      auto l = entire(subdiv[i]);
      for(auto j=entire(b); !j.at_end(); ++j, ++k, ++l) {
         while(k<*j) {
            ++k;
            ++l;
         }
         basis.push_back(*l);
      }
      const Scalar basis_det  = det(vertices.minor(basis,All));
      const int    basis_sign = basis_det>0 ? 1 : -1;
      const SetInt non_basis  = subdiv[i]-basis;

      // for each maximal face F, all points have to be lifted to the same facet
      for (const auto& j: non_basis)
         equats /= new_row(j, vertices, basis, basis_sign, basis_det);

      // for all adjacent maximal faces, all vertices not contained in F have to be lifted
      // in the same direction
      for (int l=i+1; l<n_facets; ++l)
         if (rank(vertices.minor(subdiv[i]*subdiv[l],All)) == dim)
            inequs /= new_row(*((subdiv[l]-subdiv[i]).begin()), vertices, basis, basis_sign, basis_det);

      // additional equations for the non-used points
      for (const auto& l: not_used)
         inequs /= new_row(l, vertices, basis, basis_sign, basis_det);

   }
   return std::pair<const Matrix<Scalar>,const Matrix<Scalar>>(inequs, equats);
}

} }

#endif // POLYMAKE_POLYTOPE_IS_REGULAR_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
