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
	Copyright (c) 2016-2021
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Contains functions handling pruefer sequences.
	*/

#pragma once

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/tropical/moduli_rational.h"

namespace polymake { namespace tropical {

/**
   @brief This computes the set of all Pruefer sequences of order n fulfilling one of a list of certain valency condition. These conditions are given as a matrix of integers, seen as a list of row vectors. Each row has length k+1, where k is the number of bounded edges. Column c_i, i= 0,..,k stands for the interior vertex labelled n+i and indicates what valence it should have. That means that in a sequence corresponding to row r the vertex n+i occurs valences(r,i)-1 times.
   @param Int n The number of leaves of rational curves for which we compute Pruefer sequences.
   @param Matrix<Int> valences. Each row prescribes a valence for each interior vertex
   @return Matrix<Int> A list of all Pruefer sequences fulfilling one of the valency conditions (as row vectors).
*/
Matrix<Int> prueferSequenceFromValences(Int n, const Matrix<Int>& valences);

/**
   @brief This computes the set of all Pruefer sequences corresponding to k-dimensional combinatorial types in M_0,n
   @param Int n The number of leaves of rational curves
   @param Int k The number of  bounded edges in rational curves
   @return Matrix<Int> A list of all Pruefer sequences of combinatorial types of curves with n leaves and k bounded edges (as row vectors).
*/
Matrix<Int> dimension_k_prueferSequence(Int n, Int k);

/**
   @brief Takes a list of Pruefer sequences and decodes them into a Cycle containing for each sequence the cone that corresponds to it
   @param Int n The parameter n of the M_0,n on which the sequence is defined.
   @param Matrix<Int> A list of Pruefer sequences (as row vectors)
   @return BigObject A cycle (but without any weights)
*/
template <typename Addition>
BigObject complex_from_prueferSequences(Int n, const Matrix<Int>& pseq)
{
  Vector<Set<Int>> rays_as_sets;
  Int br_cols = (n*(n-3))/2 + 1;
  Matrix<Rational> bergman_rays(0,br_cols);
  Vector<Set<Int>> cones;
  Set<Int> all_leaves = sequence(0,n);
  Vector<Rational> onlyones = ones_vector<Rational>(br_cols);
  Int nextindex = 0;
  Matrix<Int> E(n-1,n-1);
  for (Int i = 0; i < n-2; ++i) {
    for (Int j = i+1; j < n-1; ++j) {
      E(i,j) = nextindex;
      E(j,i) = nextindex;
      ++nextindex;
    }
  }

  // Go through each Pruefer sequence and decode it. Then check whether any of its rays
  // has already been added and construct its cone accordingly. 
  for (Int p = 0; p < pseq.rows(); ++p) {
    Vector<Set<Int>> partitions = decodePrueferSequence(pseq.row(p),n);

    Set<Int> pcone;

    // Iterate ray partitions
    for (Int r = 0; r < partitions.dim(); ++r) {
      Set<Int> rset = partitions[r];
      // Check if we have that ray already
      Int index = -1;
      for (Int oray = 0; oray < rays_as_sets.dim(); ++oray) {
        if (rays_as_sets[oray] == rset) {
          index = oray; break;
        }
      }
      if (index == -1) {
        rays_as_sets |= rset;
        pcone += (rays_as_sets.dim()-1);
        // Now create the bergman coordinates of the ray
        Vector<Int> raylist(rset);
        Vector<Rational> newray(br_cols);
        for (Int k = 0; k < raylist.dim()-1; ++k) {
          for (Int l = k+1; l < raylist.dim(); ++l) {
            Int newrayindex = E(raylist[k],raylist[l]);
            // If the newrayindex is one higher than the ray dimension, 
            // this means it is first of all the last pair. Also, we don't
            // add -e_n but e_1 + ... + e_{n-1} (as we mod out lineality)
            if (newrayindex < newray.dim()) {
              newray[newrayindex] = -1;
            } else {
              newray += onlyones;
            }
          }
        }
        bergman_rays /= newray; 
      } else {
        pcone += index;
      }
    } //END iterate rays of sequence
    cones |= pcone;
  } //END iterate Pruefer sequences

  // Add vertex
  bergman_rays = zero_vector<Rational>() | bergman_rays;
  bergman_rays /= unit_vector<Rational>(bergman_rays.cols(),0);
  for (Int mc = 0; mc < cones.dim(); ++mc) {
    cones[mc] += bergman_rays.rows()-1;
  }

  return BigObject("Cycle", mlist<Addition>(),
                   "PROJECTIVE_VERTICES", bergman_rays,
                   "MAXIMAL_POLYTOPES", cones);
}

} }

