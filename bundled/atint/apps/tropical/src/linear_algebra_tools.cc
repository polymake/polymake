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

	Implements some basic linear algebra functionality.
	*/

#include "polymake/client.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/tropical/linear_algebra_tools.h"
#include "polymake/tropical/LoggingPrinter.h"


namespace polymake { namespace tropical {

	using namespace atintlog::donotlog;
	// using namespace atintlog::dolog;
	//using namespace atintlog::dotrace;
	
	Vector<Rational> linearRepresentation(const Vector<Rational> &v, const Matrix<Rational> &generators) {
		Vector<Rational> solution(generators.rows());
		//Copy arguments
		Matrix<Rational> A(generators);
		Vector<Rational> w(v);
		Matrix<Rational> U = unit_matrix<Rational>(generators.rows()); //The transformation matrix for A

		if(v.dim() != generators.cols()) {
			throw std::runtime_error("Dimension mismatch of generating set and vector");
		}

		int usedRows = 0; //The number of times we actually used a row for reducing
		Rational coeff = 0;
		Vector<Rational> zv = zero_vector<Rational>(w.dim());
		if(w == zv) return solution;

		//Go through each column of w / A and try to reduce it using a row of A
		for(int c = 0; c < v.dim(); c++) {
			//dbgtrace << "Reducing column " << c+1 << endl;
			//Find the first row of A such that A(row,c) != 0 and use this row to reduce the column c.
			//Then move it to the end (i.e. above the row we used the last time)
			for(int r = 0; r < A.rows() - usedRows; r++) {
				if(A(r,c) != 0) {
					//dbgtrace << "Reducing with row " << r+1 << endl;
					//First reduce w, if necessary
					if(w[c] != 0) {
						coeff = w[c] / A(r,c);
						solution += coeff * U.row(r);
						w -= coeff * A.row(r);
						//dbgtrace << "Solution now " << solution << endl;
						if(w == zv) return solution;
					}
					//Actually we first move the row to the end
					for(int sc = 0; sc < A.cols(); sc++) {
						A(r,sc).swap(A(A.rows()-usedRows-1,sc));
					}
					for(int uc = 0; uc < U.cols(); uc++) {
						U(r,uc).swap(U(A.rows()-usedRows-1,uc));
					}
					//Now we reduce all rows below it (the ones above, we don't need anymore)
					for(int s = 0; s < A.rows() - usedRows - 1; s++) {
						if(A(s,c) != 0) {
							coeff = A(s,c) / A(A.rows() - usedRows -1,c);
							A.row(s) -= coeff * A.row(A.rows() - usedRows - 1);
							U.row(s) -= coeff * U.row(A.rows() - usedRows - 1);
						}
					}
					usedRows++;
					//dbgtrace << "Now A = \n" << A << endl;
					//dbgtrace << "Now w = \n" << w << endl;
					break;
				}
				//If we arrive at this point, we cant reduce w, so its not in the linear span
				if(r == A.rows() - usedRows - 1 && w[c] != 0) {
					//dbgtrace << "Not in linear span" << endl;
					return Vector<Rational>(0);
				}
			}
			//If we arrive here, there were no more rows in A we could use
			//Hence w can not be reduced, it is not in the linear span
			if(w[c] != 0) {
				//dbgtrace << "Not in linear span" << endl;
				return Vector<Rational>(0);
			}
		}

		return solution;
	}//END linearRepresentation

	Vector<Rational> functionRepresentationVector(const Set<int> &rayIndices, const Vector<Rational> &v,
			const Matrix<Rational> &rays,
			const Matrix<Rational> &linealitySpace) {

		int ambient_dim = std::max(rays.cols(), linealitySpace.cols());
		int lineality_dim = linealitySpace.rows();
		//dbgtrace << "Starting representation computation" << endl;
		//Put ray indices in fixed order
		Array<int> fixedIndices(rayIndices);
		//Matrix of generators
		Matrix<Rational> m(0,ambient_dim);
		//First affine ray 
		Vector<Rational> baseray;
		int baseRayIndex = -1; //Index of baseray in fixedIndices

		//Compute matrix of generators:
		//We want to write v as a*first_vertex + sum b_i (vertex_i - first_vertex) + sum c_i ray_i 
		// + sum d_i lin_i
		for(int r = 0; r < fixedIndices.size(); r++) {
			Vector<Rational> rayVector = rays.row(fixedIndices[r]);
			//Add all directional rays as is
			if(rayVector[0] == 0) {
				m = m / rayVector;
			}
			//Use relative differences in the vertex case
			else {
				if(baseRayIndex == -1) {
					baseray = rayVector;
					baseRayIndex = r;
				}
				else {
					m = m / (rayVector - baseray);
				}
			}
		}
		if(lineality_dim > 0) {
			m = m / linealitySpace;
		}

		//If there were no row indices, i.e. m = 0-space, just enter a zero row
		if(m.rows() == 0) {
			m = m / zero_vector<Rational>(ambient_dim);
		}

		//dbgtrace << "Generator matrix is " << m << endl;
		//dbgtrace << "Vector is " << v << endl;

		//Now compute the representation
		Vector<Rational> repv = linearRepresentation(v,m);

		//dbgtrace << "Representation vector: " << repv << endl;

		if(repv.dim() == 0) {
			throw std::runtime_error("Error: vector not in affine span of generators");
		}

		//Insert coefficients at correct places
		Vector<Rational> result(lineality_dim + rays.rows());
		for(int r = 0; r < fixedIndices.size(); r++) {
			if(r != baseRayIndex) {
				//If a ray came after the baseray, its matrix row index is one lower then its array index.
				int matrixindex = (baseRayIndex == -1)? r : (r > baseRayIndex? r-1 : r);
				result[fixedIndices[r]] = repv[matrixindex];
				//dbgtrace << "Inserting " << repv[matrixindex] << " at " << fixedIndices[r] << endl;
				//if this is an affine ray, substract its coefficient at the baseray
				if(rays(fixedIndices[r],0) != 0 ) {
					result[fixedIndices[baseRayIndex]] -= repv[matrixindex];
				}
			}
		}

		//dbgtrace << "Result vector is " << result << endl;

		//Insert linspace coefficients at the end
		int repvSize = repv.dim();
		for(int lingen = 0; lingen < lineality_dim; lingen++) {
			result[rays.rows() + lingen] = repv[repvSize - lineality_dim + lingen];
		}
		//dbgtrace << "Done." << endl;
		return result;    
	}//END functionRepresentationVector


	// PERL WRAPPER //////////////////////////////////////////////////////////////
	
	Function4perl(&linearRepresentation, "linearRepresentation(Vector,Matrix)");

	Function4perl(&functionRepresentationVector,"functionRepresentationVector(Set<Int>, Vector,Matrix,Matrix)");
}}
