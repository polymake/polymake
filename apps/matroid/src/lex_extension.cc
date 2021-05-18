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
#include "polymake/Set.h"
#include "polymake/matroid/modular_cut.h"

namespace polymake { namespace matroid {

using namespace graph;
using namespace graph::lattice;

bool is_modular_cut(const BigObject M, const Array<Set<Int> >& modular_cut, bool verbose)
{
   BigObject LF_obj = M.give("LATTICE_OF_FLATS");
   const Lattice<BasicDecoration, Sequential> LF(LF_obj);
   return is_modular_cut_impl(modular_cut, LF, verbose);
}

BigObject lex_extension(const BigObject N, const Array<Set<Int>>& modular_cut, OptionSet options)
{
   const bool check_modular_cut = options["check_modular_cut"], verbose = options["verbose"];
   BigObject LF_obj = N.give("LATTICE_OF_FLATS");
   const Lattice<BasicDecoration, Sequential> LF(LF_obj);

   if (check_modular_cut && !is_modular_cut_impl(modular_cut, LF, verbose)) {
      throw std::runtime_error("The given set is not a modular cut in the lattice of flats of the input matroid.");
   }

   // prepare data structures for lattice of flats
   Map<Set<Int>, Int> rank_of, index_of;
   rank_of[Set<Int>()] = -1;
   for (Int i = 0; i <= LF.rank(); ++i) {
      for (const auto f_index : LF.nodes_of_rank(i)) {
         rank_of[LF.face(f_index)] = i;
         index_of[LF.face(f_index)] = f_index;
      }
   }

   Set<Set<Int>> modular_cut_set;
   for (auto mit = entire(modular_cut); !mit.at_end(); ++mit)
      modular_cut_set += *mit;

   /*
     Let M = N cup e be the lexicographic extension determined by the modular_cut.
     The flats LF of N partition into three sets, as follows:
     (we keep the notation from the paper cited below)

     (3) F_3 = modular_cut.
     If F in F_3 then  F notin flats(M)  and  F cup e in flats(M).
     (i.e., we added  e  to the flat.)

     (2) F_2 = collar of modular_cut.
     If  F in F_2  then  F in flats(M)  and  F cup e notin flats(M).
     Here the collar of modular_cut is the set of elements of LF covered by a member of F_3.

     (1) F_1 = LF - F_2 - F_3.
     If  F in F_1  then  F in flats(M)  and  F cup e in flats(M).

     See Dillon Mayhew and Gordon F. Royle, Matroids with Nine Elements,
     www.arxiv.org/pdf/math/0702316
   */

   // We only compute the new flats of maximal rank (the "matroid hyperplanes")

   const Int n = N.give("N_ELEMENTS");
   const Int r = N.give("RANK");

   Set<Set<Int>> matroid_hyperplanes_N; // make it a Set to output them in canonical lex order
   Set<Set<Int>> collar;
   for (auto mit = entire(modular_cut); !mit.at_end(); ++mit) {
      if (rank_of[*mit] == r-1)
         matroid_hyperplanes_N += *mit + scalar2set(n);
      for (auto covered_flats_it = LF.in_adjacent_nodes(index_of[*mit]).begin(); !covered_flats_it.at_end(); ++covered_flats_it)
         collar += LF.face(*covered_flats_it);
   }

   for (auto cit = entire(collar); !cit.at_end(); ++cit) {
      if (rank_of[*cit] == r-1 && !modular_cut_set.contains(*cit))
         matroid_hyperplanes_N += *cit;
   }

   // Now calculate F_2 union F_3 and leave it in collar
   collar += modular_cut_set;

   for (Int i = 0; i <= LF.rank(); ++i) {
      for (const auto f_index : LF.nodes_of_rank(i)) {
         const Set<Int>& f = LF.face(f_index);
         if (!collar.contains(f)) {
            if (rank_of[f] == r-1)
               matroid_hyperplanes_N += f;
            if (rank_of[f] == r-2)
               matroid_hyperplanes_N += f + n;
         }
      }
   }

   return BigObject("Matroid",
                    "N_ELEMENTS", n+1,
                    "RANK", r,
                    "MATROID_HYPERPLANES", Array<Set<Int>>(matroid_hyperplanes_N.size(), entire(matroid_hyperplanes_N)));
}

UserFunction4perl("# @category Other"
                  "# Check if a subset of the lattice of flats of a matroid is a modular cut"
                  "# @param Matroid M the matroid"
                  "# @param Array<Set> C a list of flats to check if they form a modular cut in M"
                  "# @option Bool verbose print diagnostic message in case C is not a modular cut; default is true"
                  "# @return Bool",
                  &is_modular_cut,
                  "is_modular_cut(Matroid Array<Set> { verbose => 1 })");

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# Calculate the lexicographic extension of a matroid in a modular cut"
                  "# @param Matroid M the original matroid to be extended"
                  "# @param Array<Set> C a list of flats that form a modular cut in M"
                  "# @option Bool check_modular_cut whether to check if C in fact is a modular cut; default is true"
                  "# @option Bool verbose print diagnostic message in case C is not a modular cut; default is true"
                  "# @return Matroid",
                  &lex_extension,
                  "lex_extension(Matroid Array<Set> { check_modular_cut => 1, verbose => 1 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
