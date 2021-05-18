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

	Functions to compute psi classes and products thereof.
	*/

#pragma once

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/Vector.h"
#include "polymake/PowerSet.h"
#include "polymake/tropical/moduli_rational.h"

namespace polymake { namespace tropical {

// Documentation see perl wrapper
template <typename Addition>
BigObject psi_product(Int n, Vector<Int> exponents)
{
  // First we make sure that the exponent vector is valid
  if (exponents.dim() != n) {
    throw std::runtime_error("Cannot compute psi class product: Exponent vector length is not n.");
  }
  Int k_sum = 0;
  for (Int i = 0; i < exponents.dim(); ++i) {
    if (exponents[i] < 0) {
      throw std::runtime_error("Cannot compute psi class product: Negative exponents are not allowed.");
    }
    k_sum += exponents[i];
  }

  // Check the trivial cases
  if (n < 3 || k_sum > n-3) {
    return call_function("zero_cycle");
  }

  // We have to divide each weight by this:
  Integer divisor = 1;
  for (Int k = 0; k < exponents.dim(); ++k) {
    divisor *= Integer::fac(exponents[k]);
  }

  if (k_sum == n-3) {
    // Compute the weight of the origin
    Matrix<Rational> norays(1,(n*(n-3))/2 + 2);
    norays(0,0) = 1;
    Vector<Set<Int>> vertexcone; vertexcone |= scalar2set(0);
    Vector<Integer> singleweight; singleweight |= (Integer::fac(k_sum) / divisor);
    return BigObject("Cycle", mlist<Addition>(),
                     "PROJECTIVE_VERTICES", norays,
                     "MAXIMAL_POLYTOPES", vertexcone,
                     "WEIGHTS", singleweight);
  }

  // ORDER EXPONENT VECTOR ------------------------------------------------------

  // We have to put the exponent vector in descending order, i.e. we have to apply a permutation to
  // the exponents and later to the resulting Pruefer sequence
  Vector<Int> ordered_exponents;
  Vector<Int> permutation;
  Set<Int> assigned;
  while (ordered_exponents.dim() < exponents.dim()) {
    Int max = -1;
    Int max_index = 0;
    for (Int i = 0; i < exponents.dim(); ++i) {
      if (exponents[i] > max && !assigned.contains(i)) {
        max = exponents[i]; max_index = i;
      }
    }

    ordered_exponents |= max;
    permutation |= max_index;
    assigned += max_index;
  }

  // PRÜFER SEQUENCE COMPUTATION ------------------------------------------------


  // Prepare all variables for the Prüfer sequence computation

  // The vertex we currently try to place in the sequence
  // More precisely: The vertex is current_vertex + n
  Int current_vertex = 0;
  // At [i][j] contains the positions in the sequence (counting from 0)
  // that we have already tried for the (j+1)-st occurence of vertex i
  // for the given placement of 0,..,current_vertex-1
  Vector<Vector<Vector<Int>>> placements_tried(n-2-k_sum);
  placements_tried[0] |= Vector<Int>();
  // The current Prüfer sequence (0 = entry we haven't filled yet)
  Vector<Int> current_sequence(2*n - 4 - k_sum);
  // The exponent weights of each entry. The first n are the exponents, the rest is 0
  Vector<Int> weight = ordered_exponents | zero_vector<Int>(n - 4 - k_sum);
  Vector<Int> orig_weight = exponents | zero_vector<Int>(n - 4 - k_sum);
  // At entry i contains the number of occurences we still need for vertex i
  // given the current placement
  Vector<Int> numbers_needed = 2*ones_vector<Int>(n-2-k_sum);

  // This will contain the resulting sequences
  Matrix<Int> result_sequences(0,2*n-4-k_sum);

  while (current_vertex >= 0) {

    // Find the next free space after the last tried position
    Int next_pos = -1;
    Int occurences_so_far = 0;
    // If the vertex index is too large, we have found a solution
    if (current_vertex < numbers_needed.dim()) {
      occurences_so_far = placements_tried[current_vertex].dim()-1;
      // If it's the first occurence (or the last vertex), we always take the first free position, so we only
      // have to look something up, if it's not the first occurence and not the last vertex
      if (occurences_so_far > 0 && current_vertex < numbers_needed.dim() - 1) {
        Int placements_so_far = placements_tried[current_vertex][occurences_so_far].dim();
        // If we have tried any placements so far, we try to take the next free position after
        // the last one tried, otherwise we take the first free entry after the placement of
        // the last occurence
        if (placements_so_far > 0) {
          next_pos = placements_tried[current_vertex][occurences_so_far][ placements_so_far-1];
        } else {
          Int last_placements = placements_tried[current_vertex][occurences_so_far-1].dim();
          next_pos = placements_tried[current_vertex][occurences_so_far-1][last_placements-1];
        }
      }
      // If it is the first occurence, we still have to check if we already tried the first free
      // position
      else {
        if (placements_tried[current_vertex][occurences_so_far].dim() > 0) {
          next_pos = current_sequence.dim();
        }
      }
      do {
        ++next_pos;
        if (next_pos >= current_sequence.dim()) break;
      } while (current_sequence[next_pos] != 0);
    }
    else {
      if (!(Set<Int>(current_sequence)).contains(0)) {
        result_sequences /= current_sequence;
      }
      next_pos = current_sequence.dim();
    }


    // STEP DOWN: If we cannot place the vertex, we go back a step
    if (next_pos >= current_sequence.dim()) {
      // Remove placements of the current step
      if (current_vertex < numbers_needed.dim())
        placements_tried[current_vertex] = placements_tried[current_vertex].slice(~scalar2set(occurences_so_far));
      // and go back one vertex if this is the first occurence
      if (occurences_so_far == 0) {
        --current_vertex;
      }

      if (current_vertex >= 0) {
        // Remove the occurence of the step we moved back to, since we want to recompute it
        Int occurence_to_remove = placements_tried[current_vertex].dim();
        Int placement_to_remove = placements_tried[current_vertex][occurence_to_remove-1].dim();
        Int position_to_remove = placements_tried[current_vertex][occurence_to_remove-1][placement_to_remove-1];

        current_sequence[position_to_remove] = 0;
        numbers_needed[current_vertex] += (1 - weight[position_to_remove]);
      }

    } // END step down

    // INSERT
    else {
      // Insert current vertex and recompute number of entries needed
      current_sequence[next_pos] = current_vertex + n;
      numbers_needed[current_vertex] += (weight[next_pos] -1);
      placements_tried[current_vertex][occurences_so_far] |= next_pos;
      // STEP UP
      // If we still need entries with this vertex, we try the next placement,
      // otherwise we go to the next vertex
      if (numbers_needed[current_vertex] == 0) {
        ++current_vertex;
      }
      if (current_vertex < numbers_needed.dim()) placements_tried[current_vertex] |= Vector<Int>();

    } // END Insert and step up

  } // END compute Prüfer sequences


  // Now we have to permute the columns of the sequences back according to the reordering on the
  // exponents
  result_sequences.minor(All,sequence(0,n)) = permuted_inv_cols(result_sequences.minor(All,sequence(0,n)), permutation);


  // MODULI CONE CONVERSION -----------------------------------------------------------


  // Prepare variables for the tropical fan
  Matrix<Rational> rays(0, (n*(n-3))/2 + 1);
  Vector<Rational> newray( (n*(n-3))/2 + 1);
  Vector<Set<Int>> cones;
  Vector<Integer> tropical_weights;

  // First we create the edge index matrix E(i,j) that contains at element i,j the edge index of edge (i,j)
  // in the complete graph on n-1 nodes
  Int nextindex = 0;
  Matrix<Int> E(n-1,n-1);
  for (Int i = 0; i < n-1; ++i) {
    for (Int j = i+1; j < n-1; ++j) {
      E(i,j) = nextindex;
      E(j,i) = nextindex;
      ++nextindex;
    }
  }

  // Iterate all Pruefer sequences
  for (Int s = 0; s < result_sequences.rows(); ++s) {

    // Convert to partition list
    Vector<Set<Int>> partitions = decodePrueferSequence(result_sequences.row(s), n);

    Set<Int> newcone;

    // Go through each partition and compute its ray
    for (Int p = 0; p < partitions.dim(); ++p) {
      newray.fill(0);
      for (auto raypair = entire(all_subsets_of_k(partitions[p],2)); !raypair.at_end(); ++raypair) {
        Int smaller_index = raypair->front();
        Int larger_index = raypair->back();
        newray[ E(smaller_index,larger_index)] = Addition::orientation();
      } // END iterate pairs in partition

      // Now see if the ray is already in our list, otherwise add it
      Int ray_index = -1;
      for (Int oray = 0; oray < rays.rows(); ++oray) {
        if (rays.row(oray) == newray) {
          ray_index = oray; break;
        }
      }
      if (ray_index == -1) {
        rays /= newray;
        ray_index = rays.rows()-1;
      }

      newcone += ray_index;

    } // END iterate partitions

    // Add the cone
    cones |= newcone;

    // Compute its weight from the Pruefer sequence
    Vector<Int> weights_at_vertices(newcone.size()+1);
    // Read off the k-weight at each vertex from the sequence
    for (Int i = 0; i < n; ++i) {
      weights_at_vertices[result_sequences(s,i) - n] += orig_weight[i];
    }
    Integer w = 1;
    for (Int i = 0; i < weights_at_vertices.dim(); ++i) {
      w *= Integer::fac(weights_at_vertices[i]);
    }
    tropical_weights |= (w / divisor);

  } // END iterate sequences

  // Add a vertex
  rays = zero_vector<Rational>() | rays;
  rays /= unit_vector<Rational>(rays.cols(),0);

  for (Int mc = 0; mc < cones.dim(); ++mc) {
    cones[mc] += scalar2set(rays.rows()-1);
  }

  return BigObject("Cycle", mlist<Addition>(),
                   "PROJECTIVE_VERTICES", rays,
                   "MAXIMAL_POLYTOPES", cones,
                   "WEIGHTS", tropical_weights);

} // END function psi_product


// Documentation see perl wrapper
template <typename Addition>
BigObject psi_class(Int n, Int i)
{
  if (n < 0 || i < 1 || i > n) {
    throw std::runtime_error("Cannot compute psi_class: Invalid parameters");
  }
  return psi_product<Addition>(n, unit_vector<Int>(n,i-1));
} // END function psi_class

} }

