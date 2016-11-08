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
#include "polymake/Map.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/topaz/complex_tools.h"
#include "polymake/group/permlib.h"
#include "polymake/list"
#include <sstream>

namespace polymake { namespace topaz {

namespace {

void convert_labels(const Array<std::string>& string_labels, Array<Set<Set<int>>>& labels_as_set)
{
   Entire<Array<Set<Set<int>>>>::iterator lsit = entire(labels_as_set);
   for (Entire<Array<std::string>>::const_iterator lit = entire(string_labels); !lit.at_end(); ++lit) {
      std::istringstream is(*lit);
      is.ignore(1); // "{"
      Set<Set<int>> labels;
      while (is.good() && is.peek() != '}') {
         is.ignore(1); // '{'
         while (is.good() && is.peek() != '}') {
            Set<int> new_set;
            while (is.good() && is.peek() != '}') {
               if (is.peek() == '{') is.ignore(1); 
               int i;
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

bool on_boundary (const Set<Set<int>>& label, int d, const IncidenceMatrix<>& VIF)
{
   Set<int> face;
   for (Entire<Set<Set<int>>>::const_iterator lit = entire(label); !lit.at_end(); ++lit)
      face += *lit;
   for (Entire<Rows<IncidenceMatrix<>>>::const_iterator rit = entire(rows(VIF)); !rit.at_end(); ++rit) 
      if (!(face - *rit).size()) return true; // it's contained in the boundary
   return false;
}

void identify_labels(int d, const group::PermlibGroup& identification_group, const IncidenceMatrix<>& VIF, Array<Set<Set<int>>>& labels_as_set)
{
   for (Entire<Array<Set<Set<int>>>>::iterator lit = entire(labels_as_set); !lit.at_end(); ++lit)
      if (on_boundary(*lit, d, VIF)) 
         *lit = *(identification_group.orbit(*lit).begin());
}

} // end anonymous namespace

perl::Object bs2quotient(perl::Object p, perl::Object bs)
{
   const Array<Array<int>> generators = p.give("QUOTIENT_SPACE.IDENTIFICATION_ACTION.GENERATORS");
   const group::PermlibGroup identification_group(generators);
   const IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");
   const Array<std::string> labels = bs.give("VERTEX_LABELS");
   const int n = labels.size();
   const Array<Set<int>> facets = bs.give("FACETS");
   if (!facets.size() || !facets[0].size()) throw std::runtime_error("Got no facets");
   const int d = facets[0].size() - 1;
   
   Array<Set<Set<int>>> labels_as_set(n);
   convert_labels(labels, labels_as_set);
   identify_labels(d, identification_group, VIF, labels_as_set);

   std::vector<std::string> identified_labels;
   Map<Set<Set<int>>,int> index_of;
   int index(0);
   std::ostringstream os;
   for (const auto& lset : labels_as_set) {
      if (!index_of.contains(lset)) {
         index_of[lset] = index++;
         wrap(os) << lset;
         identified_labels.push_back(os.str());
         os.str("");
      }
   }

   Set<Set<int>> identified_facets;
   for (const auto& f : facets) {
      Set<int> new_facet;
      for (const auto& s : f)
         new_facet += index_of[labels_as_set[s]];
      identified_facets += new_facet;
   }

   perl::Object q("topaz::SimplicialComplex");
   q.take("FACETS") << identified_facets;
   q.take("VERTEX_LABELS") << identified_labels;
   q.take("PURE") << true;
   q.take("DIM") << d;
   return q;
}

InsertEmbeddedRule("REQUIRE_APPLICATION polytope\n\n");

UserFunction4perl("# @category Producing a new simplicial complex from others"
                  "# Create a simplicial complex from a simplicial subdivision of a given complex"
                  "# by identifying vertices on the boundary of the original complex according to a group that acts on vertices." 
                  "# @param polytope::Polytope P the underlying polytope"
                  "# @param SimplicialComplex complex a sufficiently fine subdivision of P, for example the second barycentric subdivision"
                  "# @return SimplicialComplex",
                  &bs2quotient,
                  "bs2quotient(polytope::Polytope SimplicialComplex)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
