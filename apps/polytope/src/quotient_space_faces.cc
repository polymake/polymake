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

#include "polymake/client.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/hash_set"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/polytope/quotient_space_tools.h"
#include <vector>
#include <list>
#include <sstream>

namespace polymake { namespace polytope {

typedef Set<Set<int>> FaceSet;

namespace {
   // For some reason, this function has to be explicitly specified.
   // Trying to use an instantiated template doesn't work in the line
   // face_orbit.orbit(face, gen_list, pm_set_action);
   // below.
   Set<int> pm_set_action(const permlib::Permutation& p, const Set<int>& s)
   {
      Set<int> simg;
      for (const auto& i : s)
         simg += p / i;
      return simg;
   }
}

Array<Set<FaceSet>> orbits(int d,
                           const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential>& HD,
                           const Array<Array<int>>& generators)
{
   using Perm = permlib::Permutation;
   std::list<Perm::ptr> gen_list;
   for (auto perm = entire(generators); !perm.at_end(); ++perm) {
      Perm::ptr gen(new permlib::Permutation((*perm).begin(),(*perm).end()));
      gen_list.push_back(gen);
   }

   Array<Set<FaceSet>> face_orbits_of_dim(d+1);
   for (int k=0; k<=d; ++k) {
      for (const auto f_index : HD.nodes_of_rank(k+1)) {
         const Set<int> face(HD.face(f_index));
         permlib::OrbitSet<Perm, Set<int>> face_orbit;
         face_orbit.orbit(face, gen_list, pm_set_action);
         face_orbits_of_dim[k] += FaceSet(face_orbit.begin(), face_orbit.end());
      }
   }
   return face_orbits_of_dim;
}

void quotient_space_faces(perl::Object p)
{
   const int
      d = p.give("COMBINATORIAL_DIM"),
      n_vert = p.give("N_VERTICES");

   perl::Object HD_obj = p.give("HASSE_DIAGRAM");
   const graph::Lattice<graph::lattice::BasicDecoration, graph::lattice::Sequential> HD(HD_obj);

   const Array<Array<int>> id_group_generators = p.give("QUOTIENT_SPACE.IDENTIFICATION_ACTION.GENERATORS");
   const group::PermlibGroup identification_group(id_group_generators);

   Array<FaceSet> cds(d+1);
   for (int k = 0; k <= d; ++k)
      for (const auto n : HD.nodes_of_rank(k+1))
         cds[k] += identification_group.lex_min_representative(HD.face(n));

   p.take("QUOTIENT_SPACE.FACES") << cds;
   const auto face_orbits_of_dim = orbits(d, HD, id_group_generators);
   p.take("QUOTIENT_SPACE.FACE_ORBITS") << face_orbits_of_dim;
   Set<FaceSet> face_orbits;
   for (int k = 0; k <= d; ++k)
      face_orbits += face_orbits_of_dim[k];

   
   Array<Array<int>> sym_group_generators;
   if (p.lookup("GROUP.VERTICES_ACTION.GENERATORS") >> sym_group_generators) {
      perl::Object sga("group::PermutationAction");
      sga.take("GENERATORS") << induced_symmetry_group_generators(n_vert, sym_group_generators, face_orbits_of_dim);

      perl::Object g("group::Group");
      g.take("PERMUTATION_ACTION") << sga;

      p.take("QUOTIENT_SPACE.SYMMETRY_GROUP") << g;
   }
}

Function4perl(&quotient_space_faces,"quotient_space_faces(Polytope)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
