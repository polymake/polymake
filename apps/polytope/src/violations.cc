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
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"

namespace polymake { namespace polytope {

namespace {

template <typename Scalar, typename Cmp>
Set<int> violated_rows(const Matrix<Scalar>& A, const Vector<Scalar>& q, Cmp cmp)
{
   Set<int> vr;
   for (typename pm::ensure_features<Rows<Matrix<Scalar> >, pm::cons<pm::end_sensitive, pm::indexed> >::const_iterator rit = ensure(rows(A), (pm::cons<pm::end_sensitive, pm::indexed>*)0).begin(); !rit.at_end(); ++rit) 
      if (cmp(*rit * q)) vr += rit.index();
   return vr;
}

}

template<typename Scalar>
Set<int> violations(const perl::Object P, const Vector<Scalar>& q, perl::OptionSet options)
{
   const std::string section = options["section"];
   const int violating_criterion = options["violating_criterion"];
   const Matrix<Scalar> A = P.give(section);

   return

      (section == "INEQUALITIES" || 
       section == "FACETS" || 
       violating_criterion == -1) 
      ? violated_rows(A, q, pm::operations::negative<Scalar>())

      : ((section == "EQUATIONS" || 
          section == "AFFINE_HULL" ||
          violating_criterion == 0) 
         ? violated_rows(A, q, pm::operations::non_zero<Scalar>())         

         : violated_rows(A, q, pm::operations::positive<Scalar>()));
}

UserFunctionTemplate4perl("# @category Calculations"
                          "# Check which relations, if any, are violated by a point."
                          "# @param Polytope P"
                          "# @param Vector q"
                          "# @option String section Which section of P to test against q"
                          "# @option Int violating_criterion has the options: +1 (positive values violate; this is the default), 0 (*non*zero values violate), -1 (negative values violate)"
                          "# @return Set",
                          "violations<Scalar> (Polytope<Scalar> Vector<Scalar> { section => FACETS, violating_criterion => 1 } )");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
