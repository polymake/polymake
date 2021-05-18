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

namespace polymake { namespace matroid {

BigObject principal_truncation(BigObject matroid, const Set<Int>& flat)
{
  Set<Set<Int>> result_bases;
  const Array<Set<Int>> bases = matroid.give("BASES");
  Int n = matroid.give("N_ELEMENTS");
  Int r = matroid.give("RANK");
  if (r == 0) return matroid;

  for (auto& b : bases) {
    for (auto& f : flat) {
      if (b.contains(f))
        result_bases += b-f;
    }		
  }

  return BigObject("Matroid",
                   "N_ELEMENTS", n,
                   "BASES", Array<Set<Int>>(result_bases));
}

BigObject truncation(BigObject matroid)
{
  Int n = matroid.give("N_ELEMENTS");
  return principal_truncation( matroid, sequence(0, n));
}

BigObject principal_extension(BigObject matroid, const Set<Int>& flat)
{
  const	Array<Set<Int>> bases = matroid.give("BASES");
  Int n = matroid.give("N_ELEMENTS");

  Set<Set<Int>> result_bases(bases);

  for (auto &b : bases) {
    for (auto &f : flat) {
      if (b.contains(f)) {
        result_bases += b-f+n;
      }
    }
  }		

  return BigObject("Matroid",
                   "N_ELEMENTS", n+1,
                   "BASES", Array<Set<Int>>(result_bases));
}

BigObject free_extension(BigObject matroid)
{
  const Int n = matroid.give("N_ELEMENTS");
  return principal_extension(matroid, sequence(0,n));
}

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# Computes the principal truncation of a matroid with respect to a flat."
                  "# @param Matroid M A matroid"
                  "# @param Set<Int> F A set F, which is a flat of M"
                  "# @return Matroid The truncation T_F(M), i.e. the matroid whose bases"
                  "# are all sets B-p, where B is a basis of M and p is in F and B.",
                  &principal_truncation, "principal_truncation( Matroid, Set<Int>)");

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# Computes the truncation of M, i.e. the [[principal_truncation]], with F"
                  "# the full ground set"
                  "# @param Matroid M A matroid"
                  "# @return Matroid The truncation T(M)",
                  &truncation, "truncation(Matroid)");

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# Computes the principal extension of a matroid with respect to a flat."
                  "# @param Matroid M A matroid"
                  "# @param Set<Int> F A set F, which is a flat of M"
                  "# @return Matroid The principal extension M +_F n. If M is a matroid on "
                  "# 0 .. n-1, then the principal extension has ground set 0 .. n. Its bases are"
                  "# the bases of M, plus the sets B-q+n, where B is a basis of M and q is in B and F.",
                  &principal_extension, "principal_extension(Matroid, Set<Int>)");

UserFunction4perl("# @category Producing a matroid from matroids"
                  "# Computes the free extension of a matroid, i.e. the [[principal_extension]], with F"
                  "# the full ground set."
                  "# @param Matroid M A matroid"
                  "# @return Matroid The free extension of M",
                  &free_extension, "free_extension(Matroid)");

InsertEmbeddedRule("# @category Producing a matroid from matroids"
                   "# Computes the principal lift of a matroid with respect to a flat F"
                   "# @param Matroid M A matroid"
                   "# @param Set<Int> F A set F, which is a flat of M"
                   "# @return Matroid The principal lift L_F(M) = T_F(M*)*, where T_F is the"
                   "# [[principal_truncation]] and * denotes the dualizing operator\n"
                   "user_function principal_lift( Matroid, Set<Int> ) {\n"
                   "	my ($M, $F) = @_;\n"
                   "	if($M->RANK == $M->N_ELEMENTS) { return $M;}\n"
                   "	return dual(principal_truncation($M->DUAL,$F));\n"
                   "}\n");

InsertEmbeddedRule("# @category Producing a matroid from matroids"
                   "# Computes the Higgs lift of a matroid, i.e. the [[principal_lift]] with respect to the"
                   "# full ground set."
                   "# @param Matroid M A matroid."
                   "# @return Matroid The Higgs lift L_E(M)\n"
                   "user_function higgs_lift(Matroid) {\n"
                   "	my $M = shift;\n"
                   "	if($M->RANK == $M->N_ELEMENTS) { return $M;}\n"
                   "	return dual(truncation($M->DUAL));\n"
                   "}\n");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
