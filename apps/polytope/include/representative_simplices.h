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

#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/Array.h"
#include "polymake/group/permlib.h"
#include "polymake/linalg.h"
#include "polymake/Bitset.h"

namespace polymake { namespace polytope {

template <typename T>
struct type2type { typedef T type; };

template <typename Scalar, typename SetType>
class simplex_rep_iterator {

   using Orbits = Array<Set<int>>;
   using Kernel_type = ListMatrix<SparseVector<Scalar>>;

public:
   simplex_rep_iterator(const Matrix<Scalar>& _V, 
                        int _d, 
                        const group::PermlibGroup& _sym_group) 
      : sym_group(_sym_group)
      , V(_V)
      , d(_d)
      , k(0) 
      , current_kernel(d+1)
      , next_orbits(d+1)
      , next_orbit_iterator(d+1)
      , current_simplex(V.rows())
      , current_simplex_rep(V.rows())
   {
      current_kernel[0] = unit_matrix<Scalar>(V.cols());
      basis_of_rowspan_intersect_orthogonal_complement(current_kernel[0], V[0], black_hole<int>(), black_hole<int>());
      next_orbits[0] = sym_group.orbits();
      next_orbit_iterator[0] = entire(next_orbits[0]);
      if (!initialize_downward())
         throw std::runtime_error("Could not find a sufficiently large independent set. Check your assumptions on the dimension.");
   }

   bool at_end() const {
      return next_orbit_iterator[0].at_end();
   }

   simplex_rep_iterator& operator++() {
      current_simplex -= next_orbit_iterator[k]->front();
      ++next_orbit_iterator[k];
      step_iterator_with_backup();

      if (k==-1) {
         assert(next_orbit_iterator[0].at_end());
         return *this;
      }
      if (k<d) {
         if (!initialize_downward()) {
            next_orbit_iterator[0] = next_orbits[0].end();
            return *this;
         }
      }
      return *this;
   }

protected:

   int step_while_dependent_or_smaller() {
      bool good_vertex_found(false);
      int new_vertex(-1);
      while ( k < d+1  &&  
              !good_vertex_found  &&  
              !next_orbit_iterator[k].at_end()) {
         new_vertex = next_orbit_iterator[k]->front();
         good_vertex_found = 
            k==0 || 
            !is_zero(current_kernel[k] * V[new_vertex]) && 
            new_vertex > next_orbit_iterator[k-1]->front();
         if (!good_vertex_found) {
            ++next_orbit_iterator[k];
            if (!backup_iterator_until_valid()) { 
               return -1;
            }
         }
      }
      return new_vertex;
   }

   bool backup_iterator_until_valid() {
      while (k>0 && next_orbit_iterator[k].at_end()) {
         --k;
         current_simplex -= next_orbit_iterator[k]->front();
         ++next_orbit_iterator[k];
      }
      return !next_orbit_iterator[k].at_end();
   }

   void step_iterator_with_backup() {
      step_while_dependent_or_smaller();
      if (!backup_iterator_until_valid()) {
         return;
      }
      make_current_simplex();
      return;
   }

   bool initialize_downward() {
      // initialize the iterators, starting with the k-th one.
      // k is always the index of the last valid iterator

      for (; k<d+1; ++k) {
         step_iterator_with_backup();
         if (next_orbit_iterator[k].at_end()) return false; 
         if (k<d) {
            current_kernel[k+1] = current_kernel[k];
            basis_of_rowspan_intersect_orthogonal_complement(current_kernel[k+1], V[next_orbit_iterator[k]->front()], black_hole<int>(), black_hole<int>());
            next_orbits[k+1] = sym_group.setwise_stabilizer(current_simplex).orbits();
            next_orbit_iterator[k+1] = entire(next_orbits[k+1]);
         }
      }
      k = d;
      return true;
   }

public:

   const SetType& operator* () const { return current_simplex_rep; }
   const SetType* operator-> () const { return &(operator*()); }

   friend std::ostream& operator<< (std::ostream& os, const simplex_rep_iterator& sit) {
      os << "its: ==(";
      for (int i=0; i<sit.d+1; ++i)
         if (!sit.next_orbit_iterator[i].at_end()) {
            os << sit.next_orbit_iterator[i]->front() << ((i==sit.k) ? "* " : " "); 
         } else {
            os << "! ";
         }
      wrap(os) << ")== set: " << sit.current_simplex << " "; 
      os << "orbits: ";
      for (int i=0; i<sit.d+1; ++i) {
         os << "[";
         for (int j=0; j<sit.next_orbits[i].size(); ++j) {
            os << "{";
            for (const auto& s : sit.next_orbits[i][j]) 
               os << s << " ";
            os << "}";
         }
         os << "]";
      }
      return os;
   }

protected:

   template<typename U>
   void reset_current_simplex() {
      reset_current_simplex(type2type<U>());
   }

   template<typename S> 
   void reset_current_simplex(S) {
      current_simplex.clear();
   }

   void reset_current_simplex(type2type<Bitset>) {
      current_simplex.clear();
   }

   void make_current_simplex() {
      reset_current_simplex<SetType>();
      for (auto ait = entire(next_orbit_iterator); !ait.at_end(); ++ait)
         if (!ait->at_end()) current_simplex += (*ait)->front();
      current_simplex_rep = sym_group.lex_min_representative(current_simplex);
   }

   const group::PermlibGroup sym_group;
   const Matrix<Scalar> V;
   const int d;
   int k;
   Array<Kernel_type> current_kernel;
   Array<Orbits> next_orbits;
   Array<pm::iterator_range<Orbits::const_iterator>> next_orbit_iterator;
   SetType current_simplex, current_simplex_rep;
};
   

      
} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
