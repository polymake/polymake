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

#ifndef POLYMAKE_TIGHT_SPAN_H
#define POLYMAKE_TIGHT_SPAN_H

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/linalg.h"

#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/fan/face_lattice_tools.h"
#include "polymake/fan/hasse_diagram.h"
#include "polymake/FacetList.h"
#include "polymake/list"

namespace polymake { namespace fan {

   template <typename MatrixTop>
      IncidenceMatrix<>
      tight_span_from_incidence_with_excluded_faces(const GenericIncidenceMatrix<MatrixTop>& VIF, const Set< Set<int> > &excluded_faces, const int upper_bound)
      {
         //special cases
         if(VIF.rows() == 0)
            return IncidenceMatrix<>(0,0);
         if(VIF.rows() == 1)
            return IncidenceMatrix<>({{0}});

         // compute the tight span
         graph::HasseDiagram HD;
         face_lattice::compute_tight_span(T(VIF), excluded_faces, filler(HD,true), upper_bound);
         IncidenceMatrix<> TS(HD.max_faces(), VIF.rows());
         TS.squeeze_cols();
         return TS;
      }

   template <typename MatrixTop>
      IncidenceMatrix<>
      tight_span_from_incidence(const GenericIncidenceMatrix<MatrixTop>& VIF, const Array<IncidenceMatrix<> > &ListOfCones, const Array<int> dims, const int dim, const int upper_bound)
      {
         //special cases
         if(VIF.rows() == 0)
            return IncidenceMatrix<>(0,0);
         if(VIF.rows() == 1)
            return IncidenceMatrix<>({{0}});

         Set<Set<int> > excluded_faces;
         // compute the rigides (in HDpart on hight 0) of the HasseDiagram and determine the excluded faces
         graph::HasseDiagram HDpart = hasse_diagram_fan_computation(VIF, ListOfCones, dims, dim, dim-1);
         for(auto it=entire(HDpart.node_range_of_dim(0)); !it.at_end(); ++it)
            if(HDpart.out_adjacent_nodes(*it).size() < 2)
               excluded_faces+=HDpart.face(*it);

         // compute the tight span
         graph::HasseDiagram HD;
         face_lattice::compute_tight_span(T(VIF), excluded_faces, filler(HD,true), upper_bound);
         IncidenceMatrix<> TS(HD.max_faces(), VIF.rows());
         TS.squeeze_cols();
         return TS;
      }

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
