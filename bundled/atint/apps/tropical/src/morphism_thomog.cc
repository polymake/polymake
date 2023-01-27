/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor,
	Boston, MA  02110-1301, USA.

	---
	Copyright (C) 2011 - 2015, Simon Hampe <simon.hampe@googlemail.com>

	---
	Copyright (c) 2016-2023
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Implements morphism_thomog.cc 
	*/

#include "polymake/tropical/thomog.h"
#include "polymake/tropical/morphism_thomog.h"

namespace polymake { namespace tropical {

std::pair<Matrix<Rational>, Vector<Rational>>
thomog_morphism(const Matrix<Rational>& matrix, const Vector<Rational>& translate, Int domain_chart,
                Int target_chart)
{
  // Sanity checks
  if (matrix.rows() != translate.dim()) {
    throw std::runtime_error("Dimensions don't match.");
  }
  if (domain_chart < 0 || target_chart < 0 || domain_chart > matrix.cols() || target_chart > matrix.rows()) {
    throw std::runtime_error("Invalid chart coordinate.");
  }

  // Add up columns and compute homogenizing coordinates.
  Vector<Rational> column_sum = accumulate(cols(matrix), operations::add());
  Rational total_degree = accumulate(column_sum, operations::max());

  Vector<Rational> missing_degree = total_degree * ones_vector<Rational>(column_sum.dim()) - column_sum;

  // Create result
  Matrix<Rational> homog_matrix(matrix.rows() +1, matrix.cols()+1);
  homog_matrix.minor(~scalar2set(target_chart), ~scalar2set(domain_chart)) = matrix;
  homog_matrix.col(domain_chart).slice(~scalar2set(target_chart)) = missing_degree;
  homog_matrix(target_chart,domain_chart) = total_degree;
  Vector<Rational> homog_translate(translate.dim()+1);
  homog_translate.slice(~scalar2set(target_chart)) = translate;

  return { homog_matrix, homog_translate };
}

std::pair<Matrix<Rational>, Vector<Rational>>
tdehomog_morphism(const Matrix<Rational>& matrix, const Vector<Rational>& translate, Int domain_chart,
                  Int target_chart)
{
  // Sanity checks
  if (matrix.rows() != translate.dim()) {
    throw std::runtime_error("Dimensions don't match.");
  }
  if (domain_chart < 0 || target_chart < 0 || domain_chart >= matrix.cols() || target_chart >= matrix.rows()) {
    throw std::runtime_error("Invalid chart coordinate.");
  }

  Matrix<Rational> affine_matrix = matrix.minor(All, ~scalar2set(domain_chart));

  affine_matrix = tdehomog( Matrix<Rational>(T(zero_vector<Rational>(affine_matrix.cols()) / affine_matrix)), target_chart);
  affine_matrix = T(affine_matrix.minor(All, range_from(1)));

  Vector<Rational> affine_translate =
    tdehomog_vec(Vector<Rational>(Rational(0) | translate), target_chart).slice(range_from(1));

  return { affine_matrix, affine_translate };
}

bool is_homogeneous_matrix(const Matrix<Rational>& m)
{
  Vector<Rational> sum_vec = m * ones_vector<Rational>(m.cols());
  return sum_vec == same_element_vector(sum_vec[0], sum_vec.dim());
}

Function4perl(&thomog_morphism, "thomog_morphism($,$; $=0,$=0)");
Function4perl(&tdehomog_morphism, "tdehomog_morphism($,$; $=0,$=0)");
Function4perl(&is_homogeneous_matrix, "is_homogeneous_matrix(Matrix)");

} }
