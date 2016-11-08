/* Copyright (c) 1997-2015
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
#include "polymake/graph/HasseDiagram.h"
#include "polymake/polytope/sympol_interface.h"
#include "polymake/polytope/quotient_space_tools.h"
#include <vector>
#include <list>
#include <sstream>

namespace polymake { namespace polytope {

typedef Set<Set<int>> FaceSet;
typedef Array<FaceSet> RepArray;

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

Array<Set<Set<Set<int>>>> orbits(int d, 
                                 const graph::HasseDiagram& HD, 
                                 const Array<Array<int>>& generators)
{ 
   typedef permlib::Permutation PERM;
   std::list<PERM::ptr> gen_list;
   for(Entire< Array< Array <int>> >::const_iterator perm = entire(generators); !perm.at_end(); ++perm){
      PERM::ptr gen(new permlib::Permutation((*perm).begin(),(*perm).end()));
      gen_list.push_back(gen);
   }
   
   Array<Set<Set<Set<int>>>> face_orbits_of_dim(d+1);
   for (int k=0; k<=d; ++k)
      for (Entire<sequence>::const_iterator fit = entire(HD.node_range_of_dim(k)); !fit.at_end(); ++fit) {
         const Set<int> face(HD.face(*fit));
         permlib::OrbitSet<PERM, Set<int>> face_orbit;
         face_orbit.orbit(face, gen_list, pm_set_action);
         face_orbits_of_dim[k] += Set<Set<int>>(face_orbit.begin(), face_orbit.end());
      }
   return face_orbits_of_dim;
}

void quotient_space_faces(perl::Object p)
{
   const int 
      d = p.give("COMBINATORIAL_DIM"),
      n = p.give("N_VERTICES");
   const graph::HasseDiagram HD = p.give("HASSE_DIAGRAM");
   const Array<Array<int>> 
      sym_group_generators = p.give("GROUP.VERTICES_ACTION.GENERATORS"),
      id_group_generators = p.give("QUOTIENT_SPACE.IDENTIFICATION_ACTION.GENERATORS");
   const group::PermlibGroup identification_group(id_group_generators);

   RepArray cds(d+1);
   for (int k=0; k<=d; ++k) 
      for (Entire<sequence>::const_iterator it=entire(HD.node_range_of_dim(k)); !it.at_end(); ++it) 
         cds[k] += identification_group.lex_min_representative(HD.face(*it));
      
   p.take("QUOTIENT_SPACE.FACES") << cds;
   const auto face_orbits_of_dim = orbits(d, HD, id_group_generators);
   p.take("QUOTIENT_SPACE.FACE_ORBITS") << face_orbits_of_dim;
   Set<Set<Set<int>>> face_orbits;
   for (int k=0; k<=d; ++k)
      face_orbits += face_orbits_of_dim[k];

   perl::Object sga("group::PermutationAction");
   sga.take("GENERATORS") << induced_symmetry_group_generators(n, sym_group_generators, face_orbits_of_dim);

   perl::Object g("group::Group");
   g.take("PERMUTATION_ACTION") << sga;

   p.take("QUOTIENT_SPACE.SYMMETRY_GROUP") << g;
}

Function4perl(&quotient_space_faces,"quotient_space_faces(Polytope)");


} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
