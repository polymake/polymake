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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/matroid/modular_cut.h"

namespace polymake { namespace matroid {

   bool is_modular_cut(const perl::Object M, const Array<Set<int> >& modular_cut, bool verbose)
   {
      perl::Object LF_obj = M.give("LATTICE_OF_FLATS");
      const Lattice<BasicDecoration, Sequential> LF(LF_obj);
      return is_modular_cut_impl(modular_cut, LF, verbose);
   }

   perl::Object lex_extension(const perl::Object N, const Array<Set<int> >& modular_cut, perl::OptionSet options)
   {
      const bool check_modular_cut = options["check_modular_cut"], verbose = options["verbose"];
      perl::Object LF_obj = N.give("LATTICE_OF_FLATS");
      const Lattice<BasicDecoration, Sequential> LF(LF_obj);

      if (check_modular_cut && !is_modular_cut_impl(modular_cut, LF, verbose)) {
         throw std::runtime_error("The given set is not a modular cut in the lattice of flats of the input matroid.");
      }

      // prepare data structures for lattice of flats
      Map<Set<int>, int> rank_of, index_of;
      rank_of[Set<int>()] = -1;
      for (int i=0; i<=LF.rank(); ++i) {
         for (const auto f_index : LF.nodes_of_rank(i)) {
            rank_of[LF.face(f_index)] = i;
            index_of[LF.face(f_index)] = f_index;
         }
      }

      Set<Set<int>> modular_cut_set;
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

      const int n = N.give("N_ELEMENTS");
      const int r = N.give("RANK");

      Set<Set<int>> matroid_hyperplanes_N; // make it a Set to output them in canonical lex order
      Set<Set<int>> collar;
      for (auto mit = entire(modular_cut); !mit.at_end(); ++mit) {
         if (rank_of[*mit] == r-1)
            matroid_hyperplanes_N += *mit + scalar2set(n);
         for (Graph<Directed>::in_adjacent_node_list::const_iterator covered_flats_it = LF.in_adjacent_nodes(index_of[*mit]).begin(); !covered_flats_it.at_end(); ++covered_flats_it)
            collar += LF.face(*covered_flats_it);
      }

      for (auto cit = entire(collar); !cit.at_end(); ++cit) {
         if (rank_of[*cit] == r-1 && !modular_cut_set.contains(*cit))
            matroid_hyperplanes_N += *cit;
      }

      // Now calculate F_2 union F_3 and leave it in collar
      collar += modular_cut_set;

      for (int i=0; i<=LF.rank(); ++i) {
         for (const auto f_index : LF.nodes_of_rank(i)) {
            const Set<int>& f = LF.face(f_index);
            if (!collar.contains(f)) {
               if (rank_of[f] == r-1)
                  matroid_hyperplanes_N += f;
               if (rank_of[f] == r-2)
                  matroid_hyperplanes_N += f + scalar2set(n);
            }
         }
      }

      perl::Object E("Matroid");
      E.take("N_ELEMENTS") << n+1;
      E.take("RANK") << r;
      E.take("MATROID_HYPERPLANES") << Array<Set<int> >(matroid_hyperplanes_N.size(), entire(matroid_hyperplanes_N));

      return E;
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
