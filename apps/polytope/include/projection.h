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

#pragma once

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/PowerSet.h"
#include "polymake/linalg.h"
#include "polymake/Array.h"
#include "polymake/common/labels.h"


namespace polymake { namespace polytope {

namespace {

template<typename Scalar>
Set<Int> coordinates_to_eliminate(const Array<Int>& indices, Int first_coord, Int last_coord, Int codim, const Matrix<Scalar>& linear_span, bool revert)
{
   Set<Int> coords_to_eliminate;
   if (indices.empty()) {
      bool found = false;
      for (auto i = entire(all_subsets_of_k(range(first_coord, last_coord), codim)); !found && !i.at_end(); ++i) {
         if (det(linear_span.minor(All,*i)) != 0) {
            coords_to_eliminate = *i;
            found=true;
         }
      }
      if (!found) throw std::runtime_error("projection: no non-singular minor in LINEAR_SPAN!");
   } else {
      for (auto i = entire(indices); !i.at_end(); ++i) {
         if (*i < first_coord || *i > last_coord)
            throw std::runtime_error("projection: index out of range");
         coords_to_eliminate += *i;
      }
      if (!revert)
         coords_to_eliminate=range(first_coord,last_coord)-coords_to_eliminate;
   }
   return coords_to_eliminate;
}

template<typename Scalar>
void process_rays(BigObject& p_in, Int first_coord, const Array<Int>& indices, OptionSet& options, const Matrix<Scalar>& linear_span, const Set<Int>& coords_to_eliminate, BigObject& p_out)
{
   Matrix<Scalar> Rays, lineality;
   bool points_read=false;
   std::string got_property;
  
   if (p_in.lookup_with_property_name("RAYS | INPUT_RAYS", got_property) >> Rays) {
      points_read=true;
      if ( indices.empty() )  { // if we do a full projection then vertices remain vertices, so we write back whatever we got
         p_out.take(got_property) << Rays.minor(All,~coords_to_eliminate);
         if (p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> lineality && lineality.rows() > 0)
            p_out.take( got_property=="RAYS" || got_property=="VERTICES" ? "LINEALITY_SPACE" : "INPUT_LINEALITY") << lineality.minor(All,~coords_to_eliminate);
         else {
            Matrix<Rational> empty(0, Rays.cols() - coords_to_eliminate.size());
            p_out.take( got_property=="RAYS" || got_property=="VERTICES" ? "LINEALITY_SPACE" : "INPUT_LINEALITY") << empty;
         }
      } else {
         p_out.take("INPUT_RAYS") << remove_zero_rows(Rays.minor(All,~coords_to_eliminate));
         if (p_in.lookup("LINEALITY_SPACE | INPUT_LINEALITY") >> lineality && lineality.rows() > 0) 
            p_out.take("INPUT_LINEALITY") << lineality.minor(All,~coords_to_eliminate);  
         else {
            Matrix<Rational> empty(0, Rays.cols() - coords_to_eliminate.size());
            p_out.take("INPUT_LINEALITY") << empty;
         }
      }
   }
  
   if (!points_read && options["nofm"])
      throw std::runtime_error("projection: no rays found and Fourier-Motzkin elimination excluded");
   if ( indices.empty() && !options["no_labels"]) {
      // here we assume that, if VERTEX_LABELS are present in the object, then also VERTICES are known
      // otherwise this will trigger a convex hull computation
      Int n_vertices = p_in.give("N_RAYS");
      const std::vector<std::string> labels = common::read_labels(p_in, "RAY_LABELS", n_vertices);
      p_out.take("RAY_LABELS") << labels;
   }
}

} // end namespace

} // end namespace polytope
} // end namespace polymake

