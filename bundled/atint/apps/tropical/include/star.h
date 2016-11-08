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

*/

#ifndef POLYMAKE_ATINT_STAR_H
#define POLYMAKE_ATINT_STAR_H

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/tropical/convex_hull_tools.h"

namespace polymake { namespace tropical {
   
   /**
     @brief Computes the Star around a point in a given set of cones
     @param point The point around which the star is to be computed, given in non-trop.-homog. coordinates
     @param rays The rays of the surrounding cones (needn't contain point) in non-trop.-homog. coordinates
     @param cones The surrounding cones (i.e. should all contain point). Should form a complex.
     @param result_rays Will contain the rays of the result (without leading zeros). Might not be irredundant
     @param result_cones Will contain the cones of the result. A cone might be given by a redundant list of rays. The cones will be in the exact same order as in "cones", i.e. the i-th fan cone is the star of the i-th cone at point. Note that the rays are not in any way irredundant (but we don't care for this computation)
     */
   template <typename TConesMatrix>
      std::pair< Matrix<Rational>, std::vector< Set<int> > > computeStar(const Vector<Rational>& point, 
            const Matrix<Rational>& rays, 
            const GenericIncidenceMatrix<TConesMatrix>& cones) {
         //Prepare result variables
         ListMatrix<Vector<Rational> > result_rays(0,rays.cols()-1);
         std::vector<Set<int> > result_cones;

         //Iterate all surrounding cones
         for (auto sc = entire(rows(cones)); !sc.at_end(); ++sc) {
            Set<int> cone;
            for (auto r = entire(*sc); !r.at_end(); ++r) {
               if (rays(*r, 0) == 0) {
                  result_rays /= rays.row(*r).slice(1);
               } else {
                  result_rays /= rays.row(*r).slice(1) - point.slice(1);
               }
               cone += (result_rays.rows()-1);
            }
            result_cones.push_back(cone);
         }
         std::pair< Matrix<Rational>, std::vector< Set<int> > > result = 
            std::make_pair( result_rays, result_cones);
         cdd_normalize_rays(result.first);
         return result;
      }
}}

#endif
