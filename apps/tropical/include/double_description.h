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

/** @file Double_Description.h
    @brief Implementation of files for tropical double description
  */

#ifndef POLYMAKE_TROPICAL_DOUBLE_DESCRIPTION_H
#define POLYMAKE_TROPICAL_DOUBLE_DESCRIPTION_H

#include "polymake/Rational.h"
#include "polymake/TropicalNumber.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Set.h"
#include "polymake/tropical/covectors.h"
#include "polymake/tropical/thomog.h"

namespace polymake {
   namespace tropical {

      /*
       *  @brief: check if point is contained in a tropical cone defined by
       *  inequalities which are given by their apices and INfeasible sectors
       */
      template <typename VectorTop, typename MatrixTop, typename Addition, typename Scalar>
      bool is_contained(const GenericVector<VectorTop, TropicalNumber<Addition, Scalar> >& point, const GenericMatrix<MatrixTop, TropicalNumber<Addition,Scalar> >& apices, const Array<Set<int>>& sectors)
      {
         bool check = true;
         int row_index = 0;
         IncidenceMatrix<> M(generalized_apex_covector(point, apices));
         for(const auto& r : rows(M)) {
            Set<int> t(r);
            if (incl(t,sectors[row_index])<=0) check = false;
            row_index++;
         }
         return check;
      }

      /*
       *  @brief: check if a point fulfills the inequality system A x <= B x for min resp. A x >= B x for max
       */
      template <typename VectorTop, typename Matrix1, typename Matrix2, typename Addition, typename Scalar>
      bool is_contained(const GenericVector<VectorTop, TropicalNumber<Addition, Scalar> >& point, const GenericMatrix<Matrix1, TropicalNumber<Addition,Scalar> >& lhs, const GenericMatrix<Matrix2, TropicalNumber<Addition,Scalar> >& rhs)
      {
         bool check = true;
         for(int i = 0; i < lhs.rows(); i++) {
            if (point * lhs > point * rhs) check = false;
         }
         return check;
      }
      
      /*
       *  @brief: convert inequality A x <= B x for min resp. A x >= B x for max
       *  to the description by
       *  apices and INfeasible sectors. Here, B is on the INfeasible side.
       */
      template <typename Addition, typename Scalar>
      std::pair< Matrix< TropicalNumber< Addition, Scalar> >, Array<Set<int> > > matrixPair2apexSet(const Matrix< TropicalNumber<Addition, Scalar> >& A, const Matrix< TropicalNumber<Addition,Scalar> >& B)
      {
         typedef TropicalNumber<Addition,Scalar> TNumber;
         Array<Set<int> > sectors(A.rows());
         Matrix<TNumber> W(A.rows(), A.cols());
         TNumber entry;
         for(int i = 0; i < A.rows(); i++) {
            for(int j = 0; j < A.cols(); j++) {
               if ( A(i,j) != B(i,j) ) {
                  entry = A(i,j) + B(i,j);
                  W(i,j) = entry;
                  if ( entry == B(i,j) ) sectors[i] += j;
               }
            }
         }
         return std::make_pair(W, sectors);
      }


      /*
       *  @brief: compute the extremals of a tropical cone given by the
       *  intersection of a tropical halfspace with another tropical cone
       *  which is given by an inner and and outer description.
       */
      template <typename MatrixTop, typename Addition, typename Scalar>
      Matrix<TropicalNumber<Addition, Scalar> > extremals_from_generators(const GenericMatrix<MatrixTop, TropicalNumber<Addition, Scalar> >& generators)
      {
         typedef TropicalNumber<Addition,Scalar> TNumber;
         ListMatrix<Vector<TNumber> > extremals;

         for(const auto& r : rows(generators)) {
            bool is_extremal = false;
            for (auto coord = entire(rows(single_covector(r, generators))); !coord.at_end(); coord++) {
               if (!((*coord).size() - 1)) { is_extremal = true; break; }
            }
            if (is_extremal) {
               extremals /= r;
            }
         }
         return extremals;
      }

      

