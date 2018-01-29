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

#ifndef POLYMAKE_TROPICAL_THOMOG_H
#define POLYMAKE_TROPICAL_THOMOG_H

#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/TropicalNumber.h"

namespace polymake{ namespace tropical {

	template <typename Coefficient, typename VType>
	Vector<Coefficient> thomog_vec(const GenericVector<VType, Coefficient> &affine, int chart=0, bool has_leading_coordinate = true) {
		if(affine.dim() <= 1) return Vector<Coefficient>(affine);
		if(chart < 0 || chart > (affine.dim()- (has_leading_coordinate? 1 : 0)))
			throw std::runtime_error("Invalid chart coordinate");
		Vector<Coefficient> proj(affine.dim()+1);
		proj.slice(~scalar2set(has_leading_coordinate? chart+1 : chart)) = affine;
		return proj;
	}

	template <typename Coefficient, typename MType>
	Matrix<Coefficient> thomog (const GenericMatrix<MType, Coefficient> &affine, int chart=0, bool has_leading_coordinate = true) {
		if(affine.rows() == 0) return Matrix<Coefficient>(0,affine.cols()+1);
		if(chart < 0 || chart > (affine.cols() - (has_leading_coordinate? 1 : 0))) {
			throw std::runtime_error("Invalid chart coordinate.");	
		}
		Matrix<Coefficient> proj(affine.rows(),affine.cols() + 1);
		proj.minor(All,~scalar2set(has_leading_coordinate? chart+1 : chart)) = affine;
		return proj;
	}

	//This does the converse of thomog, i.e. removes a coordinate by shifting
	// it to 0.
	template <typename Coefficient, typename MType>
	Matrix<Coefficient> tdehomog(const GenericMatrix<MType, Coefficient> &proj, int chart=0, bool has_leading_coordinate = true) {
		if(chart < 0 || chart > (proj.cols()- (has_leading_coordinate? 2 : 1))){
			throw std::runtime_error("Invalid chart coordinate");
		}
		Matrix<Coefficient> affine(proj);
		if (has_leading_coordinate) {
		  Matrix<Coefficient> sub(repeat_col(affine.col(chart+1),affine.cols()-1));
		  affine.minor(All,sequence(1,affine.cols()-1)) -= sub;
		}
		else  {
  		  Matrix<Coefficient> sub(repeat_col(affine.col(chart),affine.cols()));
		  affine -= sub;
		}				
		return affine.minor(All,~scalar2set(has_leading_coordinate? chart+1 : chart));
	}


	
	template <typename Coefficient, typename VType>
	Vector<Coefficient> tdehomog_vec(const GenericVector<VType, Coefficient> &proj, int chart=0, bool has_leading_coordinate = true) {
		if(proj.dim() <= 1) return Vector<Coefficient>();
		if(chart < 0 || chart > (proj.dim() - (has_leading_coordinate? 2 : 1))) 
			throw std::runtime_error("Invalid chart coordinate");
		
		Vector<Coefficient> affine(proj);
		if(has_leading_coordinate) {
		    Vector<Coefficient> subtr(zero_value<Coefficient>() | affine[chart+1] * ones_vector<Coefficient>(affine.dim()-1));
		    affine -= subtr;
		}
		else {
		    Vector<Coefficient> subtr(affine[chart] * ones_vector<Coefficient>(affine.dim()));
		    affine -= subtr;
		}
		return affine.slice(~scalar2set(has_leading_coordinate? chart+1 : chart));
	}

	/*
	 * @brief: scale the rows of a matrix so that
	 * the first non-null entry of each row is tropical one
	 */
	template <typename Addition, typename Scalar, typename MatrixTop>
	  Matrix<TropicalNumber<Addition, Scalar> > normalized_first(const GenericMatrix<MatrixTop, TropicalNumber<Addition, Scalar> >& homogeneous_points) {
	  typedef TropicalNumber<Addition,Scalar> TNumber;

	  Matrix<TNumber> result(homogeneous_points);
	  for(auto r : rows(result)) {
	    TNumber value;
	    for(auto entry : r) {
	      if (!is_zero(entry)) {value = entry; break;}
	    }
	    if (!is_zero(value)) r /= value;
	  }
	  return result;
	}

	/*
	 * @brief: scale the rows of a matrix so that
	 * the first non-null entry of each row is tropical one
	 */
	template <typename Addition, typename Scalar, typename VectorTop>
	  Vector<TropicalNumber<Addition, Scalar> > normalized_first(const GenericVector<VectorTop, TropicalNumber<Addition, Scalar> >& homogeneous_point) {
	  typedef TropicalNumber<Addition,Scalar> TNumber;

	  Vector<TNumber> result(homogeneous_point);
	  TNumber value;
	  for(auto entry : result) {
	    if (!is_zero(entry)) {value = entry; break;}
	  }
	  if (!is_zero(value)) result /= value;
	  return result;
	}

}}

#endif
