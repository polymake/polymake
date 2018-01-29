/* Copyright (c) 1997-2018
   Ewgenij Gawrilow, Michael Joswig (Technische Universitaet Berlin, Germany)
   http://www.polymake.org

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
#include "polymake/Matrix.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/Integer.h"
#include "polymake/linalg.h"
#include "polymake/matroid/deletion_contraction.h"

namespace polymake { namespace matroid {

   namespace {

      int get_index(int element, const Array<int>& index)
      {
         Array<int>::const_iterator where=std::lower_bound(index.begin(), index.end(), element);
         return where != index.end() && *where==element ? where - index.begin() : -1;
      }

      //Produced a potential matrix for binary representation.
      //Also returns the basis for which the unit matrix was chosen.
      std::pair<Matrix<int>, Set<int>> produce_binary_matrix(const int n_elements, const int rank, const Array<Set<int>>& bases)
      {
         const Set<int> basis = bases.front();
         const Array<int> basis_index(basis);

         Matrix<int> bpoints(n_elements, rank);

         //Write the unit matrix for the first basis
         int i=0;
         for (auto b=entire(basis); !b.at_end(); ++b, ++i) {
            bpoints(*b, i)=1;
         }

         //Insert 1's according to basis exchange axiom
         for (auto it=entire(bases); !it.at_end(); ++it) {
            const int intersection_size = (basis * (*it)).size();
            if (intersection_size == rank-1) {
               int temp=(*it-basis).front();
               int basis_element_index=get_index((basis-*it).front(), basis_index);
               bpoints(temp, basis_element_index)=1;
            }
         }

         return std::make_pair(bpoints, basis);
      }

      // Computes the bases of a subset of points mod p
      // Minor is assumed to have full (column) rank.
      Set<Set<int>> bases_for_finite_field(const Matrix<int>& points, const Set<int>& minor, int p)
      {
         Set<Set<int>> result;
         for (auto i=entire(all_subsets_of_k(minor, points.cols())); !i.at_end(); ++i) {
            if (det( points.minor(*i,All)) % p != 0)
               result.push_back(*i);  // all_subsets_of_k come in lexicographically ascending order
         }
         return result;
      }


      // Take a ternary column (i.e. containing 0,1,-1) and changes it to the "next"
      // vector in the sense that the support remains unchanged and the 
      // supporting +-1-part is treated as encoding a binary number, which is incremented by 1.
      // However, the first nonzero entry has to remain 1 always and if the row cannot be increased anymore,
      // the row is resetted to contain only 1's in its support and returns false, otherwise true.
      bool increase_ternary_row( Matrix<int> &points, int row) {
         const Set<int> supp = support(points.row(row));
         if(supp.size() <= 1) return false;
         int fr = supp.front();

         for(Entire< Set<int> >::const_reverse_iterator r_it = rentire(supp); *r_it != fr; r_it++) {
           points(row, *r_it) *= -1;
           if(points(row, *r_it) < 0) return true;
         }

         points.row(row).slice(supp).fill(1);
         //If we arrive here, we can't actually increase the vector 
         return false;         
      }

   }//end namespace

   void binary_representation(perl::Object matroid)
   {
      const Array< Set<int> > bases=matroid.give("BASES");
      const int r = matroid.give("RANK");
      const int n = matroid.give("N_ELEMENTS");

      if(r == 0) {
         matroid.take("BINARY") << 1;
         matroid.take("BINARY_VECTORS") << Matrix<int>(n,1);
         return;
      }

      const Matrix<int> bpoints = produce_binary_matrix(n, r, bases).first;

      const Set<Set<int>> bpoint_basis_set = bases_for_finite_field(bpoints, sequence(0, bpoints.rows()), 2);
      if (bpoint_basis_set == Set<Set<int>>(bases)) {
         matroid.take("BINARY_VECTORS") << bpoints;
         matroid.take("BINARY") << true;
      } else {
         matroid.take("BINARY") << false;
      }

   }

   void ternary_representation(perl::Object matroid)
   {
      const Array< Set<int> > bases=matroid.give("BASES");
      const int r = matroid.give("RANK");
      const int n = matroid.give("N_ELEMENTS");

      if (r == 0) {
         matroid.take("TERNARY") << true;
         matroid.take("TERNARY_VECTORS") << Matrix<int>(n,1);
         return;
      }

      //First we compute the support of the matrix representation.
      std::pair< Matrix<int>, Set<int> > tpoint_prototype = produce_binary_matrix( n, r, bases);
      Matrix<int> tpoints = tpoint_prototype.first;
      const Set<int> unit_basis = tpoint_prototype.second;
      const Array<int> remaining_elements ( Set<int>(sequence(0,n) - unit_basis));

      Map<int,int> label_identity;
      for(Entire<Series<int,true> >::const_iterator li = entire(sequence(0,n)); !li.at_end(); li++) {
         label_identity[*li] = *li;
      }

      //Compute bases of restrictions to unit_basis + remaining_elements[0],...,unit_basis + (all remaining_elements)
      Map<int, Set< Set<int> > > restriction_bases;
      Map<int, Set<int> > restricted_ground_sets;
      restriction_bases[ remaining_elements.size()-1] = Set<Set<int>>{bases};
      restricted_ground_sets[ remaining_elements.size()-1] = sequence(0,n);
      Set<int> deletion_set = Set<int>(); 
      for(int re_index = remaining_elements.size()-1; re_index >= 1; re_index--) {
         restriction_bases[ re_index-1 ] = minor_bases(Deletion(), restriction_bases[re_index], 
                                                       scalar2set(remaining_elements[re_index]), 
                                                       label_identity);
         restricted_ground_sets[ re_index-1 ] = restricted_ground_sets[ re_index ] 
                                                - remaining_elements[re_index];
      }

      // Index of the "remaining_elements" element currently being added
      int backtrack_index = 0;
      bool need_to_increase = false;

      //Backtracking algorithm for iterating all potential presentations.
      while(backtrack_index >= 0) {
         
         if(need_to_increase) {
            bool have_increased = false;
            //The first vector after the unit matrix is never changed.
            if (backtrack_index > 0) 
               have_increased = increase_ternary_row( tpoints, remaining_elements[ backtrack_index ]);

            if (!have_increased) {
               backtrack_index--;
               continue;
            }
            else {
               need_to_increase = false;
            }
         }

         if (bases_for_finite_field(tpoints,restricted_ground_sets[backtrack_index], 3)
               == restriction_bases[ backtrack_index ]) {
            backtrack_index++; 
            if (backtrack_index == remaining_elements.size()) {
               //This is a presentation!
               matroid.take("TERNARY") << true;
               matroid.take("TERNARY_VECTORS") << tpoints;
               return;
            }
         }
         else {
            need_to_increase = true;
         }
      }//END while

      //If we arrive here, it's not ternary
      matroid.take("TERNARY") << false;
   }

   Function4perl(&binary_representation, "binary_representation(Matroid)");
   Function4perl(&ternary_representation, "ternary_representation(Matroid)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
