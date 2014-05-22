/* Copyright (c) 1997-2014
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
#include "polymake/vector"
#include "polymake/Matrix.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope {

template <typename Scalar>
perl::Object subcone(perl::Object c_in, const Set<int> selection, perl::OptionSet options)
{
   const Matrix<Scalar> V=c_in.give("RAYS");
   const int n_rays=V.rows();
   const int n_rays_out=selection.size();

   if (n_rays_out && (selection.front() < 0 || selection.back() >= n_rays))
      throw std::runtime_error("subcone: not a proper ray selection");

   const Matrix<Scalar> L=c_in.give("LINEALITY_SPACE");
   const int adim=c_in.give("CONE_AMBIENT_DIM"); // don't look at V.cols() because of trivial cone

   perl::Object c_out(perl::ObjectType::construct<Scalar>("Cone"));
   c_out.set_description() << "subcone of " << c_in.name() << endl;

   c_out.take("RAYS") << V.minor(selection,All);
   c_out.take("LINEALITY_SPACE") << L;
   c_out.take("CONE_AMBIENT_DIM") << adim;

   if (options["relabel"]) {
      std::vector<std::string> labels(n_rays_out);
      read_labels(c_in, "RAY_LABELS", labels);
      c_out.take("RAY_LABELS") << select(labels, selection);
   }

   return c_out;
}

UserFunctionTemplate4perl("# @category  Producing a cone"
                          "# Make a subcone from a cone."
                          "# @param Cone C the input cone"
                          "# @option Bool relabel creates an additional section [[RAY_LABELS]];"
                          "# @return Cone",
                          "subcone<Scalar>(Cone<Scalar>, Set, { relabel => undef})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
