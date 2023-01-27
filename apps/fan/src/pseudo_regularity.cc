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
#include "polymake/Matrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/common/lattice_tools.h"
#include "polymake/polytope/solve_LP.h"

namespace polymake { namespace fan {

template <typename Scalar>
Matrix<Scalar> pseudo_regular(BigObject fan)
{
  const Matrix<Scalar> raysout= fan.give("RAYS");
  const Matrix<Scalar> rays = -raysout;
  const IncidenceMatrix<> max_cones = fan.give("MAXIMAL_CONES");
  const Int num_rays = rays.rows();
  const Int num_max_cones = max_cones.rows();
  const Int dim = rays.cols();//fan.give("FAN_DIM");
  const Int lp_dim = num_max_cones*dim+num_rays+1;

  // for the d-dim. fan, we construct a polytope in dimension [(#max.cones)*d + #rays + 1]
  // the first summand describes the vertices of the polytope whose normal fan shall be the given fan, the second the distances of the facet hyperplanes to the origin, the last one a slack variable epsilon >= 0

  // the first max. cone gives us the first part of the inequality matrix I and the equality matrix L

  const Int num_rays_cone0 = max_cones.row(0).size();
  const Int num_rays_not_cone0 =  num_rays - num_rays_cone0;

  SparseMatrix<Rational> L( num_rays_cone0 , lp_dim );
  Int r = 0;
  for (const Int it : max_cones.row(0)) {
    for (Int i = 0; i < dim; ++i) {
      L[r][ 0*dim+i ] = rays[it][i];
    }
    L[r][ num_max_cones*dim+it ] = -1;
    ++r;
  }

  const auto all_rays = sequence(0, num_rays);

  SparseMatrix<Rational> I( num_rays_not_cone0+num_rays+2 , lp_dim );
  // epsilon >= 0 & epsilon <= 1
  I[0][lp_dim-1] = -1;
  I[1][lp_dim-1] = 1;
  Int rr = 2;
  // the a_i's > 0
  for (Int i = 0; i < num_rays; ++i) {
    I[rr][ num_max_cones*dim+i ] = -1;
    I[rr][ lp_dim-1 ] = 1;
    ++rr;
  }
  // <n_i,p_j> < a_i for all normals n_i that are not rays of the cone C_j corresponding to the vertex p_j
  for (const Int it : all_rays - max_cones.row(0)) {
    for (Int i = 0; i < dim; ++i) {
      I[rr][ 0*dim+i ] = rays[it][i];
    }

    I[rr][ num_max_cones*dim+it ] = -1;
    I[rr][ lp_dim-1 ] = 1;
    ++rr;
  }

  for (Int i = 1 ; i < num_max_cones ; ++i) {
    const Int num_rays_cone_i = max_cones.row(i).size();
    const Int num_rays_not_cone_i =  num_rays - num_rays_cone_i;

    SparseMatrix<Rational> L_i( num_rays_cone_i, lp_dim);
    Int a = 0;
    for (const Int it : max_cones.row(i)) {
      for (Int j = 0 ; j < dim ; ++j) {
        L_i[a][ i*dim+j ] = rays[it][j];
      }
      L_i[a][ num_max_cones*dim+it ] = -1;
      ++a;
    }
    L /= L_i;

    SparseMatrix<Rational> I_i( num_rays_not_cone_i , lp_dim );
    Int aa = 0;
    for (const Int it : all_rays - max_cones.row(i)) {
      for (Int j = 0 ; j < dim ; ++j) {
        I_i[aa][ i*dim+j ] = rays[it][j];
      }
      I_i[aa][ num_max_cones*dim+it ] = -1;
      I_i[aa][ lp_dim-1 ] = 1;
      ++aa;
    }
    I /= I_i;
  }


  // each rays should be the normal of a FACET, not just representing a suppoting hyperplane through the corr. vertices
  // we make sure of this by demanding each facet-centroid to lie only on its facet defining hyperplane, that is NOT on the other hyperplanes
  for (Int i = 0 ; i < num_rays ; ++i) {
    SparseMatrix<Rational> I_i(num_rays-1 , lp_dim);
    const auto& cones_with_ray_i = max_cones.col(i);
    const Int num_cones_with_ray_i = cones_with_ray_i.size();
    Int curr = 0;
    for (Int k = 0 ; k < num_rays ; ++k) {
      if (k != i) {
        for (const Int it : cones_with_ray_i) {
          for (Int l = 0 ; l < dim ; ++l) {
            I_i[curr][ it*dim+l ] = rays[k][l];
          }
        }
        I_i[curr][ num_max_cones*dim+k ] = -num_cones_with_ray_i;
        ++curr;
      }
    }
    I /= I_i;
  }

  I = zero_vector<Scalar>() | -I;
  I[1][0] = 1;
  L = zero_vector<Scalar>() | L;

  const auto LP_sol = polytope::solve_LP(I, L, unit_vector<Scalar>(I.cols(), I.cols()-1), true);

  if (LP_sol.status == polytope::LP_status::valid &&
      LP_sol.objective_value > 0) {
     Matrix<Scalar> Facets(num_rays, dim+1);
     for (Int i = 0; i < num_rays ; ++i)
        Facets[i] = LP_sol.solution[ num_max_cones*dim+i+1 ] | -rays[i];

     return Facets;
  }

  return Matrix<Scalar>{};
}

FunctionTemplate4perl("pseudo_regular<Scalar>(PolyhedralFan<Scalar>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
