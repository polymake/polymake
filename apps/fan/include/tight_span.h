/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_TIGHT_SPAN_H
#define POLYMAKE_TIGHT_SPAN_H

#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"

#include "polymake/Set.h"
#include "polymake/list"
#include "polymake/graph/Decoration.h"

namespace polymake { namespace fan {

   //Excludes all faces contained in a list of "boundary faces"
   class NoBoundaryCut {
      protected:
         const std::list<Set<int> >& max_boundary_faces;
         const IncidenceMatrix<>& maximal_cones;
      public:
         NoBoundaryCut(const std::list<Set<int> >& mbf, const IncidenceMatrix<>& mc) :
            max_boundary_faces(mbf), maximal_cones(mc) {}

         inline bool operator()(const graph::lattice::BasicDecoration& data) const {
            if(data.face.size() == 0) return true;
            auto face = accumulate(rows(maximal_cones.minor(data.face,All)), operations::mul());
            for(auto mf_it : max_boundary_faces)
               if(incl(face, mf_it) < 1) return false;
            return true;
         }
   };

   template <typename Scalar, typename MTop, typename VTop, typename MatrixTop>
      Matrix<Scalar> tight_span_vertices( const GenericMatrix<MTop, Scalar>& points, const GenericIncidenceMatrix<MatrixTop>& VIF, const GenericVector<VTop, Scalar>& lift )
      {
         const bool full_dim = ( rank(points)==points.cols() );
         const int dim = full_dim ? points.cols()+1 : points.cols() ;
         Matrix<Scalar> vertices( VIF.rows(), dim );
         Matrix<Scalar> eq(points.rows(),dim);
         eq.minor(All,sequence(0,points.cols())) = points;
         eq.col(0)=lift;
         if(full_dim){
            eq.col( dim-1 ) = ones_vector<Scalar>(points.rows())-points.minor(All,sequence(1,points.cols()-1))*ones_vector<Scalar>(points.cols()-1);
         }

         int i = 0;
         for(auto it= entire(rows(VIF)); !it.at_end(); ++it, ++i){
            vertices.row(i) = null_space(eq.minor(*it,All)).row(0);
            if(vertices[i][0]<0)
               vertices.row(i)*=-1;
         }
         return vertices;
      }

}}

#endif
