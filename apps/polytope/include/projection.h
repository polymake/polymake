/* Copyright (c) 1997-2019
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

#ifndef PROJECTION_HEADER
#define PROJECTION_HEADER

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
Set<int> coordinates_to_eliminate(const Array<int>& indices, int first_coord, int last_coord, int codim, const Matrix<Scalar>& linear_span, bool revert)
{
   Set<int> coords_to_eliminate;
   if (indices.empty()) {
      bool found=false;
      for (auto i=entire(all_subsets_of_k(range(first_coord, last_coord), codim)); !found && !i.at_end(); ++i) {
         if (det(linear_span.minor(All,*i))!=0) {
            coords_to_eliminate=*i;
            found=true;
         }
      }
      if (!found) throw std::runtime_error("projection: no non-singular minor in LINEAR_SPAN!");
   } else {
      for (auto i=entire(indices); !i.at_end(); ++i) {
         if (*i<first_coord || *i>last_coord)
            throw std::runtime_error("projection: index out of range");
         coords_to_eliminate+=*i;
      }
      if (!revert)
         coords_to_eliminate=range(first_coord,last_coord)-coords_to_eliminate;
   }
   return coords_to_eliminate;
}

template<typename Scalar>
void process_rays(perl::Object& p_in, int first_coord, const Array<int>& indices, perl::OptionSet& options, const Matrix<Scalar>& linear_span, const Set<int>& coords_to_eliminate, perl::Object& p_out)
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
      int n_vertices = p_in.give("N_RAYS");
      const std::vector<std::string> labels = common::read_labels(p_in, "RAY_LABELS", n_vertices);
      p_out.take("RAY_LABELS") << labels;
   }
}

} // end namespace

} // end namespace polytope
} // end namespace polymake

#endif
