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
#include "polymake/vector"
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/common/labels.h"

namespace polymake { namespace polytope {

template <typename Scalar>
BigObject subcone(BigObject c_in, const Set<Int>& selection, OptionSet options)
{
   const Matrix<Scalar> V=c_in.give("RAYS");
   const Int n_rays = V.rows();
   const Int n_rays_out = selection.size();

   if (n_rays_out && (selection.front() < 0 || selection.back() >= n_rays))
      throw std::runtime_error("subcone: not a proper ray selection");

   const Matrix<Scalar> L = c_in.give("LINEALITY_SPACE");
   const Int adim = c_in.give("CONE_AMBIENT_DIM"); // don't look at V.cols() because of trivial cone

   BigObject c_out("Cone", mlist<Scalar>());
   c_out.set_description() << "subcone of " << c_in.name() << endl;

   c_out.take("RAYS") << V.minor(selection,All);
   c_out.take("LINEALITY_SPACE") << L;
   c_out.take("CONE_AMBIENT_DIM") << adim;

   if (!options["no_labels"]) {
      const std::vector<std::string> labels = common::read_labels(c_in, "RAY_LABELS", n_rays);
      c_out.take("RAY_LABELS") << select(labels, selection);
   }

   return c_out;
}

UserFunctionTemplate4perl("# @category  Producing a cone"
                          "# Make a subcone from a cone."
                          "# @param Cone C the input cone"
                          "# @option Bool no_labels Do not create [[RAY_LABELS]]. default: 0"
                          "# @return Cone",
                          "subcone<Scalar>(Cone<Scalar>, Set, { no_labels => 0})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
