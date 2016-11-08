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

#ifndef POLYMAKE_TROPICAL_COVECTORS_H
#define POLYMAKE_TROPICAL_COVECTORS_H

#include "polymake/tropical/arithmetic.h"
#include "polymake/linalg.h"

namespace polymake { namespace tropical {



	/*
	 * @brief compute the covector of a single point in tropical projective space wrt a matrix of generators
	 *
	 */
	template <typename Addition, typename Scalar, typename VectorTop>
	  IncidenceMatrix<> single_covector( const GenericVector<VectorTop, TropicalNumber<Addition,Scalar> > &point, const Matrix<TropicalNumber<Addition,Scalar> > &generators) {
	  typedef TropicalNumber<Addition, Scalar> TNumber;
	  const int dimension(generators.cols());
	  Set<int> non_support = sequence(0, point.dim()) - support(point);

	  Array<Set<int> > pt_covector(dimension);
	  
	  int gn_index = 0;
	  for(typename Entire< Rows< Matrix<TNumber> > >::const_iterator gn = entire(rows(generators)); !gn.at_end(); gn++, gn_index++) {
	    Vector<TNumber> tdiff = rel_coord(*gn, point.top());
	    TNumber extremum = accumulate(tdiff, operations::add());
	    Set<int> extremal_entries = non_support;
	    int td_index = 0;
	    // determine the extremal entries
	    for(typename Entire<Vector<TNumber> >::iterator td = entire(tdiff); !td.at_end(); td++, td_index++) {
	      if(*td == extremum) extremal_entries += td_index;
	    } 
	    // add the containing sectors to the covector of pt
	    for(Entire<Set<int> >::iterator ext_it = entire(extremal_entries); !ext_it.at_end(); ext_it++) {
	      pt_covector[*ext_it] += gn_index;
	    }
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
	template <typename Addition, typename Scalar>
	  Array<IncidenceMatrix<> > covectors( const Matrix<TropicalNumber<Addition,Scalar> > &points,
					       const Matrix<TropicalNumber<Addition,Scalar> > &generators) {
	  typedef TropicalNumber<Addition,Scalar> TNumber;
	  const int n(points.rows());
	  Array<IncidenceMatrix<> > result(n);
	  int pt_index = 0;
	  for(typename Entire< Rows <Matrix<TNumber> > >::const_iterator pt = entire(rows(points)); !pt.at_end(); pt++, pt_index++) {
	    //call the computation of the covector for every single point
	    result[pt_index] = single_covector(*pt, generators);
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
	      result[pt_index] = single_covector(Vector<TropicalNumber<Addition, Scalar>>(pt->slice(1)), generators);
	    }
            else {
	      Set<int> one_entries = support(pt->slice(1)); //the indices of the 1-entries of the 0/1-ray
	      if ( (*pt)[ one_entries.front()+1 ] * Addition::orientation() < 0 )
                one_entries = sequence(0, dimension) - one_entries;

	      result[pt_index] = artificial_ray_covector (one_entries, generators);
	    }
	  }
	  return result;
        }
} }

#endif
