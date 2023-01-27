/* Copyright (c) 1997-2023
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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

#include "polymake/client.h"
#include "polymake/list"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace matroid {
      
/**
 * @brief Computes the valuated fundamental circuit
 * @param Array<Set<Int>> bases. List of bases
 * @param Vector<TropicalNumber<Addition,Scalar> > valuation. Basis valuations.
 * @param Int b. The index of a basis B.
 * @param Int n. The number of elements of the ground set.
 * @param Int v. An element not in the basis.
 * @return The valuated fundamental circuit C(B,v)
 */
template <typename Addition, typename Scalar>
Vector<TropicalNumber<Addition, Scalar>>
fundamental_circuit(Int n,
                    const Array<Set<Int>>& bases,
                    const Vector<TropicalNumber<Addition, Scalar>>& valuation, 
                    Int b,
                    Int v)
{
  Vector<TropicalNumber<Addition, Scalar>> result(n);
  for (Int i = 0; i < n; ++i) {
    Set<Int> biv = bases[b]+v-i;
    for (Int j = 0; j < bases.size(); ++j) {
      if (bases[j] == biv) {
        result[i] = valuation[j]; break;
      }
    }
  } //END iterate ground set
  return result;    
} //END fundamental_circuit(..)

// -----------------------------------------------------------

/**
 * @brief Computes [[VALUATION_ON_CIRCUITS]] from [[VALUATION_ON_BASES]].
 */
template <typename Addition, typename Scalar>
void valuated_circuits_from_bases(BigObject vm)
{
  // Extract values
  Int n = vm.give("N_ELEMENTS");
  Array<Set<Int>> bases = vm.give("BASES");
  Array<Set<Int>> circuits = vm.give("CIRCUITS");
  Vector<TropicalNumber<Addition, Scalar>> valuation = vm.give("VALUATION_ON_BASES");
	
  // Go through each circuit and find a pair (B,v), such that it is the fundamental circuit C(B,v)
  Matrix<TropicalNumber<Addition, Scalar>> result(circuits.size(), n);
  for (Int c = 0; c < circuits.size(); ++c) {
    for (Int b = 0; b < bases.size(); ++b) {
      Set<Int> missing = circuits[c] - bases[b];
      if (missing.size() == 1) {
        Int v = *( missing.begin());
        result.row(c) = fundamental_circuit(n, bases,valuation, b,v); 
        break;
      }
    }
  }

  vm.take("VALUATION_ON_CIRCUITS") << result;
}

// -----------------------------------------------------------

/**
 * @brief Computes [[VALUATION_ON_BASES]] from [[VALUATION_ON_CIRCUITS]].
 */
template <typename Addition, typename Scalar>
void valuated_bases_from_circuits(BigObject vm)
{
  // Extract values
  Array<Set<Int>> bases = vm.give("BASES");
  Array<Set<Int>> circuits = vm.give("CIRCUITS");
  Matrix<TropicalNumber<Addition, Scalar>> valuation = vm.give("VALUATION_ON_CIRCUITS");

  // We assign valuation tropical one to the first basis. Then we iteratively find
  // bases that differ from an already valuated basis by a basis exchange operation.
  // We then use fundamental circuits to compute the valuation of the new basis
  Vector<TropicalNumber<Addition, Scalar>> result(bases.size());

  result[0] = TropicalNumber<Addition,Scalar>::one();
  Set<Int> defined{0};
  std::list<Int> queue;
  for (Int i = 1; i < bases.size(); ++i)
    queue.push_back(i);
	
  while (!queue.empty()) {
    Int front = queue.front();
    queue.pop_front();
    // Find a valuated basis, such that the symmetric difference has two elements
    bool found_one = false;
    for (auto def = entire(defined); !def.at_end(); ++def) {
      Set<Int> symdif = bases[front] ^ bases[*def];
      if (symdif.size() == 2) {
        found_one = true;
        Int infront = * ((symdif - bases[*def]).begin());
        Int indef = * ((symdif - bases[front]).begin());
        // Find the fundamental circuit of def + infront
        Set<Int> Bplusv = bases[*def] + infront;
        for (Int c = 0; c < circuits.size(); ++c) {
          if ((Bplusv * circuits[c]).size() == circuits[c].size()) {
            // Normalize the circuit such that it is 0 at infront
            Vector<TropicalNumber<Addition, Scalar>> normalized_circuit = valuation.row(c);
            normalized_circuit *= TropicalNumber<Addition,Scalar>::one() / valuation(c,infront);
            // The valuation of the new basis is the valuation
            // of this circuit at position indef * the valuation of the defined basis
            result[front] = normalized_circuit[indef] * result[*def];
            defined += front;
            break;
          }
        } //END iterate circuits
        break;
      } //END if symdif.size = 2
    } //END iterate valuated bases
	  
    // If we found none, we keep it for later.
    if (!found_one) queue.push_back(front);

  } //END iterate queue

  vm.take("VALUATION_ON_BASES") << result;
} //END valuated_bases_from_circuits

// -----------------------------------------------------------
// ----- PERL GLUE ---

//   FunctionTemplate4perl("fundamental_circuit<Addition,Scalar>($,Array<Set<Int> >, Vector<TropicalNumber<Addition,Scalar> >,$, $)");
FunctionTemplate4perl("valuated_bases_from_circuits<Addition,Scalar>(ValuatedMatroid<Addition,Scalar>)");
FunctionTemplate4perl("valuated_circuits_from_bases<Addition,Scalar>(ValuatedMatroid<Addition,Scalar>)");

} }
