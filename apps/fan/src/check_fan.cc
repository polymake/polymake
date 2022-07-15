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
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/FacetList.h"

namespace polymake { namespace fan {
namespace {

bool is_face (const Set<Int>& F, const IncidenceMatrix<>& I)
{
   Set<Int> face;

   Int i = 0;
   while ( i < I.rows() && face.size() == 0 && incl(F,I[i]) > 0 ) ++i;

   if ( i == I.rows() ) {
      return false;
   }

   face = I[i];
   while ( i < I.rows() && face.size() > F.size() ) {
      if ( incl( F, I[i] ) <= 0 ) {
         face *= I[i];
      }
      ++i;
   }

   return ( face.size() == F.size() );
}

}


template <typename Coord>
BigObject check_fan_objects(const Array<BigObject>& all_cones, OptionSet options)
{
   const Int n_i_cones = all_cones.size();
   const bool verbose = options["verbose"];
   FacetList max_cones;

   const BigObjectType t = all_cones.element_type();
   const Int lineality_dim = all_cones[0].give("LINEALITY_DIM");
   const Int ambientDim = all_cones[0].give("CONE_AMBIENT_DIM");

   for (Int c_i = 0; c_i < n_i_cones; ++c_i) {
      BigObject cone = all_cones[c_i];
      const Matrix<Coord> c_rays = cone.give("RAYS");
      const Matrix<Coord> facets = cone.give("FACETS");
      const Matrix<Coord> eqs = cone.give("LINEAR_SPAN");
      const IncidenceMatrix<> inc = cone.give("RAYS_IN_FACETS");
      const Int cone_lin_dim = cone.give("LINEALITY_DIM");
      if (cone_lin_dim != lineality_dim) {
         if (verbose) cout << "Cones do not have the same lineality space." << endl;
         throw std::runtime_error("not a fan");
      }
      // test intersection property
      for (Int c_j = c_i+1; c_j < n_i_cones; ++c_j) {
         const Matrix<Coord> facets2 = all_cones[c_j].give("FACETS");
         const Matrix<Coord> eqs2 = all_cones[c_j].give("LINEAR_SPAN");
         const Matrix<Coord> c_rays2=all_cones[c_j].give("RAYS");

         BigObject inters(t, "INEQUALITIES", facets / facets2, "EQUATIONS", eqs / eqs2);
         const Matrix<Coord> int_rays = inters.give("RAYS");

         const Int n_int_rays = int_rays.rows();
         if (n_int_rays > 0) {
            // the rays of the intersection must be rays of the two cones
            Set<Int> rays_in_cone, rays_in_cone2;
            for (Int i = 0; i < n_int_rays; ++i) {
               bool found = false;
               for (Int j = 0; !found && j < c_rays.rows(); ++j) {
                  if (c_rays[j] == int_rays[i]) {
                     rays_in_cone.insert(j);
                     found = true;
                  }
               }
               if (!found) {
                  if (verbose) cout << "Ray " << int_rays[i] << " is not a ray of the cone with rays\n" << c_rays << "." << endl;
                  throw std::runtime_error("not a fan");
               }
               found = false;
               for (Int j = 0; !found && j < c_rays2.rows(); ++j) {
                  if (c_rays2[j] == int_rays[i]) {
                     rays_in_cone2.insert(j);
                     found = true;
                  }
               }
               if (!found) {
                  if (verbose) cout << "Ray " << int_rays[i] << " is not a ray of the cone with rays\n" << c_rays2 << "." << endl;
                  throw std::runtime_error("not a fan");
               }
            }
            // if the intersection has the same number of rays than one of the cones, it is one of the cones
            // otherwise we explicitely check if it is a face of
            if (!(n_int_rays == c_rays.rows() || n_int_rays == c_rays2.rows())) {
               if (!is_face(rays_in_cone, inc)) {
                  if (verbose) cout << "Intersection " << rays_in_cone << " from " << c_i << " with " << c_j << " not a face of the first one, RIF: " << inc << endl;
                  throw std::runtime_error("not a fan");
               }
               const IncidenceMatrix<> inc2 = all_cones[c_j].give("RAYS_IN_FACETS");
               if (!is_face(rays_in_cone2, inc2)) {
                  if (verbose) cout << "Intersection " << rays_in_cone2 << " from " << c_i << " with " << c_j << " not a face of the second one, RIF: " << inc2 << endl;
                  throw std::runtime_error("not a fan");
               }
            }
         }
      }
   }

   hash_map<Vector<Coord>, Int> rays;

   Int n_rays = 0;

   for (BigObject cone : all_cones) {
      const Matrix<Coord> c_rays = cone.give("RAYS");
      Set<Int> ray_indices;
      for (auto r = entire(rows(c_rays)); !r.at_end(); ++r) {
         auto r_iti = rays.find(*r);
         if (r_iti == rays.end()) {
            rays[*r] = n_rays;
            ray_indices.insert(n_rays++);
         } else {
            ray_indices.insert(r_iti->second);
         }
      }
      max_cones.insertMax(ray_indices);
      //FIXME: one could also extract and save other properties here.
   }

   // create ray matrix
   Matrix<Coord> R(n_rays, ambientDim);
   for (auto r = entire(rays); !r.at_end(); ++r)
      R.row(r->second) = r->first;

   BigObject f("PolyhedralFan", mlist<Coord>());

   const Matrix<Coord> lineality=all_cones[0].give("LINEALITY_SPACE");
   f.take("LINEALITY_SPACE") << lineality;
   f.take("RAYS") << R;
   f.take("MAXIMAL_CONES") << max_cones;
   return f;
}


template <typename Coord>
BigObject check_fan(const Matrix<Coord>& i_rays, const IncidenceMatrix<>& i_cones, OptionSet options)
{
   const Int n_i_cones = i_cones.rows();
   Matrix<Coord> linealitySpace;
   const Int ambientDim = i_rays.cols();
   if (!(options["lineality_space"] >> linealitySpace))
      linealitySpace = Matrix<Coord>(0, ambientDim);
   BigObjectType t("Cone", mlist<Coord>());
   Array<BigObject> all_cones(t, n_i_cones);
   for (Int i = 0; i < n_i_cones; ++i) {
      all_cones[i].take("INPUT_RAYS") << i_rays.minor(i_cones[i], All);
      all_cones[i].take("INPUT_LINEALITY") << linealitySpace;
   }
   BigObject f = check_fan_objects<Coord>(all_cones, options);
   f.take("INPUT_RAYS") << i_rays;
   f.take("INPUT_CONES") << i_cones;
   return f;
}

UserFunctionTemplate4perl("# @category Consistency check"
                          "# Checks whether a given set of //rays// together with a list //cones//"
                          "# defines a polyhedral fan."
                          "# If this is the case, the output is the [[PolyhedralFan]] defined by //rays//"
                          "# as [[INPUT_RAYS]], //cones// as [[INPUT_CONES]], //lineality_space// as"
                          "# [[LINEALITY_SPACE]] if this option is given."
                          "# @param Matrix rays"
                          "# @param IncidenceMatrix cones"
                          "# @option Matrix lineality_space Common lineality space for the cones."
                          "# @option Bool verbose prints information about the check."
                          "# @return PolyhedralFan",
                          "check_fan<Coord>(Matrix<Coord>, IncidenceMatrix; {lineality_space=>undef, verbose=>false})");

UserFunctionTemplate4perl("# @category Consistency check"
                          "# Checks whether the [[polytope::Cone]] objects form a polyhedral fan."
                          "# If this is the case, returns that [[PolyhedralFan]]."
                          "# @param Array<Cone> cones"
                          "# @option Bool verbose prints information about the check."
                          "# @return PolyhedralFan",
                          "check_fan_objects<Coord>(Cone<Coord> +; {verbose=>false})");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
