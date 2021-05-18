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

	Functions to create special morphisms.
	*/

#pragma once

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/SparseVector.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Set.h"
#include "polymake/Map.h"

namespace polymake { namespace tropical {

// Documentation see perl wrapper
template <typename Addition>
BigObject evaluation_map(Int n, const Matrix<Rational>& delta, Int i)
{
  if (n <= 0 || delta.rows() <= 0 || i <= 0 || i > n) {
    throw std::runtime_error("Cannot create evaluation map: Invalid parameters");
  }

  // Dimension of the target space
  Int r = delta.cols()-1;

  // We create the map on the 
  // The matrix of ev_i is pr(R^r) + G*pr(M_0,N), where
  // pr denotes the projection (to homogeneous coordinates!) 
  // and G is the matrix evaluating the differences between the marked points
  // i and n (which is the special leaf).
  // Since we take ev_n as the special evaluation, the map reads under moduli coordinates:
  // R^((n-1) over 2) -> R^r, x |-> - sum_(k=1)^|delta| x_{ik} v_k (where v_k is the k-th
  // row of delta). 

  // Projection matrices
  Int N = n + delta.rows();
  Int modulidim = (N*(N-3))/2+1; 

  // The projection to R^r needs the first coordinate as homogenizing coordinate.
  const Matrix<Rational> projR = unit_vector<Rational>(r+modulidim, 0) /
                                 (zero_matrix<Rational>(r, modulidim) | unit_matrix<Rational>(r));

  Matrix<Rational> projM = unit_matrix<Rational>(modulidim) | Matrix<Rational>(modulidim,r);

  Matrix<Rational> G(r+1,modulidim);
  if (i < n) {    // If i is the special leaf, G is the zero map
    // First we create the edge index matrix E(i,j) that contains at element i,j 
    // the edge index of edge (i,j)
    // in the complete graph on N-1 nodes (in lexicographic order).
    Int nextindex = 0;
    Matrix<Int> E(N-1,N-1);
    for (Int k = 0; k < N-2; ++k) {
      for (Int j = k+1; j < N-1; ++j) {
        E(k,j) = nextindex;
        E(j,k) = nextindex;
        ++nextindex;
      }
    }	
    // Create index map (i,j) -> index(i,j)
    Int evalIndex = delta.rows() + i-1;
    // Set all rows corr. to coordinate (n+i,k) to -  v_k
    for (Int k = 0; k < delta.rows(); ++k) {
      G.col(E(evalIndex,k)) = Addition::orientation() * delta.row(k);
    }
  }

  Matrix<Rational> map_matrix = projR + G*projM;

  return BigObject("Morphism", mlist<Addition>(), "MATRIX", map_matrix);
}

// Documentation see perl wrapper
template <typename Addition>
BigObject evaluation_map_d(Int n, Int r, Int d, Int i)
{
  if (n <= 0 || r <= 0 || d <= 0 || i <= 0 || i > n) {
    throw std::runtime_error("Cannot create evaluation map: Invalid parameters");
  }

  // Create standard d-fold direction matrix
  Matrix<Rational> delta(0,r+1);
  for (Int k = 0; k <= r; ++k) {
    for (Int j = 1; j <= d; ++j) {
      delta /= unit_vector<Rational>(r+1,k);
    }
  }

  return evaluation_map<Addition>(n,delta,i);
}

// Documentation see perl wrapper
template <typename Addition>
BigObject projection_map(Int n, const Set<Int>& coords)
{
  // Create matrix
  Matrix<Rational> proj_matrix(coords.size(), n+1);
  Int image_index = 0;
  for (const Int c : coords) {
    if (c > n) {
      throw std::runtime_error("Cannot create projection: Image dimension larger than domain dimension");
    }
    proj_matrix.col(c) = unit_vector<Rational>(coords.size(),image_index);
    ++image_index;
  }

  return BigObject("Morphism", mlist<Addition>(), "MATRIX", proj_matrix);
}

// Documentation see perl wrapper
template <typename Addition>
BigObject projection_map_default(Int n, Int m)
{
  if (m > n) {
    throw std::runtime_error("Cannot create projection: Image dimension larger than domain dimension");
  }
  return projection_map<Addition>(n, sequence(0,m+1));
}

// Documentation see perl wrapper
template <typename Addition>
BigObject forgetful_map(Int n, const Set<Int>& leaves_to_forget)
{
  // First check, that the leaves are in (1,..,n)
  if (leaves_to_forget.front() < 1 || leaves_to_forget.back() > n) {
    throw std::runtime_error("Cannot compute forgetful map: The forgotten leaves should be in {1,..,n}");
  }

  // Compute domain and image dimension
  Int domain_dim = (n*(n-3))/2 +1;
  Int small_n = n - leaves_to_forget.size();
  Int image_dim = (small_n*(small_n - 3))/2 + 1;

  // Check if we forget so many leaves that we get the zero map
  if (small_n <= 3) {
    return BigObject("Morphism", mlist<Addition>(), "MATRIX", Matrix<Rational>(small_n == 3 ? 1 : 0, domain_dim));
  }
  // Check if we don't forget anything at all
  if (leaves_to_forget.size() == 0) {
    return BigObject("Morphism", mlist<Addition>(),
                     "MATRIX", unit_matrix<Rational>(domain_dim));
  }

  // Prepare map mapping remaining leaves to {1,..,n-|leaves_to_forget|}
  Map<Int, Int> remaining_map;
  Int next_index = 1;
  for (Int i = 1; i <= n; ++i) {
    if (!leaves_to_forget.contains(i)) {
      remaining_map[i] = next_index;
      ++next_index;
    }
  }

  // This maps each edge (i,j) in the complete graph on small_n-1 vertices to an
  // index in 0 .. N. Each such edge is a flat, whose vector is (minus) a unit vector
  Map<std::pair<Int, Int>, Int> unit_edges;
  Int unit_index = 0;
  for (Int i = 1; i < small_n-1; ++i) {
    for (Int j = i+1; j < small_n; ++j) {
      unit_edges[std::make_pair(i,j)] = unit_index;
      ++unit_index;
    }
  }

  // Prepare matrix representing forgetful map
  Matrix<Rational> ffm(image_dim,0);

  // Compute image of each unit vector e_i, correpsonding to all v_{k,l} (- v_kl for Max)
  // with 1 <= k < l < n
  for (Int k = 1; k < n-1; ++k) {
    for (Int l = k+1; l < n; ++l) {
      // If klset contains forgotten leaves, the ray is mapped to zero
      if (leaves_to_forget.contains(k) || leaves_to_forget.contains(l)) {
        ffm |= zero_vector<Rational>(image_dim);
      } else {
        // First we compute the new number of the leaves in the image
        Int newk = remaining_map[k];
        Int newl = remaining_map[l];

        // The easy case is that newl is not the new maximal leaf
        if (newl < small_n) {
          // The image is just the corr. unit vector.
          ffm |= unit_vector<Rational>(image_dim, unit_edges[std::make_pair(newk,newl)]);
        } //END if newl < small_n
        else {
          // If the new set contains the maximal leaf, we have to take the complement to 
          // compute the moduli coordinates
          Set<Int> complement = (sequence(1,small_n) - newk) - newl;
          // Compute moduli coordinates as usual: Sum up over all flat vectors of the 
          // 1-edge flats, i.e. all pairs contained in complement
          Vector<Rational> ray_image(image_dim);
          Vector<Int> slist(complement);
          for (Int i = 0; i < slist.dim(); ++i) {
            for (Int j = i+1; j < slist.dim(); ++j) {
              // If its the last edge (small_n-2, small_n-1), we add the ones_vector, otherwise, 
              // we substract the corr. unit_vector
              ray_image[ unit_edges[std::make_pair(slist[i], slist[j])] ] += 1;
            }
          } //END compute ray image

          ffm |= ray_image;

        } //END if newl == small_n

      } //END compute image of ray
    } //END iterate l
  } //END iterate k

  return BigObject("Morphism", mlist<Addition>(), "MATRIX", ffm);
}

} }

