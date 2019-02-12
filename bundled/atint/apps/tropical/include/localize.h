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

   Contains functionality for computing divisors
   */

#ifndef POLYMAKE_ATINT_LOCALIZE_H
#define POLYMAKE_ATINT_LOCALIZE_H

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/tropical/refine.h"
#include "polymake/tropical/misc_tools.h"
#include "polymake/tropical/separated_data.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/thomog.h"

namespace polymake { namespace tropical {

// Documentation see perl wrapper
template <typename Addition>
perl::Object local_restrict(perl::Object complex, const IncidenceMatrix<>& cones)
{
  // Extract values
  IncidenceMatrix<> maximalCones = complex.give("MAXIMAL_POLYTOPES");
  Matrix<Rational> rays = complex.give("VERTICES");
  const Matrix<Rational>& linspace = complex.give("LINEALITY_SPACE");
  Vector<Integer> weights = complex.give("WEIGHTS");

  // Find out which cones are no longe compatible
  Set<int> remainingCones;
  for (int c = 0; c < maximalCones.rows(); ++c) {
    if (is_coneset_compatible(maximalCones.row(c), cones)) {
      remainingCones += c;
    }
  }

  // Adapt cone description and ray indices
  maximalCones = maximalCones.minor(remainingCones,All);
  weights = weights.slice(remainingCones);
  Set<int> usedRays = accumulate(rows(maximalCones),operations::add());

  // We have to take care when adapting the local restriction:
  // If cones is input by hand, it may have less columns then there are rays left.
  IncidenceMatrix<> newlocalcones(cones.rows(), rays.rows());
  newlocalcones.minor(All, sequence(0, cones.cols())) = cones;
  newlocalcones = newlocalcones.minor(All,usedRays);

  rays = rays.minor(usedRays,All);
  maximalCones = maximalCones.minor(All,usedRays);

  perl::Object result("Cycle", mlist<Addition>());
  result.take("VERTICES") << rays;
  result.take("MAXIMAL_POLYTOPES") << maximalCones;
  result.take("LINEALITY_SPACE") << linspace;
  result.take("WEIGHTS") << weights;
  result.take("LOCAL_RESTRICTION") << newlocalcones;

  return result;
}

} }

#endif
