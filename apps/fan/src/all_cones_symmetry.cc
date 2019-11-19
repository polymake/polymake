/* Copyright (c) 1997-2019
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
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/hash_set"
#include "polymake/hash_map"

namespace polymake { namespace fan {

using graph::Lattice;
using graph::lattice::Sequential;
using graph::lattice::BasicDecoration;

void all_cones_symmetry(perl::Object f, int dim)
{
   const Array<Set<int>> cones=f.give("MAXIMAL_CONES_REPS");
   const Matrix<Rational> rays=f.give("RAYS");
   const Array<Array<int>> symmetry=f.give("RAYS_IMAGES");
   const Matrix<Rational> lin_space=f.give("LINEALITY_SPACE");
   const int lin_dim=lin_space.rows();
   const int amb_dim=rays.rows();


   //if the dimension of the fan is unknown it will be computed in res_dim and amb_dim
   //is an upper bound
   bool dim_given(true);
   int res_dim(dim);
   Array<int> max_cones_dims;
   if (dim<0) {
      dim_given=false;
      dim=amb_dim;
      max_cones_dims=Array<int>(cones.size());
   }

   Array<int> dims(dim,0);
   Array<int> orbit_dims(dim,0);

   typedef hash_set<Set<int>> Hashset;
   Array<Hashset> all_cones(dim);
   const int n_syms=symmetry[0].size();
   Array<std::vector<Set<int>>> cone_orbits(dim); // all cones up to symmetry
   Array<std::vector<int>> cone_orbit_sizes(dim); // the sizes of all arbits of cones

   int c_i = 0;
   for (const auto& cone : cones) {
      const int n_rays=cone.size();
      const Array<int> cone_A(n_rays, entire(cone));
      perl::Object c("Cone");
      c.take("RAYS") << rays.minor(cone, All);
      c.take("LINEALITY_SPACE") << lin_space;
      const Lattice<BasicDecoration, Sequential> hd = c.give("HASSE_DIAGRAM");
      const int c_dim(hd.rank());
      if (!dim_given) {
         res_dim = std::max(res_dim, c_dim);
         max_cones_dims[c_i++] = c_dim + lin_dim;
      }
      for (int l = 0; l < c_dim; ++l) {
         for (const auto fn : hd.nodes_of_rank(l+1)) {
            const Set<int>& face_old = hd.face(fn);

            Set<int> face;
            for (const auto& k : face_old) {
               face.insert(cone_A[k]);
            }

            if (all_cones[l].find(face) == all_cones[l].end()) {
               // if the face does not exist so far, we create its whole orbit
               int orbit_size = 0;
               for (int j = 0; j < n_syms; ++j) {
                  Set<int> new_cone;
                  for (const auto& k : face) {
                     new_cone.insert(symmetry[k][j]);
                  }
                  if (all_cones[l].insert(new_cone).second) {
                     ++orbit_size;
                     ++dims[l];
                  }
               }
               cone_orbits[l].push_back(face);
               cone_orbit_sizes[l].push_back(orbit_size);
               ++orbit_dims[l];
            }
         }
      }
   }

   if (!dim_given) {
      f.take("COMBINATORIAL_DIM") << res_dim;
      f.take("MAXIMAL_CONES_REPS_DIMS") << max_cones_dims;
      dims.resize(res_dim);
      orbit_dims.resize(res_dim);
      cone_orbits.resize(res_dim);
      cone_orbit_sizes.resize(res_dim);
   }
   f.take("F_VECTOR") << dims;
   f.take("ORBITS_F_VECTOR") << orbit_dims;
   f.take("CONES_REPS") << cone_orbits;
   f.take("CONES_ORBIT_SIZES") << cone_orbit_sizes;
}

Function4perl(&all_cones_symmetry, "all_cones_symmetry(PolyhedralFan; $=-1)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
