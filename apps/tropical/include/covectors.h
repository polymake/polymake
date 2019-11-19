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

#ifndef POLYMAKE_TROPICAL_COVECTORS_H
#define POLYMAKE_TROPICAL_COVECTORS_H

#include "polymake/tropical/arithmetic.h"
#include "polymake/linalg.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace tropical {

   struct CovectorDecoration : public GenericStruct<CovectorDecoration> {
      DeclSTRUCT( DeclFIELD(face, Set<int>)
                  DeclFIELD(rank,int)
                  DeclFIELD(covector,IncidenceMatrix<>) );
      CovectorDecoration() {}
      CovectorDecoration(Set<int> f, int r, IncidenceMatrix<> cv) : face(f), rank(r), covector(cv) {}
   };


   	/*
	 * @brief compute the covector of a single point in tropical projective space wrt a matrix of generators
	 *
	 */
   template <typename Addition, typename Scalar, typename Vector1, typename Vector2>
     Set<int> single_covector( const GenericVector<Vector1, TropicalNumber<Addition,Scalar> > &point, const GenericVector<Vector2, TropicalNumber<Addition,Scalar> > &apex) {
	  typedef TropicalNumber<Addition, Scalar> TNumber;
	  Set<int> pt_covector = sequence(0, point.dim()) - support(point);
	  Vector<TNumber> tdiff = rel_coord(apex, point);
	  TNumber extremum = accumulate(tdiff, operations::add());
	  int td_index = 0;
	  // determine the extremal entries
	  for(auto td : tdiff) {
	    if(td == extremum) pt_covector += td_index;
	    td_index++;
	  }
	  return pt_covector;
	}

   
	/*
	 * @brief compute the covector of a single point in tropical projective space wrt a matrix of generators
	 *
	 */

    template <typename Addition, typename Scalar, typename VectorTop, typename MatrixTop>
      IncidenceMatrix<> single_covector( const GenericVector<VectorTop, TropicalNumber<Addition,Scalar> > &point, const GenericMatrix<MatrixTop, TropicalNumber<Addition,Scalar> > &generators) {
	  typedef TropicalNumber<Addition, Scalar> TNumber;
	  const int dimension(generators.cols());
	  Set<int> non_support = sequence(0, point.dim()) - support(point);
	  Array<Set<int> > pt_covector(dimension);

	  int gn_index = 0;
	  for(auto gn : rows(generators)) {
	    Vector<TNumber> tdiff = rel_coord(gn, point);
	    TNumber extremum = accumulate(tdiff, operations::add());
	    Set<int> extremal_entries = non_support;
	    int td_index = 0;
	    // determine the extremal entries
	    for(auto td : tdiff) {
	      if(td == extremum) extremal_entries += td_index;
	      td_index++;
	    }
	    // add the containing sectors to the covector of pt
	    for(auto ext_it : extremal_entries) {
	      pt_covector[ext_it] += gn_index;
	    }
	    gn_index++;
	  }
	  return IncidenceMatrix<>(pt_covector);
	}


	/*
	 * @brief determine the covector of a 0-1-ray wrt a matrix of generators
	 * for this only the index set of the 1 entries and the supports of the generators are taken into account
	 */
	template <typename Addition, typename Scalar>
	  IncidenceMatrix<> artificial_ray_covector(const Set<int> &one_entries,
						    const Matrix<TropicalNumber<Addition, Scalar>> &generators) {
	  const int dimension(generators.cols());
	  RestrictedIncidenceMatrix<> pt_covector(dimension);
	  int gn_index = 0;
	  for (auto gn = entire(rows(generators)); !gn.at_end(); ++gn, ++gn_index) {

	    if ( incl(one_entries, sequence(0, dimension) - support(*gn)) <= 0 ) {
              for (int r=0; r<dimension; ++r)
                pt_covector(r, gn_index)=true;
	    }
	    else {
	      for (int covector_index : one_entries) {
		pt_covector(covector_index, gn_index)=true;
	      }
	    }
	  }
	  return IncidenceMatrix<>(std::move(pt_covector));
	}


	//Documentation see perl wrapper
	template <typename Addition, typename Scalar, typename Matrix1, typename Matrix2>
	  Array<IncidenceMatrix<> > covectors( const GenericMatrix<Matrix1, TropicalNumber<Addition,Scalar> > &points,
					       const GenericMatrix<Matrix2, TropicalNumber<Addition,Scalar> > &generators) {
	  const int n(points.rows());
	  Array<IncidenceMatrix<> > result(n);
	  int pt_index = 0;
	  for(auto pt : rows(points)) {
	    //call the computation of the covector for every single point
	    result[pt_index] = single_covector(pt, generators);
	    pt_index++;
	  }//END iterate points
	return result;
	}


	//Versions for scalar matrices.

	template <typename Addition, typename Scalar>
		Array<IncidenceMatrix<> > covectors_of_scalar_vertices( const Matrix<Scalar> &points,
									const Matrix<TropicalNumber<Addition,Scalar> > &generators)
        {
	  const int dimension(generators.cols());
	  Array<IncidenceMatrix<> > result(points.rows());
	  int pt_index = 0;
	  for (auto pt = entire(rows(points)); !pt.at_end(); pt++, pt_index++) {
	    if ((*pt)[0] == 1) {
	      result[pt_index] = single_covector(Vector<TropicalNumber<Addition, Scalar>>(pt->slice(range_from(1))), generators);
	    }
            else {
	      Set<int> one_entries = support(pt->slice(range_from(1))); //the indices of the 1-entries of the 0/1-ray
	      if ( (*pt)[ one_entries.front()+1 ] * Addition::orientation() < 0 )
                one_entries = sequence(0, dimension) - one_entries;

	      result[pt_index] = artificial_ray_covector (one_entries, generators);
	    }
	  }
	  return result;
        }

		/*
	 * @brief compute the generalized covector of a single point w.r.t. apices defining inequalities
	 *
	 */
    template <typename Addition, typename Scalar, typename VectorTop, typename MatrixTop>
      IncidenceMatrix<> generalized_apex_covector( const GenericVector<VectorTop, TropicalNumber<Addition,Scalar> > &point, const GenericMatrix<MatrixTop, TropicalNumber<Addition,Scalar> > &apices) {
	  typedef TropicalNumber<Addition, Scalar> TNumber;

	  IncidenceMatrix<> pt_covector(apices.rows(),apices.cols());

	  for (auto apex = entire<indexed>(rows(apices)); !apex.at_end(); ++apex) {
	    TNumber extremum = *apex*point;
	    if (!is_zero(extremum)) {
	      Vector<TNumber> hadaprod(attach_operation(*apex,point.top(),operations::mul()));
	      Set<int> extremal_entries = indices(attach_selector(hadaprod, operations::fix2<TNumber,operations::eq>(extremum)));

	      // add the containing sectors to the covector of pt
	      pt_covector[apex.index()] = extremal_entries;
	    }
	  }
	  return pt_covector;
	}

}

using tropical::CovectorDecoration;

}



#endif
