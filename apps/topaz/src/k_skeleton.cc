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
#include "polymake/topaz/complex_tools.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"

namespace polymake { namespace topaz {
  
namespace {

void combinatorial_k_skeleton_impl(perl::Object p_in, perl::Object& p_out, const int k, perl::OptionSet options)
{
   const Array< Set<int> > C = p_in.give("FACETS");
   const PowerSet<int> Skeleton = k_skeleton(C,k);
   
   p_out.set_description() << k << "-skeleton of " << p_in.name() << endl;
   p_out.take("FACETS") << Skeleton;

   if (!options["nol"]) {
      const Array<std::string> L = p_in.give("VERTEX_LABELS");
      p_out.take("VERTEX_LABELS") << L;
   }
}


perl::Object combinatorial_k_skeleton(perl::Object p_in, const int k, perl::OptionSet options)
{
   perl::Object p_out("SimplicialComplex");
   combinatorial_k_skeleton_impl(p_in, p_out, k, options);
   return p_out;
}

template <typename Scalar>
perl::Object k_skeleton(perl::Object p_in, const int k, perl::OptionSet options)
{
   perl::Object p_out(perl::ObjectType::construct<Scalar>("topaz::GeometricSimplicialComplex"));
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
                  "# @param int k"
                  "# @option Bool vertex_labels whether to create [[VERTEX_LABELS]]"
                  "# @return SimplicialComplex",
                  &combinatorial_k_skeleton, "k_skeleton(SimplicialComplex $ { vertex_labels=>0 })");

UserFunctionTemplate4perl("# @category Producing a new simplicial complex from others\n"
                          "# Produce the //k//-skeleton.\n"
                          "# @param GeometricSimplicialComplex complex"
                          "# @param int k"
                          "# @option Bool vertex_labels whether to create [[VERTEX_LABELS]]"
                          "# @return GeometricSimplicialComplex",
                          "k_skeleton<Scalar>(GeometricSimplicialComplex<Scalar> $ { vertex_labels=>0 })");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
