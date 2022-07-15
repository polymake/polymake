/* Copyright (c) 1997-2022
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
#include "polymake/hash_map"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/FacetList.h"
#include "polymake/linalg.h"

namespace polymake { namespace fan {

template <typename Coord>
void remove_redundancies(BigObject f)
{
   const Int ambientDim = f.give("FAN_AMBIENT_DIM");
   const Matrix<Coord> i_rays = f.give("INPUT_RAYS");
   const IncidenceMatrix<> i_cones = f.give("INPUT_CONES");
   const Int n_i_cones=i_cones.rows();
   BigObjectType t("Cone", mlist<Coord>());

   Matrix<Coord> lineality_space;
   const Matrix<Coord> lin = f.lookup("INPUT_LINEALITY");
   BigObject(t, "INPUT_RAYS", i_rays.minor(i_cones.row(0), All), "INPUT_LINEALITY", lin)
      .give("LINEALITY_SPACE") >> lineality_space;

   hash_map<Vector<Coord>, Int> rays;
   FacetList max_cones;
   Int n_rays = 0;
   for (Int i = 0; i < n_i_cones; ++i) {
      BigObject cone(t, "INPUT_RAYS", i_rays.minor(i_cones.row(i),All), "INPUT_LINEALITY", lineality_space);
      const Matrix<Coord> c_rays = cone.give("RAYS");
      Set<Int> ray_indices;
      for (auto r = entire(rows(c_rays)); !r.at_end(); ++r) {
         const Vector<Coord> the_ray(*r);
         auto r_iti = rays.find(the_ray);
         if (r_iti == rays.end()) {
            bool found = false;
            for (auto rr = entire(rays); !found && !rr.at_end(); ++rr) {
              try {
                const Vector<Coord> v = lin_solve(T((rr->first)/lineality_space),the_ray);
                if (v[0] > 0) {
                  ray_indices.insert(rr->second);
                  found = true;
                }
              }catch(const infeasible&){}
            }
            if (!found) {
              rays[the_ray]=n_rays;
              ray_indices.insert(n_rays++);
            }
         }
         else ray_indices.insert(r_iti->second);
      }
      max_cones.insertMax(ray_indices);
   }

   // create ray matrix
   Matrix<Coord> R(n_rays,ambientDim);
   for (auto r=entire(rays); !r.at_end(); ++r)
      R.row(r->second)=r->first;
   f.take("RAYS")<<R;
   f.take("LINEALITY_SPACE")<<lineality_space;

   // Take care of the empty fan and the fan containing only the origin
   if(max_cones.empty() && n_i_cones > 0)
      f.take("MAXIMAL_CONES")<< IncidenceMatrix<>(1,0);
   else
      f.take("MAXIMAL_CONES")<<max_cones;
}

FunctionTemplate4perl("remove_redundancies<Coord>(PolyhedralFan<Coord>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
