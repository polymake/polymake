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

#ifndef POLYMAKE_TROPICAL_THOMOG_H
#define POLYMAKE_TROPICAL_THOMOG_H

#include "polymake/Rational.h"
#include "polymake/Matrix.h"

namespace polymake{ namespace tropical {

	template <typename Coefficient>
	Vector<Coefficient> thomog_vec(const Vector<Coefficient> &affine, int chart=0, bool has_leading_coordinate = true) {
		if(affine.dim() <= 1) return Vector<Coefficient>(affine);
		if(chart < 0 || chart > (affine.dim()- (has_leading_coordinate? 1 : 0)))
			throw std::runtime_error("Invalid chart coordinate");
		Vector<Coefficient> proj(affine.dim()+1);
		proj.slice(~scalar2set(has_leading_coordinate? chart+1 : chart)) = affine;
		return proj;
	}

	template <typename Coefficient>
	Matrix<Coefficient> thomog (const Matrix<Coefficient> &affine, int chart=0, bool has_leading_coordinate = true) {
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
	template <typename Coefficient>
	Matrix<Coefficient> tdehomog(const Matrix<Coefficient> &proj, int chart=0, bool has_leading_coordinate = true) {
		if(proj.rows() == 0) return Matrix<Coefficient>(0,proj.cols()-1);
		if(chart < 0 || chart > (proj.cols()- (has_leading_coordinate? 2 : 1))){
			throw std::runtime_error("Invalid chart coordinate");
		}
		Matrix<Coefficient> affine = proj.minor(All,~scalar2set(has_leading_coordinate? chart+1 : chart));
		for(int r = 0; r < affine.rows(); r++) {
			if(has_leading_coordinate) {
				affine.row(r).slice(~scalar2set(0)) -= proj(r,chart+1)* ones_vector<Coefficient>(affine.cols()-1);
			}
			else {
				affine.row(r) -= proj(r,chart)*ones_vector<Coefficient>(affine.cols());
			}
		}
		return affine;
	}

	template <typename Coefficient>
	Vector<Coefficient> tdehomog_vec(const Vector<Coefficient> &proj, int chart=0, bool has_leading_coordinate = true) {
		if(proj.dim() <= 1) return Vector<Coefficient>();
		if(chart < 0 || chart > (proj.dim() - (has_leading_coordinate? 2 : 1))) 
			throw std::runtime_error("Invalid chart coordinate");
		Vector<Coefficient> affine = proj.slice(~scalar2set(has_leading_coordinate? chart+1 : chart));
		if(has_leading_coordinate) {
			affine.slice(~scalar2set(0)) -= proj[chart+1]* ones_vector<Coefficient>(affine.dim()-1);
		}
		else {
			affine -= proj[chart] * ones_vector<Coefficient>(affine.dim());
		}
		return affine;
	}

}}

#endif
