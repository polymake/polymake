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
	Copyright (c) 2016-2022
	Ewgenij Gawrilow, Michael Joswig, and the polymake team
	Technische Universität Berlin, Germany
	https://polymake.org

	Implements pruefer.h
	*/

#include "polymake/tropical/pruefer.h"

namespace polymake { namespace tropical {

// Documentation see header
Matrix<Int> prueferSequenceFromValences(Int n, const Matrix<Int>& valences)
{
  // Compute basic parameters
  Int no_of_edges = valences.cols()-1;
  Int seq_length = n + no_of_edges-1;

  Matrix<Int> result(0, seq_length);

  // Iterate all rows of valences
  for (Int v = 0; v < valences.rows(); ++v) {

    // Compute for each interior vertex n+i,i=1,..k the possible distribution of
    // valence(i) - 2 entries on seq_length - (sum_j=1^(i-1) v_i) - 1 free spaces
    Vector<Array<Set<Int>>> distributions;
    // For convenience, store the number of distributions per vertex in this list:
    Vector<Int> entrymax;
    for (Int k = 0; k < valences.cols()-1; ++k) {
      // Compute number of free entries that will be available for this vertex after
      // having placed all smaller vertices (subtracting the first occurrence, of course)
      Int v_sum = 0; 
      for (Int i = 0; i < k; ++i) {
        v_sum += valences(v,i)-1;
      }
      distributions |= all_subsets_of_k(sequence(0, seq_length - v_sum - 1), valences(v,k)-2);
      entrymax |= distributions[k].size();
    }

    // We find all sequences by iterating the following vector: Read as (s_1,..,s_k),
    // where s_i means we take distributions[i][s_i] to distribute vertex i on the remaining
    // free entries
    Vector<Int> seq_iterator(valences.cols()-1);

    while (true) {
      // Construct sequence corresponding to the iterator
      Vector<Int> current_sequence(seq_length);
      Vector<Int> free_entries(sequence(0, current_sequence.dim()));
      // Go through all but the last vertex
      for (Int k = 0; k < valences.cols()-1; ++k) {
        // Take smallest free entry for first appearance of current vector
        current_sequence[free_entries[0]] = n+k;
        free_entries = free_entries.slice(range_from(1));
        // Insert remaining entries
        const Set<Int>& entry_distro = distributions[k][seq_iterator[k]];
        Set<Int> entries_to_set(free_entries.slice(entry_distro));
        current_sequence.slice(entries_to_set) = same_element_vector(n+k, entries_to_set.size());
        free_entries = free_entries.slice(~entry_distro);
      } // END insert entries

      // Insert last vertex
      current_sequence.slice(free_entries) = same_element_vector(n + valences.cols()-1, free_entries.size());
      result /= current_sequence;

      // Increase seq_iterator by 1
      Int current_index = seq_iterator.dim()-1;
      //Go through the iterator backwards until you find an entry that is not maximal.
      // On the way, set all entries that are maximal to 0.
      // If we have to set the first element to 0, we're done
      while (seq_iterator[current_index] == entrymax[current_index]-1 && current_index >= 0) {
        seq_iterator[current_index] = 0;
        --current_index;
      }
      if (current_index < 0) break;
      ++seq_iterator[current_index];
    } // END iterate all Pruefer sequences      
  } // END iterate valences

  return result;

} // END prueferSequenceFromValences

///////////////////////////////////////////////////////////////////////////////////////

//Documentation see header
Matrix<Int> dimension_k_prueferSequence(Int n, Int k)
{
  // First we create a polytope whose lattice points describe the possible distributions of
  // valences on the interior vertices p_1 < ... < p_(k+1)

  Int vertex_count = k+1;
  Int seq_length = n + k-1;

  // The sum of all valences must be (length of Pruefer sequence + #vertices)
  Matrix<Rational> eq(0, vertex_count+1);
  Vector<Rational> eqvec = ones_vector<Rational>(vertex_count);
  eqvec = Rational(-seq_length - (k+1)) | eqvec;
  eq /= eqvec;

  // Each valence must be >= 3  
  Matrix<Rational> ineq = unit_matrix<Rational>(vertex_count);
  ineq = same_element_vector(Rational(-3), vertex_count) | ineq;

  BigObject p("polytope::Polytope",
              "INEQUALITIES", ineq,
              "EQUATIONS", eq);
  Matrix<Int> latt = p.call_method("LATTICE_POINTS");
  latt = latt.minor(All, range_from(1));

  return prueferSequenceFromValences(n, latt);
} // END dimension_k_prueferSequence

///////////////////////////////////////////////////////////////////////////////////////

Function4perl(&prueferSequenceFromValences, "prueferSequenceFromValences($,Matrix<Int>)");

Function4perl(&dimension_k_prueferSequence, "dimension_k_prueferSequence($,$)");

FunctionTemplate4perl("complex_from_prueferSequences<Addition>($,Matrix<Int>)");

} }
