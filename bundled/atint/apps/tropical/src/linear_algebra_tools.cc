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
#include "polymake/ListMatrix.h"
#include "polymake/Matrix.h"
#include "polymake/Vector.h"
#include "polymake/Rational.h"
#include "polymake/tropical/linear_algebra_tools.h"


namespace polymake { namespace tropical {

Vector<Rational> linearRepresentation(Vector<Rational> w, Matrix<Rational> A)
{
  Vector<Rational> solution(A.rows());
  Matrix<Rational> U = unit_matrix<Rational>(A.rows()); //The transformation matrix for A

  if (w.dim() != A.cols()) {
    throw std::runtime_error("Dimension mismatch of generating set and vector");
  }

  int usedRows = 0; //The number of times we actually used a row for reducing
  Rational coeff = 0;
  Vector<Rational> zv = zero_vector<Rational>(w.dim());
  if (w == zv) return solution;

  // Go through each column of w / A and try to reduce it using a row of A
  for (int c = 0; c < w.dim(); ++c) {
    // Find the first row of A such that A(row,c) != 0 and use this row to reduce the column c.
    // Then move it to the end (i.e. above the row we used the last time)
    for (int r = 0; r < A.rows() - usedRows; ++r) {
      if (A(r,c) != 0) {
        // First reduce w, if necessary
        if (w[c] != 0) {
          coeff = w[c] / A(r,c);
          solution += coeff * U.row(r);
          w -= coeff * A.row(r);
          if (w == zv) return solution;
        }
        // Actually we first move the row to the end
        for (int sc = 0; sc < A.cols(); ++sc) {
          A(r,sc).swap(A(A.rows()-usedRows-1,sc));
        }
        for (int uc = 0; uc < U.cols(); ++uc) {
          U(r,uc).swap(U(A.rows()-usedRows-1,uc));
        }
        // Now we reduce all rows below it (the ones above, we don't need anymore)
        for (int s = 0; s < A.rows() - usedRows - 1; ++s) {
          if (A(s,c) != 0) {
            coeff = A(s,c) / A(A.rows() - usedRows -1,c);
            A.row(s) -= coeff * A.row(A.rows() - usedRows - 1);
            U.row(s) -= coeff * U.row(A.rows() - usedRows - 1);
          }
        }
        ++usedRows;
        break;
      }
      // If we arrive at this point, we cant reduce w, so it's not in the linear span
      if (r == A.rows() - usedRows - 1 && w[c] != 0) {
        return Vector<Rational>(0);
      }
    }
    // If we arrive here, there were no more rows in A we could use
    // Hence w can not be reduced, it is not in the linear span
    if (w[c] != 0) {
      return Vector<Rational>(0);
    }
  }

  return solution;
} // END linearRepresentation

Vector<Rational> functionRepresentationVector(const Set<int>& rayIndices,
                                              const Vector<Rational>& v,
                                              const Matrix<Rational>& rays,
                                              const Matrix<Rational>& linealitySpace)
{
  const int ambient_dim = std::max(rays.cols(), linealitySpace.cols());
  const int lineality_dim = linealitySpace.rows();
  // Put ray indices in fixed order
  Array<int> fixedIndices(rayIndices);
  // Matrix of generators
  ListMatrix<Vector<Rational>> m(0,ambient_dim);
  // First affine ray 
  Vector<Rational> baseray;
  int baseRayIndex = -1; //Index of baseray in fixedIndices

  // Compute matrix of generators:
  // We want to write v as a*first_vertex + sum b_i (vertex_i - first_vertex) + sum c_i ray_i 
  // + sum d_i lin_i
  for (auto r = entire<indexed>(fixedIndices); !r.at_end(); ++r) {
    const auto& rayVector = rays.row(*r);
    // Add all directional rays as is
    if (rayVector[0] == 0) {
      m /= rayVector;
    } else if (baseRayIndex == -1) {
      // Use relative differences in the vertex case
      baseray = rayVector;
      baseRayIndex = r.index();
    } else {
      m /= (rayVector - baseray);
    }
  }
  if (lineality_dim > 0) {
    m /= linealitySpace;
  }

  // If there were no row indices, i.e. m = 0-space, just append a zero row
  if (m.rows() == 0) {
    m /= zero_vector<Rational>(ambient_dim);
  }

  // Now compute the representation
  Vector<Rational> repv = linearRepresentation(v,m);

  if (repv.dim() == 0) {
    throw std::runtime_error("Error: vector not in affine span of generators");
  }

  // Insert coefficients at correct places
  Vector<Rational> result(lineality_dim + rays.rows());
  for (auto r = entire<indexed>(fixedIndices); !r.at_end(); ++r) {
    int r_index = r.index();
    if (r_index != baseRayIndex) {
      // If a ray came after the baseray, its matrix row index is one lower then its array index.
      int matrixindex = (baseRayIndex == -1) ? r_index : (r_index > baseRayIndex ? r_index-1 : r_index);
      result[*r] = repv[matrixindex];
      // if this is an affine ray, substract its coefficient at the baseray
      if (rays(*r,0) != 0) {
        result[fixedIndices[baseRayIndex]] -= repv[matrixindex];
      }
    }
  }

  // Insert linspace coefficients at the end
  int repvSize = repv.dim();
  for (int lingen = 0; lingen < lineality_dim; ++lingen) {
    result[rays.rows() + lingen] = repv[repvSize - lineality_dim + lingen];
  }
  return result;    
} // END functionRepresentationVector

// PERL WRAPPER //////////////////////////////////////////////////////////////

Function4perl(&linearRepresentation, "linearRepresentation(Vector,Matrix)");

Function4perl(&functionRepresentationVector,"functionRepresentationVector(Set<Int>, Vector,Matrix,Matrix)");

} }
