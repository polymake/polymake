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

#pragma once

#include "polymake/Rational.h"
#include "polymake/list"
#include "polymake/Vector.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/TropicalNumber.h"

namespace polymake { namespace matroid {

/**
 * @brief Finds the valuation of a subset in a valuated matroid. If its a basis, it returns
 * the corresponding valuation. Otherwise, it returns tropical zero.
 * @param Array<Set<Int>> bases. The list of bases.
 * @param Vector<TropicalNumber<Addition,Scalar> > valuation. The valuations.
 * @param Set<Int> s. The set whose valuation should be computed.
 * @return Int The valuation of s.
 */
template <typename Addition, typename Scalar>
TropicalNumber<Addition,Scalar> find_valuation(const Array<Set<Int>>& bases,
                                               const Vector<TropicalNumber<Addition, Scalar>>& valuation,
                                               const Set<Int>& s)
{
  for (Int b = 0; b < bases.size(); ++b) {
    Set<Int> inter = bases[b] * s;
      if (inter.size() == bases[b].size() && inter.size() == s.size()) {
        return valuation[b];
      }
  } //END iterate bases
  return TropicalNumber<Addition,Scalar>::zero();
} //END find_valuation

// -------------------------------------------
  
template <typename Addition, typename Scalar>
bool check_valuated_basis_axioms(const Array<Set<Int>>& bases,
                                 const Vector<TropicalNumber<Addition, Scalar>>& valuation,
                                 OptionSet options)
{
  bool verbose = options["verbose"];
  // Sanity check
  if (bases.size() != valuation.dim()) {
    throw std::runtime_error("Number of bases and valuations differ.");
  }
  // Check that the list of bases is not empty.
  if (bases.empty()) {
    if (verbose) cout << "There is no basis" << endl;
    return false;
  }

  // Check basis exchange axiom
  for (Int i = 0; i < bases.size(); ++i) {
    for (Int j = 0; j < bases.size(); ++j) {
      if (i != j) {
        Set<Int> iwithoutj = bases[i] - bases[j];
        Set<Int> jwithouti = bases[j] - bases[i];
        for (auto u = entire(iwithoutj); !u.at_end(); ++u) {
          bool found_one = false;
          for (auto v = entire(jwithouti); !v.at_end(); ++v) {
            Set<Int> B1 = bases[i] - *u + *v;
            Set<Int> B2 = bases[j] - *v + *u;
            TropicalNumber<Addition,Scalar> wbi = valuation[i];
            TropicalNumber<Addition,Scalar> wbj = valuation[j];
            TropicalNumber<Addition,Scalar> wB1 = find_valuation(bases,valuation,B1);
            TropicalNumber<Addition,Scalar> wB2 = find_valuation(bases,valuation,B2);
            if (wbi*wbj + wB1*wB2 == wB1*wB2) {
              found_one = true; 
              break;
            }
          } //END iterate potential exchange elements
          if (!found_one) {
            if (verbose) cout << "No basis exchange for bases " << i << " and " << j << " and element " << *u << endl;
            return false;
          }
        } //END iterate basis_i \ basis_j
      } //END if i != j
    } //END iterate second basis
  } //END iterate first basis

  return true;
} //END check_valuated_basis_axioms
  
// -------------------------------------------
  
template <typename Addition, typename Scalar>
bool check_valuated_circuit_axioms(const Matrix<TropicalNumber<Addition,Scalar>>& valuations, OptionSet options)
{
  bool verbose = options["verbose"];
  // First, we compute supports
  Vector<Set<Int>> supports;
  for (Int r = 0; r < valuations.rows(); ++r) {
    Set<Int> supp;
    for (Int i = 0; i < valuations.cols(); ++i) {
      if (TropicalNumber<Addition,Scalar>::zero() != valuations(r,i)) supp += i;
    }
    // Check for the empty set
    if (supp.empty()) {
      if (verbose) cout << "Circuit " << r << " is empty. " << endl;
      return false;
    }
    supports |= supp;
  } //END compute supports

  // Check for equal or contained supports.
  for (Int i = 0; i < supports.dim(); ++i) {
    for (Int j = i+1; j < supports.dim(); ++j) {
      Set<Int> inter = supports[i] * supports[j];
      if (inter.size() == supports[i].size() || inter.size() == supports[j].size()) {
        if (verbose) cout << "Circuits " << i << " and " << j << " have contained supports." << endl;
        return false;
      }
    }
  }

  // Check valuated circuit exchange axiom:
  // For each pair X, Y of valuated circuits and each u in supp(X) cap supp(Y), v in supp(X) \ supp(Y)
  // set X' = (Y_u / X_u) * X (all ops are tropical), so X'_u = Y_u. Then there exists
  // a circuit Z such that u notin supp Z, v in supp Z and 
  // X' + Y + (X_v / Z_v)*Z = X' + Y.
  for (Int x = 0; x < valuations.rows(); ++x) {
    for (Int y = 0; y < valuations.rows(); ++y) {
      if (x != y) {
        Set<Int> suppboth = supports[x] * supports[y];
        Set<Int> supponlyx = supports[x] - supports[y];
        for (auto u = entire(suppboth); !u.at_end(); u++) {
          for (auto v = entire(supponlyx); !v.at_end(); v++) {
            bool found_one = false;
            // Normalize X, such that X and Y agree on u
            Vector<TropicalNumber<Addition,Scalar>> xprime = valuations.row(x);
            xprime *= valuations(y,*u) / valuations(x,*u);
            // Go through circuits that don't have u in its support, but do contain v
            for (Int z = 0; z < supports.dim(); ++z) {
              if (!supports[z].contains(*u) && supports[z].contains(*v)) {
                // Normalize Z such that it agrees with X' on v.
                Vector<TropicalNumber<Addition, Scalar>> zprime = valuations.row(z);
                zprime *= xprime[*v] / valuations(z,*v);
                // Check if Z' + X' + Y = X' + Y
                if (zprime + xprime + valuations.row(y) == xprime + valuations.row(y)) {
                  found_one = true;
                  break;
                }
              }
            } //END iterate possible exchange circuits       

            if (!found_one) {
              if (verbose) {
                cout << "Circuit exchange axiom fails for circuits " << x << " and " << y
                     << " with " << *u << " in common support and " << *v << " only in circuit " << x << endl;
              }
              return false;
            } //END if none found
          } //END iterate X\Y
        } //END iterate common support   
      } //END if x != y
    } //END iterate y
  } //END iterate x

  return true;
} //END check_valuated_circuit_axioms

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
