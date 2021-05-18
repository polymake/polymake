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
#include "polymake/IncidenceMatrix.h"
#include "polymake/FacetList.h"

namespace polymake { namespace polytope {

namespace {

template <typename IMatrix>
FacetList faces_below(const FacetList& F, const GenericIncidenceMatrix<IMatrix>& VIF)
{
   FacetList F_below(F.cols());
   for (auto face=entire(F); !face.at_end(); ++face) {
      // intersecting k-faces with facets
      for (auto facet = entire(rows(VIF)); !facet.at_end(); ++facet) {
         Set<Int> face_below = (*face) * (*facet);
         if (face_below.size() &&
             face_below.size() < face->size() )  // only real subsets of the k-face are canditates for faces below
            F_below.replaceMax(face_below);
      }
   }
   return F_below;
}

template <typename Layer>
void print_layer(pm::PlainPrinter<>& os, const Layer& L)
{
   os << "{";
   for (auto l=entire(L); ;) {
      os << *l;
      if ((++l).at_end()) break;
      os << ' ';
   }
   os << "}\n";
}

template <typename IMatrix, typename Enum>
void print_lattice(pm::PlainPrinter<>& os, const GenericIncidenceMatrix<IMatrix>& VIF, Enum dim)
{
   FacetList F(VIF.cols(), entire(rows(VIF)));
   do {
      os << "[ " << *dim << " : " << F.size() << " ]\n";
      print_layer(os, lex_ordered(F));
      os << endl;
      F=faces_below(F,VIF);
      ++dim;
   } while (!F.empty());
}

} // end unnamed namespace

void print_face_lattice(const IncidenceMatrix<>& VIF, bool dual)
{
   if (!dual) {
      cout << "FACE_LATTICE\n\n";
      print_lattice(cout, VIF, sequence(0,0).rbegin());
   } 
   else {
      cout << "DUAL_FACE_LATTICE\n\n";
      print_lattice(cout, T(VIF), sequence(0,0).begin());
   }
}

UserFunction4perl("# @category Geometry"
                  "# Write the face lattice of a vertex-facet incidence matrix //VIF// to stdout."
                  "# If //dual// is set true the face lattice of the dual is printed."
                  "# @param IncidenceMatrix VIF"
                  "# @param Bool dual"
                  "# @example To get a nice representation of the squares face lattice, do this:"
                  "# > print_face_lattice(cube(2)->VERTICES_IN_FACETS);"
                  "# | FACE_LATTICE"
                  "# | "
                  "# | [ -1 : 4 ]"
                  "# | {{0 1} {0 2} {1 3} {2 3}}"
                  "# | "
                  "# | [ -2 : 4 ]"
                  "# | {{0} {1} {2} {3}}",
                  &print_face_lattice, "print_face_lattice(IncidenceMatrix;$=0)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
