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

   if (complete) {
      return BigObject("PolyhedralFan", "COMPLETE", complete, "FAN_AMBIENT_DIM", ambient_dim, "LINEALITY_DIM", lineality_space.rows(),
                       "FAN_DIM", d, "RAYS", rays, "MAXIMAL_CONES", new_max_cones, "LINEALITY_SPACE", lineality_space);
   } else {
      return BigObject("PolyhedralFan", "COMPLETE", complete, "FAN_AMBIENT_DIM", ambient_dim, "LINEALITY_DIM", lineality_space.rows(),
                       "INPUT_RAYS", rays, "INPUT_CONES", new_max_cones, "INPUT_LINEALITY", lineality_space);
   }
}

UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Computes the common refinement of two fans. This is the fan made of"
                          "# all intersections of cones of the first fan with cones of the"
                          "# second fan. Note that the support of the result is the intersection"
                          "# of the supports of the input fans."
                          "# @param PolyhedralFan f1"
                          "# @param PolyhedralFan f2"
                          "# @return PolyhedralFan"
                          "# @example [prefer cdd] Two two-dimensional fans with different support"
                          "# > $s = simplex(2);"
                          "# > $c = new Polytope(POINTS=>[[1,0,0],[0,-1,0],[0,0,-1]]);"
                          "# > $f1 = normal_fan($s);"
                          "# > $f2 = normal_fan($c);"
                          "# > print $f1->RAYS;"
                          "# | -1 -1"
                          "# | 1 0"
                          "# | 0 1"
                          "# > print $f1->MAXIMAL_CONES;"
                          "# | {1 2}"
                          "# | {0 2}"
                          "# | {0 1}"
                          "# > print $f2->RAYS;"
                          "# | -1 0"
                          "# | 0 -1"
                          "# > print $f2->MAXIMAL_CONES;"
                          "# | {0 1}"
                          "# > $cc = common_refinement($f1,$f2);"
                          "# > print $cc->RAYS;"
                          "# | -1 -1"
                          "# | -1 0"
                          "# | 0 -1"
                          "# > print $cc->MAXIMAL_CONES;"
                          "# | {0 1}"
                          "# | {0 2}",
                          "common_refinement<Coord>(PolyhedralFan<Coord>,PolyhedralFan<Coord>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
