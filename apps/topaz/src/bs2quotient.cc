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
#include "polymake/Map.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/group/permlib.h"
#include "polymake/list"
#include "polymake/topaz/barycentric_subdivision.h"
#include <sstream>

namespace polymake { namespace topaz {

using graph::Lattice;
using graph::lattice::Sequential;
    
      
namespace {
   
void convert_labels(const Array<std::string>& string_labels, Array<Set<Set<Int>>>& labels_as_set)
{
   auto lsit = entire(labels_as_set);
   for (auto lit = entire(string_labels); !lit.at_end(); ++lit) {
      std::istringstream is(*lit);
      is.ignore(1); // "{"
      Set<Set<Int>> labels;
      while (is.good() && is.peek() != '}') {
         is.ignore(1); // '{'
         while (is.good() && is.peek() != '}') {
            Set<Int> new_set;
            while (is.good() && is.peek() != '}') {
               if (is.peek() == '{') is.ignore(1); 
               Int i;
               is >> i;
               new_set += i;
               if (is.peek() == ',' || is.peek() == ' ') is.ignore(1); 
            }
            labels += new_set;
            if (is.peek() == '}') is.ignore(1); 
            if (is.peek() == ',' || is.peek() == ' ') is.ignore(1); 
         }
      }
      *lsit++ = labels;
   }
}

template<typename GroupOrOrbits>   
void identify_labels(const GroupOrOrbits&,
                     const IncidenceMatrix<>&,
                     Array<Set<Set<Int>>>&);
   
template<>
void identify_labels(const group::PermlibGroup& identification_group,
                     const IncidenceMatrix<>& VIF,
                     Array<Set<Set<Int>>>& labels_as_set)
{
   for (auto lit = entire(labels_as_set); !lit.at_end(); ++lit)
      if (on_boundary(*lit, VIF)) {
         *lit = *(identification_group.orbit(*lit).begin());
      }
}

template<>   
void identify_labels(const Array<Set<Set<Set<Int>>>>& face_orbits,
                     const IncidenceMatrix<>& VIF,
                     Array<Set<Set<Int>>>& labels_as_set)
{
   Map<Set<Int>, Set<Int>> face_rep; // maps each face to its representative
   for (auto bydim_it = entire(face_orbits); !bydim_it.at_end(); ++bydim_it) { // *: Set<Set<Set>> = set of orbits of faces
      for (auto orbit_it = entire(*bydim_it); !orbit_it.at_end(); ++orbit_it) {// *: Set<Set> = one orbit of faces
         auto face_it = entire(*orbit_it);
         Set<Int> this_rep = *face_it;
         for (; !face_it.at_end(); ++face_it) {
            face_rep[*face_it] = this_rep;
         }
      }
   }
   for (auto lit = entire(labels_as_set); !lit.at_end(); ++lit)
      if (on_boundary(*lit, VIF)) {
         Set<Set<Int>> flag;
         for (auto fit = entire(*lit); !fit.at_end(); ++fit)
            flag += face_rep[*fit];
         *lit = flag;
      }
}

void
identify_labels_by_equivalence(const Array<Set<Set<Set<Int>>>>& face_orbits,
                               const IncidenceMatrix<>& VIF,
                               Array<Set<Set<Int>>>& labels_as_set)
{
   cerr << "identify_labels_by_equivalence. VIF=\n" << VIF << endl;
   Map<Set<Int>, Set<Int>> face_rep; // maps each faces to its representative
   for (auto bydim_it = entire(face_orbits); !bydim_it.at_end(); ++bydim_it) { // *: Set<Set<Set>> = set of orbits of faces
      for (auto orbit_it = entire(*bydim_it); !orbit_it.at_end(); ++orbit_it) {// *: Set<Set> = one orbit of faces
         auto face_it = entire(*orbit_it);
         Set<Int> this_rep = *face_it;
         for (; !face_it.at_end(); ++face_it) {
            face_rep[*face_it] = this_rep;
         }
      }
   }
   for (auto lit = entire(labels_as_set); !lit.at_end(); ++lit) {
      Set<Set<Int>> flag;
      for (auto fit = entire(*lit); !fit.at_end(); ++fit)
         flag += face_rep[*fit];
      cerr << "ilbe: " << *lit << " --> " << flag << endl;
      if (on_boundary(*lit, VIF)) {
         *lit = flag;
      } else {
         cerr << "not on boundary: " << *lit << endl;
      }
   }
}

template<typename ClassesOrGroup>   
BigObject
bs2quotient_impl(BigObject p,
                 const Array<Set<Int>>& facets,
                 Array<Set<Set<Int>>>& labels_as_set,
                 const ClassesOrGroup& classes_or_group)
{
   const IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
   const Int d = labels_as_set[0].size()-1;
   
   // each vertex of the 2nd barycentric subdivision corresponds to a flag in the original complex
   identify_labels(classes_or_group, VIF, labels_as_set);

   std::vector<std::string> identified_labels;
   Map<Set<Set<Int>>, Int> index_of;
   Int max_index(0);
   std::ostringstream os;
   
   for (const auto& lset : labels_as_set) {
      if (!index_of.contains(lset)) {
         index_of[lset] = max_index++;
         wrap(os) << lset;
         identified_labels.push_back(os.str());
         os.str("");
      }
   }
   
   Set<Set<Int>> identified_facets;
   for (const auto& f : facets) {
      Set<Int> new_facet;
      for (const auto& s : f)
         new_facet += index_of[labels_as_set[s]];
      identified_facets += new_facet;
   }
   
   return BigObject("topaz::SimplicialComplex",
                    "FACETS", identified_facets,
                    "VERTEX_LABELS", identified_labels,
                    "PURE", true,
                    "DIM", d);
}

} // end anonymous namespace

      
BigObject bs2quotient_by_group(BigObject p)
{
   const Array<Array<Int>> generators = p.give("QUOTIENT_SPACE.IDENTIFICATION_ACTION.GENERATORS");
   const Lattice<BasicDecoration, Sequential> HD = p.give("HASSE_DIAGRAM");
   
   Array<Set<Int>> facets;
   Array<Set<Set<Int>>> labels_as_set;
   std::tie(facets, labels_as_set) = second_barycentric_subdivision(HD);
   const group::PermlibGroup identification_group(generators);
   
   return bs2quotient_impl(p, facets, labels_as_set, identification_group);
}


BigObject bs2quotient_by_equivalence(BigObject p)
{
   const Array<Set<Set<Set<Int>>>> face_classes = p.give("QUOTIENT_SPACE.FACE_CLASSES");
   const Lattice<BasicDecoration, Sequential> HD = p.give("HASSE_DIAGRAM");

   Array<Set<Int>> facets;
   Array<Set<Set<Int>>> labels_as_set;
   std::tie(facets, labels_as_set) = second_barycentric_subdivision(HD);

   return bs2quotient_impl(p, facets, labels_as_set, face_classes);
}

BigObject bs2quotient_by_equivalence_2(BigObject p, BigObject bs)
{
   const Array<Set<Set<Set<Int>>>> face_orbits = p.give("QUOTIENT_SPACE.FACE_ORBITS");
   const IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
   const Array<std::string> labels = bs.give("VERTEX_LABELS");
   const Int n = labels.size();
   const Array<Set<Int>> facets = bs.give("FACETS");
   if (!facets.size() || !facets[0].size()) throw std::runtime_error("Got no facets");

   // each vertex of the 2nd barycentric subdivision corresponds to a flag in the orginal complex
   Array<Set<Set<Int>>> labels_as_set(n);  // maps a vertex to a flag (as set of faces)
   convert_labels(labels, labels_as_set);
   identify_labels_by_equivalence(face_orbits, VIF, labels_as_set);
   // cerr << "labels: " << labels << endl << "labels_as_set: " << labels_as_set << endl;
   const Int d = labels_as_set[0].size()-1;

   std::vector<std::string> identified_labels;
   Map<Set<Set<Int>>, Int> index_of;
   Int index = 0;
   std::ostringstream os;
   for (const auto& lset : labels_as_set) {
      if (!index_of.contains(lset)) {
         index_of[lset] = index++;
         wrap(os) << lset;
         identified_labels.push_back(os.str());
         os.str("");
      }
   }

   Set<Set<Int>> identified_facets;
   for (const auto& f : facets) {
      Set<Int> new_facet;
      for (const auto& s : f)
         new_facet += index_of[labels_as_set[s]];
      identified_facets += new_facet;
   }

   return BigObject("topaz::SimplicialComplex",
                    "FACETS", identified_facets,
                    "VERTEX_LABELS", identified_labels,
                    "PURE", true,
                    "DIM", d);
}
      
std::pair<Array<Set<Int>>, Array<Set<Set<Int>>>>
second_barycentric_subdivision_from_HD(Lattice<BasicDecoration,Sequential> HD)
{
   return second_barycentric_subdivision(HD);
}

std::pair<Array<Set<Int>>, Array<Set<Set<Int>>>>
second_barycentric_subdivision_caller(BigObject p)
{
   if (p.isa("Polytope")) {
      const Lattice<BasicDecoration, Sequential> HD = p.give("HASSE_DIAGRAM");
      return second_barycentric_subdivision(HD);
   } else {
      const Lattice<BasicDecoration, Nonsequential> HD = p.give("HASSE_DIAGRAM");
      return second_barycentric_subdivision(HD);
   }
}
      
InsertEmbeddedRule("REQUIRE_APPLICATION polytope\n\n");

UserFunction4perl("# @category Producing a new simplicial complex from others"
                  "# Create a simplicial complex from a simplicial subdivision of a given complex"
                  "# by identifying vertices on the boundary of the second barycentric subdivision of the original complex"
                  "# according to a group that acts on vertices." 
                  "# @param polytope::Polytope P the underlying polytope"
                  "# @return SimplicialComplex",
                  &bs2quotient_by_group,
                  "bs2quotient_by_group(polytope::Polytope)");

UserFunction4perl("# @category Producing a new simplicial complex from others"
                  "# Create a simplicial complex from a given complex"
                  "# by identifying vertices on the boundary of the second barycentric subdivision of the original complex"
                  "# according to some equivalence relation on faces." 
                  "# @param polytope::Polytope P the underlying polytope"
                  "# @return SimplicialComplex",
                  &bs2quotient_by_equivalence,
                  "bs2quotient_by_equivalence(polytope::Polytope)");

UserFunction4perl("# @category Producing a new simplicial complex from others"
                  "# Create a simplicial complex from a simplicial subdivision of a given complex"
                  "# by identifying vertices on the boundary of the original complex according to some equivalence relation on faces." 
                  "# @param polytope::Polytope P the underlying polytope"
                  "# @param SimplicialComplex complex a sufficiently fine subdivision of P, for example the second barycentric subdivision"
                  "# @return SimplicialComplex",
                  &bs2quotient_by_equivalence_2,
                  "bs2quotient_by_equivalence(polytope::Polytope SimplicialComplex)");

UserFunction4perl("# @category Other"
                  "# Create the list of faces of the second barycentric subdivision"
                  "# @param Lattice L (for example, a HASSE_DIAGRAM)"
                  "# @return Pair<Array<Set>,Array<Set<Set>>>",
                  &second_barycentric_subdivision_from_HD,
                  "second_barycentric_subdivision(Lattice<BasicDecoration,Sequential>)");

UserFunction4perl("# @category Other"
                  "# Create the list of faces of the second barycentric subdivision"
                  "# @param polytope::Polytope P or SimplicialComplex S"
                  "# @return Pair<Array<Set>,Array<Set<Set>>>",
                  &second_barycentric_subdivision_caller,
                  "second_barycentric_subdivision($)");

      
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
