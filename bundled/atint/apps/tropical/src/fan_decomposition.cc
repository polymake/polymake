/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	---
	Copyright (c) 2016-2020
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Compute the local fans of a tropical cycle.
	*/

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/separated_data.h"

namespace polymake { namespace tropical {

// Documentation see perl wrapper
template <typename Addition>
ListReturn fan_decomposition(BigObject cycle)
{
  // Extract values
  Matrix<Rational> rays = cycle.give("VERTICES");
  IncidenceMatrix<> cones = cycle.give("MAXIMAL_POLYTOPES");
  IncidenceMatrix<> verticesInCones = T(cones);
  Vector<Integer> weights = cycle.give("WEIGHTS");
  Matrix<Rational> lineality = cycle.give("LINEALITY_SPACE");

  IncidenceMatrix<> local_restriction;
  if (cycle.exists("LOCAL_RESTRICTION")) {
    cycle.give("LOCAL_RESTRICTION") >> local_restriction;
  }

  Set<Int> nonfar = far_and_nonfar_vertices(rays).second;

  ListReturn result;
  for (auto nf = entire(nonfar); !nf.at_end(); ++nf) {
    if (local_restriction.rows() > 0) {
      if (!is_coneset_compatible(scalar2set(*nf), local_restriction))
        continue;
    }

    Set<Int> conesAtVertex = verticesInCones.row(*nf);
    Set<Int> usedRays = accumulate(rows( cones.minor(conesAtVertex,All)), operations::add());

    Matrix<Rational> fanRays(rays);
    // Replace other nonfar vertices by difference
    Set<Int> othernonfar = (nonfar * usedRays) - *nf;
    for (auto onf = entire(othernonfar); !onf.at_end(); ++onf) {
      fanRays.row(*onf) = fanRays.row(*onf) - fanRays.row(*nf);
    }
    fanRays.row(*nf) = unit_vector<Rational>(fanRays.cols(),0);
    fanRays = fanRays.minor(usedRays,All);

    BigObject fanCycle("Cycle", mlist<Addition>());
    fanCycle.take("PROJECTIVE_VERTICES") << fanRays; 
    fanCycle.take("MAXIMAL_POLYTOPES") << cones.minor(conesAtVertex,usedRays);
    fanCycle.take("WEIGHTS") << weights.slice(conesAtVertex);
    fanCycle.take("LINEALITY_SPACE") << lineality;
    result << fanCycle;
  }
  return result;
}

UserFunctionTemplate4perl("# @category Basic polyhedral operations"
                          "# This computes the local fans at all (nonfar) vertices of a tropical cycle"
                          "# @param Cycle<Addition> C A tropical cycle"
                          "# @return Cycle<Addition> A list of local cycles",
                          "fan_decomposition<Addition>(Cycle<Addition>)");

} }
