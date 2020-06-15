/* Copyright (c) 1997-2020
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
#include "polymake/Array.h"
#include "polymake/Set.h"
#include "polymake/Bitset.h"
#include "polymake/IncidenceMatrix.h"

namespace polymake { namespace polytope {
namespace {

typedef std::list<std::string> label_list;
typedef RestrictedIncidenceMatrix<only_cols> facet_matrix;

template <typename SetTop>
std::string circuit_label (const GenericSet<SetTop>& circuit)
{
   std::string label;
   Int pos = 0;
   for (auto it = entire(circuit.top()); !it.at_end(); ++it) {
      const Int e(*it), e2(e/2);
      label.append(e2-pos, '0');
      label+= e%2==0? '+' : '-';
      pos=e2+1;
   }
   return label;
}

template <typename SetTop> inline
void add_facet (facet_matrix& VIF, label_list& labels,
                const GenericSet<SetTop>& circuit, const Array<Bitset>& CubeFacets)
{
   VIF /= accumulate(select(CubeFacets, circuit), operations::mul());
   labels.push_back(circuit_label(circuit));
}

// subset of given order represented by least significant bits
Int first_valid_subset(const Int order) { return (1L << order)-1; }

bool last_bit_set(const Int x) { return x&1; }
bool last_bit_clear(const Int x) { return !(x&1); }
bool last_two_bits_clear(const Int x) { return !(x&3); }

// count the number of bits
Int card(Int subset)
{
   return __builtin_popcountl(static_cast<unsigned long>(subset));
}

// check for even gaps and set 0-bit
bool valid(Int subset)
{
   if (!subset) return true;
   if (last_bit_clear(subset)) return false;
   for (;;) {
      while (last_bit_set(subset)) subset/=2;
      if (!subset) return true;
      while (last_two_bits_clear(subset)) subset/=4;
      if (last_bit_clear(subset)) return false;
   }
}

// next subset of given order with even gaps (no gap at the beginning)
Int next_valid_subset(const Int order, Int last)
{
   Int subset(last | 1); // make sure it's odd
   for (;;) {
      subset += 2;
      const Int c = card(subset);
      if (c != order) continue;
      if (valid(subset)) return subset;
   }
}

void extend_circuits(facet_matrix& VIF,
                     label_list& labels,
                     const Set<Int>& circuit,
                     const Array<Bitset>& CubeFacets,
                     const Int n, const Int d,
                     const Int p, const Int k)
{
   // enumerate some subsets of {p+k,..,n-1} of order n-d+1-p
   const Int
      order = n-d-p+1,
      o2 = 1L <<order,
      m = n-p-k, // number of bits necessary to represent subset
      m2 = 1L << m; // 2**m
   Int subset = first_valid_subset(order);
   while (subset < m2) {
      // variate all signs
      for (Int sign_vector = 0; sign_vector < o2; ++sign_vector) {
         Set<Int> S;
         Int check_signs = 1;
         for (Int y = 1, no = 0; y <= subset; y *= 2, ++no) {
            if (y&subset) {
               if (check_signs&sign_vector)
                  S += 2*(p+k+no)+1;
               else
                  S += 2*(p+k+no);
               check_signs*=2;
            }
         }
         add_facet(VIF, labels, circuit+S, CubeFacets);
      }
      if (order==1) break;
      subset = next_valid_subset(order,subset);
   }
}

} // end unnamed namespace

BigObject neighborly_cubical(Int d, Int n)
{
  // there is a certain restriction on the parameters
  const Int m = 8*sizeof(Int)-2;
  if (d < 2 || d > n || n > m)
    throw std::runtime_error("neighborly_cubical: 2 <= d <= n <= " + std::to_string(m));

  const Int n_vertices = 1L << n;

  // produce a combinatorial description of the n-cube
   Array<Bitset> CubeFacets(2*n);
   for (Int i = 0; i < n_vertices; ++i)
     for (Int x = i, k = 0; k < n; x >>= 1, ++k)
       CubeFacets[2*k+(x%2)] += i;

   facet_matrix VIF(n_vertices);
   label_list labels;

   Set<Int> circuit; // the one to extend

   // the "p==0" case: Gale's evenness criterion
   for (Int k = 1; k < d; ++k)
      extend_circuits(VIF,labels,circuit,CubeFacets,n,d,0,k);

   // take the first p but not the (p+1)st
   for (Int p = 1; p <= n-d; ++p) {
      const Int pm1 = p-1;
         
      // the signs among the first p-1 vectors alternate, starting with -1
      Set<Int> c_pm1(series(1,pm1-pm1/2,4) + series(2,pm1/2,4));

      // consider an even gap; then the sign sigma at p is (-1)^p
      if (last_bit_clear(p)) // p even
         circuit = c_pm1 + (2*pm1);   // "+"
      else
         circuit = c_pm1 + (2*pm1+1); // "-"

      // just a partial circuit up to now
      for (Int k = 2; p+k < n; k += 2) {
         extend_circuits(VIF,labels,circuit,CubeFacets,n,d,p,k);
      }

      // consider an odd gap; then the sign sigma at p is (-1)^p
      if (last_bit_clear(p)) // p even
         circuit = c_pm1 + (2*pm1+1); // "-"
      else
         circuit = c_pm1 + (2*pm1);   // "+"
      // just a partial circuit up to now
      for (Int k = 1; p+k < n; k += 2) {
         extend_circuits(VIF,labels,circuit,CubeFacets,n,d,p,k);
      }
   }

   // now we need the circuits consisting of the first n-d+1 (two choices for signs)
   const Int pm1 = n-d;
   circuit = series(1,pm1-pm1/2,4) + series(2,pm1/2,4); // always starts with a negative one
   add_facet(VIF, labels, circuit+(2*pm1), CubeFacets);
   add_facet(VIF, labels, circuit+(2*pm1+1), CubeFacets);

   BigObject p_out("Polytope<Rational>",
                   "VERTICES_IN_FACETS", IncidenceMatrix<>(std::move(VIF)),
                   "N_VERTICES", n_vertices,
                   "FACET_LABELS", labels);
   p_out.set_description() << "Neighborly cubical " << d << "-polytope" << endl;
   return p_out;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the combinatorial description of a neighborly cubical polytope."
                  "# The facets are labelled in oriented matroid notation as in the cubical Gale evenness criterion."
                  "#\t See Joswig and Ziegler, Discr. Comput. Geom. 24:315-344 (2000)."
                  "# @param Int d dimension of the polytope"
                  "# @param Int n dimension of the equivalent cube"
                  "# @return Polytope",
                  &neighborly_cubical, "neighborly_cubical");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
