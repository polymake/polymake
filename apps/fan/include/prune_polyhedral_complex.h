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
#include "polymake/graph/Closure.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/BasicLatticeTypes.h"
#include "polymake/graph/lattice_builder.h"
#include "polymake/linalg.h"

namespace polymake { namespace fan {


// Certain constructions that are implemented for PolyhedralFan do not lift to
// PolyhedralComplex naturally. One such construction would be taking the
// k_skeleton: If the PolyhedralComplex has rays, then one might get cones that
// only live in the 0-plane, and hence do not contribute feasible polytopes.
// The other such construction that we know of is the common_refinement.
//
// This function eliminates the infeasible polytopes and afterwards also
// removes unused rays.
template<typename Scalar>
BigObject prune_polyhedral_complex(BigObject input){
   BigObject result("PolyhedralComplex", mlist<Scalar>());
   bool has_rays = input.exists("RAYS");
   Matrix<Scalar> rays = has_rays ? input.give("RAYS") : input.give("INPUT_RAYS");
   if(indices(attach_selector(rays.col(0), polymake::operations::negative())).size() != 0){
      throw std::runtime_error("Input fan extends below height 0.");
   }
   IncidenceMatrix<NonSymmetric> cones = has_rays ? input.give("MAXIMAL_CONES") : input.give("INPUT_CONES");
   Matrix<Scalar> lineality(0, rays.cols());
   Int ldim = input.give("LINEALITY_DIM");
   if(ldim > 0){
      input.give("LINEALITY_SPACE | INPUT_LINEALITY") >> lineality;
      if(indices(attach_selector(lineality.col(0), polymake::operations::non_zero())).size() != 0){
         throw std::runtime_error("Input fan extends below height 0.");
      }
   }
   
   Set<Int> far_vertices = far_points(rays);
   if(far_vertices.size() != 0){
      Set<Int> feasible_polytopes, used_rays;
      for(Int i=0; i<cones.rows(); i++){
         if(incl(cones.row(i), far_vertices) > 0){
            used_rays += cones.row(i);
            feasible_polytopes += i;
         }
      }
      rays = rays.minor(used_rays, All);
      cones = cones.minor(feasible_polytopes, used_rays);
   }

   if(has_rays){
      result.take("VERTICES") << rays;
      result.take("MAXIMAL_POLYTOPES") << cones;
      result.take("LINEALITY_SPACE") << lineality;
   } else {
      result.take("POINTS") << rays;
      result.take("INPUT_POLYTOPES") << cones;
      result.take("INPUT_LINEALITY") << lineality;
   }
   return result;
}

} // namespace fan 
} // namespace polymake

