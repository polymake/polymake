/*
 T his program is free s*oftware; you can redistribute it and/or
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
 Copyright (c) 2016-2019
 Ewgenij Gawrilow, Michael Joswig, and the polymake team
 Technische Universität Berlin, Germany
 https://polymake.org
 
 This file contains the definition of the generalized refinement function
 */

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"

#ifndef POLYMAKE_ATINT_REFINE_h_
#define POLYMAKE_ATINT_REFINE_h_

namespace polymake { namespace tropical{ 

  struct RefinementResult {
	perl::Object complex;
	Matrix<Rational> rayRepFromX;
	Matrix<Rational> rayRepFromY;
	Matrix<Rational> linRepFromX;
	Matrix<Rational> linRepFromY;
	Vector<int> associatedRep;
  }; 
  
  
  
  /**
  @brief This is a multi-purpose function used to refine polyhedral complexes and rational functions 
  along each other. It is not intended for direct use by the user but should be wrapped by other function 
  according to each purpose. Its precise behavior is the following: 
  It takes two Cycle objects, X and Y, of which we assume that |X| is contained in |Y|. 
  It then computes a weighted complex X' that has the same support as X, but each cone of which is 
  contained in a cone of Y. It can also compute the following data: 
  For each ray (or lineality space generator) of X' it can compute an appropriate linear representation 
  in rays (and linspace generators) of X and/or Y, which are in a cone containing that ray. 
  This can then be used to compute function values of a function defined on X or Y on the new complex X'. 
  Furthermore, it can compute for each directional ray the index of a vertex sharing a cone with that ray. 
  If X has a [[LOCAL_RESTRICTION]] and refine is true, the new local restriction will be all minimal interior
  cells of previous local cells. Furthermore, all maximal cones that are no longer compatible after refinement 
  are removed. If [[LATTICE_BASES]] have been computed yet, each refined cone will have the lattice basis of 
  the cone containing it.
  @param perl::Object X A Cycle (not necessarily with defined [[WEIGHTS]]). 
  Note that the function doesn't  care whether the list of RAYS is irredundant.
  @param perl::Object Y A Cycle (not necessarily with defined [[WEIGHTS]]), such that |X| is contained in |Y|. 
  Note that the function doesn't  care whether the list of RAYS is irredundant.
  @param bool repFromX Whether a representation of the new rays in the generators of X should be computed
  @param bool repFromY Whether a representation of the new rays in the generators of Y should be computed
  @param bool computeAssoc Whether an affine ray sharing a cone should be found for each directional ray
  @param bool refine Whether we actually need to refine X (true) or whether X is already fine in Y (false)
  @param bool forceLatticeComputation Whether the properties [[LATTICE_BASES]] and [[LATTICE_GENERATORS]] of X 
  should be computed before the refinement. This is false by default.
  @return RefinementResult A struct containing the following data
  (the rep values are only defined, if the corresponding boolean flag was set):
  1) perl::Object complex: The new refined complex X'. It has weights, iff X does. It uses the same tropical
  addition as X.
  2) Matrix<Rational> rayRepFromX, linRepFromX: A row in these matrices corresponds to a row in 
  [[SEPARATED_VERTICES]] or [[LINEALITY_SPACE]] of X' and gives the coefficients in a linear representation 
  in terms of the [[SEPARATED_VERTICES]] or [[LINEALITY_SPACE]] of X. 
  More precisely, each row of rayRepFromX corresponds to the same row of [[SEPARATED_VERTICES]] of complex. 
  Each row of [[SEPARATED_VERTICES]] can be written as a linear combination of [[SEPARATED_VERTICES]] and 
  [[LINEALITY_SPACE]] of X, whose coefficients are stored in the matrix rayRepFromX. 
  The rows of linRepFromX correspond to rows of [[LINEALITY_SPACE]] of complex and they are given as \
  linear combinations of the lineality space generators of X.
  3) Matrix<Rational> rayRepFromY, linRepFromY: Same as 2),only that the linear combinations are given in terms of rays and lineality space generators of Y
  4) Vector<int> associatedRep Gives for each row in [[SEPARATED_VERTICES]] of complex the index of 
  a non-far vertex (in [[SEPARATED_VERTICES]]) sharing a cone. For each non-far vertex i, associatedRep[i] = i
  */
  RefinementResult refinement(perl::Object X, perl::Object Y, bool repFromX, bool repFromY,bool computeAssoc,bool refine, bool forceLatticeComputation=false);

}}

#endif // ATINT_REFINE_h_
