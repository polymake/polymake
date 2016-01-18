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

	Implements morphism_values.h plus some basic property computations.
	*/

#include "polymake/client.h"
#include "polymake/Rational.h"
#include "polymake/Matrix.h"
#include "polymake/tropical/specialcycles.h"
#include "polymake/tropical/LoggingPrinter.h"

namespace polymake { namespace tropical {


	using namespace atintlog::donotlog;
	//   using namespace atintlog::dolog;
	//using namespace atintlog::dotrace;
	

	//Documentation see header
	void computeConeFunction(	const Matrix<Rational> &rays, 
			const Matrix<Rational> &linspace, 
			const Matrix<Rational> &ray_values, 
			const Matrix<Rational> &lin_values, 
			Vector<Rational> &translate, Matrix<Rational> &matrix) {

		//First we need to compute a cone basis

		//Convert vertices to differences of vertices 
		Vector<Rational> basepoint = zero_vector<Rational>(rays.cols());
		Vector<Rational> basepoint_value = zero_vector<Rational>(ray_values.cols());
		Matrix<Rational> converted_rays;
		Matrix<Rational> converted_values;
		bool basepoint_found = false;
		for(int r = 0; r < rays.rows(); r++) {
			if(rays(r,0) == 1) {
				if(basepoint_found) {
					converted_rays /= (rays.row(r) - basepoint);
					converted_values /= (ray_values.row(r) - basepoint_value);
				}
				else {
					basepoint = rays.row(r);
					basepoint_value = ray_values.row(r);
					basepoint_found = true;
				}
			}
			else {
				converted_rays /= rays.row(r);
				converted_values /= ray_values.row(r);
			}
		}
		converted_rays /= linspace;
		converted_values /= lin_values;
		//Remove leading coordinate
		converted_rays = converted_rays.minor(All,~scalar2set(0));
		basepoint = basepoint.slice(~scalar2set(0));

		//dbgtrace << "Basepoint: " << basepoint << endl;
		//dbgtrace << "Basepoint value: " << basepoint_value << endl;

		//Compute basis of rays
		Set<int> ray_basis = basis_rows(converted_rays);
		converted_rays = converted_rays.minor(ray_basis,All);

		//dbgtrace << "Converted rays: " << converted_rays << endl;

		//Now compute a column basis for the computation of the transformation matrix
		Set<int> I = basis_cols(converted_rays);
		Matrix<Rational> inverse = inv(T( converted_rays.minor(All,I)));
		Matrix<Rational> trafo(converted_rays.rows(), converted_rays.cols());
		trafo.minor(All,I) = inverse;

		//dbgtrace << "Trafo: " << trafo << endl;

		//Compute function matrix:
		Matrix<Rational> values = converted_values.minor(ray_basis,All);
		matrix = T(values) * trafo;
		//Special case: If there is only one vertex and no lineality,
		//the matrix will be empty, so we have to set it by hand 
		if(matrix.rows() == 0) {
			int target_dim = std::max(ray_values.cols(), lin_values.cols());
			int domain_dim = std::max(rays.cols(), linspace.cols()) - 1;
			matrix = Matrix<Rational>(target_dim,domain_dim);
		}

		//dbgtrace << "Matrix: " << matrix << endl;

		//Finally, compute the translate
		translate = basepoint_value - matrix * basepoint;
	}

	///////////////////////////////////////////////////////////////////////////////////////

	//Documentation see header
	void computeConeFunction(	const Matrix<Rational> &rays, 
										const Matrix<Rational> &linspace, 
										const Vector<Rational> &ray_values, 
										const Vector<Rational> &lin_values, 
										Rational &translate, Vector<Rational> &functional) {
		//Convert input values
		Matrix<Rational> convert_ray_values(0,ray_values.dim());
		convert_ray_values /= ray_values;
		Matrix<Rational> convert_lin_values(0,lin_values.dim());
		convert_lin_values /= lin_values;
		Vector<Rational> convert_translate;
		Matrix<Rational> convert_functional;

		//Compute result
		computeConeFunction(rays, linspace,  convert_ray_values, convert_lin_values, convert_translate, convert_functional);

		//Convert result
		translate = convert_translate[0];
		functional = convert_functional.row(0);
	}



	/*
	 * @brief Computes the [[DOMAIN]] as the projective torus of right dimension
	 * from a given [[MATRIX]].
	 */
	template <typename Addition>
		void computeDomainFromMatrix(perl::Object morphism) {
			Matrix<Rational> mat = morphism.give("MATRIX");
			perl::Object pt = projective_torus<Addition>(mat.cols()-1,1);
			pt.give("PURE");
			morphism.take("DOMAIN") << pt;
		}

	/*
	 * @brief Computes [[VERTEX_VALUES]] and [[LINEALITY_VALUES]] from
	 * [[DOMAIN]], [[MATRIX]] and [[TRANSLATE]].
	 */
	void computeValuesFromMatrix(perl::Object morphism) {
		//Extract values
		perl::Object domain = morphism.give("DOMAIN");
		Matrix<Rational> rays = domain.give("VERTICES");
		Matrix<Rational> lineality = domain.give("LINEALITY_SPACE");
		Matrix<Rational> matrix = morphism.give("MATRIX");
		Vector<Rational> translate = morphism.give("TRANSLATE");

		Matrix<Rational> vertex_values = T(matrix * T(rays.minor(All,~scalar2set(0))));
		Matrix<Rational> lineality_values = lineality.rows() > 0 ? 
			T(matrix * T(lineality.minor(All,~scalar2set(0)))) : Matrix<Rational>(0,0);

		//For each nonfar vertex, we have to add the translate
		for(int r = 0; r < rays.rows(); r++) {
			if(rays(r,0) != 0) 
				vertex_values.row(r) += translate;
		}

		morphism.take("VERTEX_VALUES") << vertex_values;
		morphism.take("LINEALITY_VALUES") << lineality_values;
	}


	FunctionTemplate4perl("computeDomainFromMatrix<Addition>(Morphism<Addition>) : void");
	Function4perl(&computeValuesFromMatrix, "computeValuesFromMatrix(Morphism) : void");

}}
