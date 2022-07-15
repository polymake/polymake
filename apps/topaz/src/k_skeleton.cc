/* Copyright (c) 1997-2022
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
#include "polymake/topaz/complex_tools.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"

namespace polymake { namespace topaz {
  
namespace {

void combinatorial_k_skeleton_impl(BigObject p_in, BigObject& p_out, const Int k, OptionSet options)
{
   const Array<Set<Int>> C = p_in.give("FACETS");
   const PowerSet<Int> Skeleton = k_skeleton(C,k);
   
   p_out.set_description() << k << "-skeleton of " << p_in.name() << endl;
   p_out.take("FACETS") << Skeleton;

   if (!options["no_labels"]) {
      const Array<std::string> L = p_in.give("VERTEX_LABELS");
      p_out.take("VERTEX_LABELS") << L;
   }
}


BigObject combinatorial_k_skeleton(BigObject p_in, const Int k, OptionSet options)
{
   BigObject p_out("SimplicialComplex");
   combinatorial_k_skeleton_impl(p_in, p_out, k, options);
   return p_out;
}

template <typename Scalar>
BigObject k_skeleton(BigObject p_in, const Int k, OptionSet options)
{
   BigObject p_out("GeometricSimplicialComplex", mlist<Scalar>());
   combinatorial_k_skeleton_impl(p_in, p_out, k, options);
   
   Matrix<Scalar> GR;
   p_in.give("COORDINATES") >> GR;
   p_out.take("COORDINATES") << GR;

   return p_out;
}

} // end anonymous namespace

UserFunction4perl("# @category Producing a new simplicial complex from others\n"
                  "# Produce the //k//-skeleton.\n"
                  "# @param SimplicialComplex complex"
                  "# @param Int k"
                  "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                  "# @return SimplicialComplex"
                  "# @example The 2-skeleton of the 3-simplex is its boundary, a 2-sphere:"
                  "# > print isomorphic(k_skeleton(simplex(3),2), simplex(3)->BOUNDARY);"
                  "# | true",
                  &combinatorial_k_skeleton, "k_skeleton(SimplicialComplex $ { no_labels=>0 })");

UserFunctionTemplate4perl("# @category Producing a new simplicial complex from others\n"
                          "# Produce the //k//-skeleton.\n"
                          "# @param GeometricSimplicialComplex complex"
                          "# @param Int k"
                          "# @option Bool no_labels Do not create [[VERTEX_LABELS]]. default: 0"
                          "# @return GeometricSimplicialComplex"
                          "# @example The 2-skeleton of the 3-ball is its boundary, a 2-sphere:"
                          "# > print isomorphic(k_skeleton(ball(3),2), ball(3)->BOUNDARY);"
                          "# | true",
                          "k_skeleton<Scalar>(GeometricSimplicialComplex<Scalar> $ { no_labels=>0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
