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
#include "polymake/IncidenceMatrix.h"
#include "polymake/hash_map"
#include "polymake/fan/intersection.h"
#include "polymake/FacetList.h"

namespace polymake { namespace fan {
      
template <typename Scalar>
BigObject intersection(BigObject fan,
                       const Matrix<Scalar>& H)
{
   const Matrix<Scalar>    R   = fan.give("RAYS");
   const Matrix<Scalar>    OL  = fan.give("ORTH_LINEALITY_SPACE");
   const Matrix<Scalar>    intersection_lineality = null_space(OL / H);
   const IncidenceMatrix<> MC  = fan.give("MAXIMAL_CONES");

   /*
     The data structure rays_in_max_cones below collects the cones in the intersected fan.
     One and the same ray might appear several times;
     for example, when intersecting the fan 
       RAYS => [1,0], [1,1], [0,1]
       MAXIMAL_CONES => [0,1], [1,2]
     with the hyperplane of equation
       [1,-1],
     the cone [1,1] appears twice.
    */
   FacetList rays_in_max_cones;
   hash_map<Vector<Scalar>, Int> index_of;
   Int next_index = 0;
   
   // Remember the rays we generated in index_of, and save the indices of the rays in the list of rays_in_max_cones
   for (auto rit = entire(rows(MC)); !rit.at_end(); ++rit) {
      rays_in_max_cones.insertMax(indices_of( rays_of_intersection(R.minor(*rit,All), intersection_lineality, H),
                                              index_of, next_index ));
   }
   
   Matrix<Scalar> ordered_rays(index_of.size(), R.cols());
   for (const auto& index_pair : index_of)
      ordered_rays[index_pair.second] = index_pair.first;

   return BigObject("PolyhedralFan", mlist<Scalar>(),
                    "RAYS", ordered_rays,
                    "MAXIMAL_CONES", IncidenceMatrix<>(rays_in_max_cones.size(), index_of.size(), entire(rays_in_max_cones)),
                    "LINEALITY_SPACE", intersection_lineality);
}
      
      
UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Construct a new fan as the intersection of given fan with a subspace."
                          "# @param PolyhedralFan F"
                          "# @param Matrix H equations of subspace"
                          "# @return PolyhedralFan",
                          "intersection<Scalar>(PolyhedralFan<type_upgrade<Scalar>> Matrix<type_upgrade<Scalar>>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
  
