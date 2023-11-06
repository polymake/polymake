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
#include "polymake/hash_map"
#include "polymake/list"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/FacetList.h"
#include "polymake/linalg.h"

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

template <typename Coord>
std::list<Int> get_indices(const Matrix<Coord>& c_rays, hash_map<Vector<Coord>, Int>& rays, bool insert=true, bool verbose=true) {
   std::list<Int> ray_indices;
   Int n_rays = rays.size();
   for (auto r = entire(rows(c_rays)); !r.at_end(); ++r) {
      auto r_iti = rays.find(*r);
      if (r_iti == rays.end()) {
         if (!insert) {
            if (verbose)
               cout << "Intersection ray " << *r << " does not appear in any given cone." << endl;
            throw std::runtime_error("not a fan");
         }
         rays[*r] = n_rays;
         ray_indices.push_back(n_rays++);
      } else {
         ray_indices.push_back(r_iti->second);
      }
   }
   return ray_indices;
}

}

template <typename Coord>
BigObject fan_from_objects(const Array<BigObject>& all_cones, OptionSet options)
{
   const Int n_i_cones = all_cones.size();
   if (n_i_cones < 1)
      throw std::runtime_error("fan_from_objects: at least one cone required");

   const bool verbose = options["verbose"];
   const bool check = options["check"];
   const bool cplx = options["complex"];
   FacetList max_cones;

   const BigObjectType t = all_cones.element_type();
   const Int ambientDim = all_cones[0].give("CONE_AMBIENT_DIM");
   const Matrix<Coord> lineality = all_cones[0].give("LINEALITY_SPACE");

   hash_map<Vector<Coord>, Int> rays;
   Array<Array<Int>> cone_rays(n_i_cones);

   for (Int c_i = 0; c_i < n_i_cones; ++c_i) {
      BigObject cone = all_cones[c_i];
      Matrix<Coord> c_rays = cone.give("RAYS");
      project_to_orthogonal_complement(c_rays, lineality);

      auto ray_indices = get_indices(c_rays, rays);

      max_cones.insertMax(Set<Int>(ray_indices));
      // we keep these indices for the extra checks
      if (check)
         cone_rays[c_i] = ray_indices;
   }

   if (check) {
      for (Int c_i = 0; c_i < n_i_cones; ++c_i) {
         BigObject cone = all_cones[c_i];
         Matrix<Coord> c_rays = cone.give("RAYS");
         const Matrix<Coord> cone_lin = cone.give("LINEALITY_SPACE");
         if (cone_lin.rows() != rank(lineality/cone_lin)) {
            if (verbose) cout << "Cones do not have the same lineality space." << endl;
            throw std::runtime_error("not a fan");
         }

         const Matrix<Coord> facets  = cone.give("FACETS");
         const Matrix<Coord> eqs     = cone.give("LINEAR_SPAN");
         const IncidenceMatrix<> inc = cone.give("RAYS_IN_FACETS");

         // test intersection property
         for (Int c_j = c_i+1; c_j < n_i_cones; ++c_j) {
            const Matrix<Coord> facets2 = all_cones[c_j].give("FACETS");
            const Matrix<Coord> eqs2 = all_cones[c_j].give("LINEAR_SPAN");

            BigObject inters(t, "INEQUALITIES", facets / facets2, "EQUATIONS", eqs / eqs2);
            Matrix<Coord> int_rays = inters.give("RAYS");
            project_to_orthogonal_complement(int_rays, lineality);

            auto int_indices = Set<Int>(get_indices(int_rays, rays, false, verbose));
            const Int n_int_rays = int_rays.rows();

            if (n_int_rays > 0) {
               Set<Int> rays_in_cone1, rays_in_cone2;
               auto op1 = attach_operation(cone_rays[c_i], sequence(0, cone_rays[c_i].size()), operations::pair_maker());
               hash_map<Int,Int> cone_ray_ind1(op1.begin(), op1.end());
               auto op2 = attach_operation(cone_rays[c_j], sequence(0, cone_rays[c_j].size()), operations::pair_maker());
               hash_map<Int,Int> cone_ray_ind2(op2.begin(), op2.end());
               for (Int iri : int_indices) {
                  auto c_ind = cone_ray_ind1.find(iri);
                  if (c_ind == cone_ray_ind1.end()) {
                     if (verbose) cout << "Rays of intersection c_" << c_i << " \\cap c_" << c_j << " not a subset of the rays of the cone c_" << c_i << "." << endl;
                     throw std::runtime_error("not a fan");
                  } else
                     rays_in_cone1 += c_ind->second;

                  c_ind = cone_ray_ind2.find(iri);
                  if (c_ind == cone_ray_ind2.end()) {
                     if (verbose) cout << "Rays of intersection c_" << c_i << " \\cap c_" << c_j << " not a subset of the rays of the cone c_" << c_j << "." << endl;
                     throw std::runtime_error("not a fan");
                  } else
                     rays_in_cone2 += c_ind->second;
               }

               // if the intersection has the same number of rays as one of the cones, it is one of the cones
               // otherwise we explicitely check if it is a face of
               if (n_int_rays != cone_rays[c_i].size() && !is_face(rays_in_cone1, inc)) {
                  if (verbose) cout << "Intersection " << rays_in_cone1 << " from " << c_i << " with " << c_j << " not a face of the first one, RIF: " << inc << endl;
                  throw std::runtime_error("not a fan");
               }
               const IncidenceMatrix<> inc2 = all_cones[c_j].give("RAYS_IN_FACETS");
               if (n_int_rays != cone_rays[c_j].size() && !is_face(rays_in_cone2, inc2)) {
                  if (verbose) cout << "Intersection " << rays_in_cone2 << " of c_" << c_i << " with c_" << c_j << " not a face of the second one, RIF:" << endl << inc2 << endl;
                  throw std::runtime_error("not a fan");
               }
            }
         }
      }
   }

   auto mc = max_cones.empty() ? IncidenceMatrix<>(1,0) : IncidenceMatrix<>(max_cones);

   // create ray matrix
   Matrix<Coord> R(rays.size(), ambientDim);
   for (auto r = entire(rays); !r.at_end(); ++r)
      R.row(r->second) = r->first;

   BigObject f(cplx ? AnyString("PolyhedralComplex") : AnyString("PolyhedralFan"), mlist<Coord>());

   f.take("LINEALITY_SPACE") << lineality;
   f.take("RAYS") << R;
   f.take("MAXIMAL_CONES") << mc;
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
   options["check"] << true;
   BigObject f = fan_from_objects<Coord>(all_cones, options);
   f.take("INPUT_RAYS") << i_rays;
   f.take("INPUT_CONES") << i_cones;
   return f;
}

