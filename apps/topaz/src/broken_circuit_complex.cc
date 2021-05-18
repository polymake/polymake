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
#include "polymake/Array.h"
#include "polymake/Bitset.h"

namespace polymake { namespace topaz {

BigObject broken_circuit_complex(BigObject mat, Array<Int> ord = Array<Int>())
{
   const Array<Set<Int>>& circuits = mat.give("CIRCUITS");
   const Array<Set<Int>>& bases = mat.give("BASES");
   const Int n_elements = mat.give("N_ELEMENTS");

   // default ordering:
   if (ord.size() == 0)
      ord = Array<Int>(range(0, n_elements-1));

   // compute broken circuits:
   Set<Set<Int>> broken_circuits;
   for (auto c = entire(circuits); !c.at_end(); ++c) {
      Int max = 0;
      for (auto e = entire(*c); !e.at_end(); ++e)
         max = (ord[*e] > ord[max]) ? *e : max;
      broken_circuits += (*c - max);
   }

   // find index of maximal element of ground set:
   Int max = 0;
   for (Int i = 0; i < n_elements; ++i)
      max = (ord[i] > ord[max]) ? i : max;

   // build facets of broken circuit complex:
   Set<Set<Int>> facets;
   for (auto b = entire(bases); !b.at_end(); ++b) { // facets must be bases
      if (!(*b).contains(max)) continue; // facet can't contain maximal element
      bool flag = true;
      for (auto bc = entire(broken_circuits); !bc.at_end(); ++bc) {
         Int includes = incl(*b, *bc);
         if (includes == 1 || includes == 0) { // facet can't contain broken circuit
            flag = false;
            break;
         }
      }
      if (flag) facets += *b;
   }

   BigObject bc_complex("topaz::SimplicialComplex");
   bc_complex.take("FACETS") << as_array(facets);
   return bc_complex;
}


InsertEmbeddedRule("REQUIRE_APPLICATION matroid\n\n");

UserFunction4perl("# @category Producing a simplicial complex from other objects\n"
                  "# Compute the broken circuit complex of a matroid."
                  "# Given an ordering on the ground set of the matroid, a broken circuit is simply C\{c},"
                  "# where C is a circuit and c its maximal element under this ordering."
                  "# The broken circuit complex of a matroid is then the abstract simplicial complex"
                  "# generated by those subsets of its ground set, which do not contain a broken circuit."
                  "# Every such set must be a basis of the matroid."
                  "# @param matroid::Matroid M the input matroid"
                  "# @param Array<Int> O representing an ordering function on the base set of M (its semantics are that i > j iff O[i]>O[j])"
                  "# @return SimplicialComplex"
                  "# @example A matroid with 3 bases {0,1}, {0,2}, and {1,2}."
                  "# The only circuit is {0,1,2}, hence the only broken circuit (with the standard ordering) is {0,1}."
                  "# > $m = new matroid::Matroid(VECTORS=>[[1,0],[0,1],[1,1]]);"
                  "# > print broken_circuit_complex($m)->FACETS;"
                  "# | {0 2}"
                  "# | {1 2}"
                  "# @example The same matroid, but with a different ordering on its ground set."
                  "# > $m = new matroid::Matroid(VECTORS=>[[1,0],[0,1],[1,1]]);"
                  "# > $ord = new Array<Int>(0,2,1);"
                  "# > print broken_circuit_complex($m, $ord)->FACETS;"
                  "# | {0 1}"
                  "# | {1 2}",
                  &broken_circuit_complex,"broken_circuit_complex(matroid::Matroid; Array<Int> = new Array<Int>())");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
