/* Copyright (c) 1997-2020
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
#include "polymake/Rational.h"
#include "polymake/ListMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/hash_map"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace fan {

namespace {

template <typename Coord>
Array<BigObject> construct_cones(const IncidenceMatrix<>& max_cones, const Matrix<Coord>& rays, const Matrix<Coord>& lin_space, const Int ambient_dim)
{
   BigObjectType cone_type("Cone", mlist<Coord>());

   const Int size = max_cones.rows();

   Array<BigObject> all_cones(cone_type, size);
   for (Int i = 0; i < size; ++i) {
      all_cones[i].take("RAYS") << rays.minor(max_cones[i], All);
      all_cones[i].take("LINEALITY_SPACE") << lin_space;
      all_cones[i].take("CONE_AMBIENT_DIM") << ambient_dim;
   }

   return all_cones;
}

}

template <typename Coord>
BigObject common_refinement(BigObject f1, BigObject f2)
{
   const Int ambient_dim = f1.give("FAN_AMBIENT_DIM");
   if (ambient_dim != Int(f2.give("FAN_AMBIENT_DIM")))
      throw std::runtime_error("common_refinement: dimension mismatch.");

   const Int d = f1.give("FAN_DIM");
   const IncidenceMatrix<> max_cones1=f1.give("MAXIMAL_CONES");
   Matrix<Coord> rays1=f1.give("RAYS");
   Matrix<Coord> lineality_space1=f1.give("LINEALITY_SPACE");
   Matrix<Coord> lineality_space2=f2.give("LINEALITY_SPACE");
   orthogonalize(entire(rows(lineality_space1)));
   orthogonalize(entire(rows(lineality_space2)));
   Matrix<Coord> lineality_space;
   if (lineality_space1.rows() == 0 || lineality_space2.rows() == 0){
      lineality_space = Matrix<Coord>(0,ambient_dim);
   }else if(rank(lineality_space1) == ambient_dim && rank(lineality_space2) == ambient_dim){
      lineality_space = unit_matrix<Coord>(ambient_dim);
   }else{
      lineality_space = null_space(null_space(lineality_space1) / null_space(lineality_space2));
      orthogonalize(entire(rows(lineality_space)));
   }

   const IncidenceMatrix<> max_cones2=f2.give("MAXIMAL_CONES");
   Matrix<Coord> rays2=f2.give("RAYS");
   const bool complete = f1.give("COMPLETE") && f2.give("COMPLETE");
   project_to_orthogonal_complement(rays1, lineality_space1);
   project_to_orthogonal_complement(rays2, lineality_space2);


   ListMatrix<Vector<Coord> > rays(0,ambient_dim);
   hash_map<Vector<Coord>, Int> ray_map;
   bool empty_not_yet_there = true;

   std::list<Set<Int>> new_max_cones;
   Array<BigObject> all_cones1 = construct_cones(max_cones1, rays1, lineality_space1, ambient_dim);
   Array<BigObject> all_cones2 = construct_cones(max_cones2, rays2, lineality_space2, ambient_dim);

   for (auto i1=entire(all_cones1); !i1.at_end(); ++i1) {
      for (auto i2=entire(all_cones2); !i2.at_end(); ++i2) {
         BigObject inters = call_function("intersection", *i1, *i2);
         const Int inters_dim = inters.give("CONE_DIM");
         if (inters_dim == d || !complete) {
            Matrix<Coord> inters_rays = inters.give("RAYS");
            project_to_orthogonal_complement(inters_rays, lineality_space);
            Array<Int> ray_indices(inters_rays.rows());
            Int index = 0;
            for (auto i = entire(rows(inters_rays)); !i.at_end(); ++i, ++index) {

               const Vector<Coord> ray=*i;
               const auto rep = ray_map.find(ray);
               if (rep != ray_map.end()) {
                  ray_indices[index]=rep->second;
               } else {
                  ray_indices[index]=rays.rows();
                  ray_map[ray]=rays.rows();
                  rays /= ray;
               }
            }
            Set<Int> new_cone;
            for (auto j=entire(sequence(0,index)); !j.at_end(); ++j)
               if (ray_indices[*j]>=0) new_cone.insert(ray_indices[*j]);
            if (inters_dim > 0 || empty_not_yet_there){
               new_max_cones.push_back(new_cone);
               if (inters_dim == 0) empty_not_yet_there = false;
            }
         }
      }
   }

   BigObject f_out("PolyhedralFan");

   f_out.take("COMPLETE") << complete;
   f_out.take("FAN_AMBIENT_DIM") << ambient_dim;
   f_out.take("LINEALITY_DIM") << lineality_space.rows();
   if (complete) {
      f_out.take("FAN_DIM") << d;
      f_out.take("RAYS") << rays;
      f_out.take("MAXIMAL_CONES") << new_max_cones;
      f_out.take("LINEALITY_SPACE") << lineality_space;
   } else {
      f_out.take("INPUT_RAYS") << rays;
      f_out.take("INPUT_CONES") << new_max_cones;
      f_out.take("INPUT_LINEALITY") << lineality_space;
   }

   return f_out;
}

UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Computes the common refinement of two fans."
                          "# @param PolyhedralFan f1"
                          "# @param PolyhedralFan f2"
                          "# @return PolyhedralFan",
                          "common_refinement<Coord>(PolyhedralFan<Coord>,PolyhedralFan<Coord>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