template <typename Coord>
BigObject check_fan_objects(const Array<BigObject>& all_cones, OptionSet options) {
   options["check"] << true;
   options["complex"] << false;
   return fan_from_objects<Coord>(all_cones, options);
}
template <typename Coord>
BigObject check_complex_objects(const Array<BigObject>& all_cones, OptionSet options) {
   options["check"] << true;
   options["complex"] << true;
   return fan_from_objects<Coord>(all_cones, options);
}
template <typename Coord>
BigObject fan_from_cones(const Array<BigObject>& all_cones, OptionSet options) {
   options["check"] << false;
   options["complex"] << false;
   return fan_from_objects<Coord>(all_cones, options);
}
template <typename Coord>
BigObject complex_from_polytopes(const Array<BigObject>& all_cones, OptionSet options) {
   options["check"] << false;
   options["complex"] << true;
   return fan_from_objects<Coord>(all_cones, options);
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

UserFunctionTemplate4perl("# @category Consistency check"
                          "# Checks whether the [[polytope::Polytope]] objects form a polyhedral complex."
                          "# If this is the case, returns that [[PolyhedralComplex]]."
                          "# @param Array<Polytope> polytopes"
                          "# @option Bool verbose prints information about the check."
                          "# @return PolyhedralComplex",
                          "check_complex_objects<Coord>(Polytope<Coord> +; {verbose=>false})");

UserFunctionTemplate4perl("# @category Producing a fan"
                          "# Construct a polyhedral fan from a list of [[polytope::Cone]] objects."
                          "# No intersection checks are perfomed but the rays lists are canonicalized and merged."
                          "# Warning: This might result in an invalid object if the input is not correct."
                          "# @param Array<Cone> cones"
                          "# @return PolyhedralFan",
                          "fan_from_cones<Coord>(Cone<Coord> +; {verbose=>true})");

UserFunctionTemplate4perl("# @category Producing a polyhedral complex"
                          "# Construct a polyhedral complex from a list of [[polytope::Polytope]] objects."
                          "# No intersection checks are perfomed but the rays lists are canonicalized and merged."
                          "# Warning: This might result in an invalid object if the input is not correct."
                          "# @param Array<Polytope> polytopes"
                          "# @return PolyhedralComplex",
                          "complex_from_polytopes<Coord>(Polytope<Coord> +; {verbose=>true})");

FunctionTemplate4perl("fan_from_objects<Coord>(Cone<Coord> +; {verbose=>true, check=>false, complex=>false})");
} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
