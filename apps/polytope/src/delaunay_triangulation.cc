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
#include "polymake/IncidenceMatrix.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

Array<Set<int> > delaunay_triangulation(perl::Object p)
{
   const IncidenceMatrix<> v_i_f = p.give("VERTICES_IN_FACETS");

   //delete the unbounded vertices
   const Set<int> far_face= p.give("FAR_FACE");
   const IncidenceMatrix<> v_i_f_without_far_face=v_i_f.minor(All,~far_face); 
   
   const int dim = p.CallPolymakeMethod("DIM");
   const Matrix<Rational> points = p.give("FACETS");
   const Matrix<Rational> sites = p.give("SITES");
   
   int n_sims=0;

   for (Entire< Cols < IncidenceMatrix<> > >::const_iterator sim=entire(cols(v_i_f_without_far_face)); !sim.at_end(); ++sim) {
      if (sim->size()==dim) {
         Set<int> rows = *sim;
         if (det(sites.minor(rows,All)) != 0) ++n_sims;
      }
      else if (sim->size()>dim) { //we have to triangulate ourselves
         Set<int> rows = *sim;
         const Matrix<Rational> poly=points.minor(rows,All);
       
         perl::Object pol("Polytope<Rational>");
         pol.take("VERTICES")<<poly;
         const Array<Set<int> > tri=pol.give("TRIANGULATION.FACETS");
         const int n_s=tri.size();
         n_sims+=n_s;
      }
   }
   
   Array < Set <int> > triang(n_sims);
   int index=0;
   for (Entire< Cols < IncidenceMatrix<> > >::const_iterator sim=entire(cols(v_i_f_without_far_face)); !sim.at_end(); ++sim) {
      if (sim->size()==dim) {
         Set<int> rows = *sim;
         if (det(sites.minor(rows,All)) != 0) triang[index++]=*sim;
      }
      else if (sim->size()>dim) { //we have to triangulate ourselves
         Set<int> rows = *sim;
         const Matrix<Rational> poly=points.minor(rows,All);
       
         perl::Object pol("Polytope<Rational>");
         pol.take("VERTICES")<<poly;
         const Array< Set <int> > potriag=pol.give("TRIANGULATION.FACETS");
         for (Entire< Array< Set<int> > >::const_iterator j=entire(potriag); !j.at_end(); ++j) {
            Set<int> sim2(entire(select(*sim,*j)));
            if (det(sites.minor(sim2,All)) != 0) triang[index++]=sim2;
         }
      }
   } 
   return triang;        
}

UserFunction4perl("# @category Triangulations, subdivisions and volume"
                  "# Compute the (a) Delaunay triangulation of the given [[SITES]] of a VoronoiDiagram //V//. If the sites are"
                  "# not in general position, the non-triangular facets of the Delaunay subdivision are"

                  "# triangulated (by applying the beneath-beyond algorithm)."
                  "# @param VoronoiDiagram V"
                  "# @return Array<Set<Int>>"
                  "# @author Sven Herrmann",
                  &delaunay_triangulation,"delaunay_triangulation(VoronoiDiagram)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
