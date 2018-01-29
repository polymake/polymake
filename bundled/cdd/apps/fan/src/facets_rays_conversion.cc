/* Copyright (c) 1997-2018
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
#include "polymake/hash_map"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/polytope/cdd_interface.h"

namespace polymake { namespace fan {

template <typename Coord>
void facetsToRays(perl::Object f) 
{
   const int ambientDim = f.give("FAN_AMBIENT_DIM");
   const Matrix<Coord> facets=f.give("FACET_NORMALS");
   if (!facets.rows()) {
      f.take("RAYS") << Matrix<Coord>(0, ambientDim);
      f.take("MAXIMAL_CONES") << IncidenceMatrix<>(0,0);
      f.take("LINEALITY_SPACE") << Matrix<Coord>(0, ambientDim);
      return;
   }

   const SparseMatrix<int> facetIndices=f.give("MAXIMAL_CONES_FACETS");
   const int n_cones=facetIndices.rows();
   Matrix<Coord> linearSpan;
   IncidenceMatrix<> linearSpanIndices;
   bool ls_exists=false;
   if (f.lookup("LINEAR_SPAN_NORMALS") >> linearSpan && linearSpan.rows()) {
      ls_exists=true;
      f.lookup("MAXIMAL_CONES_LINEAR_SPAN_NORMALS") >>linearSpanIndices;
   }

   polytope::cdd_interface::solver<Coord> sv;

   Matrix<Coord> lineality_space;

   RestrictedIncidenceMatrix<> max_cones(n_cones);
   hash_map<Vector<Coord>, int> rays;    
   int n_rays=0;
   // iterate cones
   for (int i=0; i<n_cones;++i) {
      Set<int> pos_eq,neg_eq;
      for (auto e=entire(facetIndices.row(i)); !e.at_end(); ++e) {
         if ((*e)>0)
            pos_eq.push_back(e.index());
         else if ((*e)<0)
            neg_eq.push_back(e.index());
      }
      const Matrix<Coord> c_facets=facets.minor(pos_eq,All)/-facets.minor(neg_eq,All);
      const Matrix<Coord> c_lspan = ls_exists ? linearSpan.minor(linearSpanIndices.row(i), All)
                                              : Matrix<Rational>(0, c_facets.cols());
      std::pair<Matrix<Rational>, Matrix<Rational> > v_description =
         sv.enumerate_vertices(zero_vector<Rational>() | c_facets, zero_vector<Rational>() | c_lspan, true, true);
      Matrix<Coord> c_rays= v_description.first.minor(All,~scalar2set(0));

      if (i==0) {
         lineality_space= v_description.second.minor(All,~scalar2set(0));
         orthogonalize(entire(rows(lineality_space)));
      }
      // we need to normalize the rays wrt the lineality
      // otherwise we might get the combinatorics wrong (via different representatives for the same face)
      if (lineality_space.rows() > 0)
         project_to_orthogonal_complement(c_rays,lineality_space);

      for (auto r=entire(rows(c_rays)); !r.at_end(); ++r) {
         auto r_iti=rays.find(*r);
         if (r_iti==rays.end()) {
            rays[*r]=n_rays;
            max_cones(i, n_rays++)=true;
         }
         else {
            max_cones(i, r_iti->second)=true;
         }
      }
   }

   // create ray matrix
   Matrix<Coord> R(n_rays, ambientDim);
   for (auto r=entire(rays); !r.at_end(); ++r)
      R.row(r->second)=r->first;
   f.take("RAYS")<<R;
   f.take("LINEALITY_SPACE") << lineality_space;
   f.take("MAXIMAL_CONES") << IncidenceMatrix<>(std::move(max_cones));
}

FunctionTemplate4perl("facetsToRays<Coord> (PolyhedralFan<Coord>) : void");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