      /*
       *  @brief: compute the extremals of a tropical cone given by the
       *  intersection of a tropical halfspace with another tropical cone
       *  which is given by its extremals.
       */
      template <typename MatrixTop, typename Vector1, typename Vector2, typename Addition, typename Scalar>
      Matrix<TropicalNumber<Addition, Scalar> > intersection_extremals(const GenericMatrix<MatrixTop, TropicalNumber<Addition, Scalar> >& generators, const GenericVector<Vector1, TropicalNumber<Addition, Scalar> >& infeasible_side, const GenericVector<Vector2, TropicalNumber<Addition, Scalar> >& feasible_side)
      {
         typedef TropicalNumber<Addition,Scalar> TNumber;

         Set<int> remaining_generators;
         int r_index = 0;
         
         // check for all generators if they are contained in the halfspace given by the pair
         // (feasible_side, infeasible_side)
         for(const auto& r : rows(generators)) {
            if (Addition::orientation()*Scalar(feasible_side*r) <= Addition::orientation()*Scalar(infeasible_side*r)) remaining_generators += r_index;
            r_index++;
         }
         //         ListMatrix<Vector<TNumber> > new_points(rows(generators.minor(remaining_generators,All)));
         Set<Vector<TNumber> > new_points;

         Vector<TNumber> k;
         
         for(auto g = entire(rows(generators.minor(remaining_generators,All))); !g.at_end(); ++g) {
            for(auto h = entire(rows(generators.minor(~remaining_generators,All))); !h.at_end(); ++h) {               
               k = (infeasible_side * (*h)) * (*g) + (feasible_side * (*g)) * (*h);
                new_points += normalized_first(k);
            }
         }
         
         Matrix<TNumber> new_generators(new_points.size(), feasible_side.dim(), entire(new_points));
         new_generators /= generators.minor(remaining_generators,All);
         
         return extremals_from_generators(new_generators);
      }


      
      /*
       *  @brief: compute the extremals of a monomial tropical cone given by the
       *  intersection of a tropical halfspace (defined via nondominated point) with a monomial tropical cone
       *  which is given by generators and apices.
       */
      template <typename Matrix1, typename Matrix2, typename VectorTop, typename Addition, typename Scalar>
      Matrix<TropicalNumber<Addition, Scalar> > monoextremals(const GenericMatrix<Matrix1, TropicalNumber<Addition, Scalar> >& generators, const GenericMatrix<Matrix2, TropicalNumber<Addition, Scalar> >& apices, const GenericVector<VectorTop, Scalar>& non_dominated_point)
      {
         typedef TropicalNumber<Addition,Scalar> TNumber;
         
         Set<int> remaining_generators;
         int r_index = 0;
         Vector<TNumber> infeasible_side(non_dominated_point.dim()+1);
         infeasible_side[0] = TNumber::one();

         Vector<TNumber> feasible_side(non_dominated_point.dim()+1);
         feasible_side.slice(~scalar2set(0)) = (-non_dominated_point);

         Vector<TNumber> new_apex((0|non_dominated_point));
         

         // check for all generators if they are contained in the halfspace given by the pair
         // (feasible_side, infeasible_side)
         for(const auto& r : rows(generators)) {
            if (Addition::orientation()*Scalar(feasible_side*r) <= Addition::orientation()*Scalar(infeasible_side*r)) remaining_generators += r_index;
            r_index++;
         }

         //ListMatrix<Vector<TNumber> > new_generators(rows(generators.minor(remaining_generators,All)));
         Set<Vector<TNumber> > new_generators;
         Vector<TNumber> k;

         for(auto g = entire(rows(generators.minor(remaining_generators,All))); !g.at_end(); ++g) {
            for(auto h = entire(rows(generators.minor(~remaining_generators,All))); !h.at_end(); ++h) {               
               k = (infeasible_side * (*h)) * (*g) + (feasible_side * (*g)) * (*h);
               
               Set<int> covered_sectors;
               Set<int> apex_sectors;
               for (auto apex : rows(apices)) {
                  // why does (apices/new_apex) not work??
                  apex_sectors = single_covector(apex, k);
                  if ((apex_sectors.contains(0)) & (apex_sectors.size()==2)) {
                     covered_sectors += apex_sectors;
                  }
               }
               apex_sectors = single_covector(new_apex, k);
               if (apex_sectors.contains(0)) {
                  covered_sectors += apex_sectors;
               }
               if (covered_sectors == support(k)) {
                  new_generators += normalized_first(k);
               }
            }
         }
         Matrix<TNumber> all_generators(new_generators.size(), new_apex.dim(), entire(new_generators)); 
            
         return (generators.minor(remaining_generators,All)/all_generators);
      }


      /*
       * @brief: determine the dual generators of dehomogenized monomial generators
       *
       * @return: a pair of dual generators and the incidences with the primal generators
       */
      template <typename MatrixTop, typename Scalar>
      std::pair< Matrix<TropicalNumber<Min, Scalar> >, IncidenceMatrix<> > dual_description(const GenericMatrix<MatrixTop, Scalar>& monomial_generators) {
         typedef TropicalNumber<Min, Scalar> TNumber;

         int dim = monomial_generators.cols();
         
         Matrix<TNumber> gen(unit_matrix<TNumber>(dim+1));
         ListMatrix<Vector<TNumber> > apices;
         
         for(const auto& mg : rows(monomial_generators) ) {
            gen = monoextremals(gen, apices, mg);
            Vector<TNumber> new_apex(1);
            new_apex[0] = TNumber::one();
            new_apex |= mg;
            apices /= new_apex;
         }

         ListMatrix<Vector<TNumber>> finite_gen;

         // select only those generators with first coord 0
         for(const auto& g : rows(gen) ) {
            if (g[0]==0) finite_gen /= g; 
         }

         // compute the incidences of primal and dual generators
         Array<Set<int> > vif(monomial_generators.rows());
         int i = 0;
         for (const auto& mg : rows(monomial_generators) ) {
            int j = 0;
            for (const auto& dg : rows(finite_gen) ) {
               Vector<Scalar> sg(dg.slice(~scalar2set(0)));
               if (accumulate(sg - mg, operations::min()) == Scalar(dg[0])) vif[i] += j;
               j++;
            }
            i++;
         }

         IncidenceMatrix<> VIF(vif);

         return std::make_pair(finite_gen,VIF);
      }

} }

#endif 

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
