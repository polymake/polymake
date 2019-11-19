/* Copyright (c) 1997-2019
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

#ifndef POLYMAKE_POLYTOPE_FACE_LATTICE_TOOLS_H
#define POLYMAKE_POLYTOPE_FACE_LATTICE_TOOLS_H

#include "polymake/list"
#include "polymake/PowerSet.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/FaceMap.h"

namespace polymake { namespace polytope { namespace face_lattice {

// first - all facets containing the given face
// second - intersection of these facets = the enclosing face
typedef std::pair< Set<int>, Set<int> > face_closure;

/** Compute the closure of a face S
    @retval first all facets containing S
    @retval second intersection of these facets = the enclosing face
*/
template <typename TSet, typename TMatrix>
face_closure
closure(const GenericSet<TSet, int>& S, const GenericIncidenceMatrix<TMatrix>& M)
{
   // intersection of all facets incident to vertices from S
   const Set<int> facets=accumulate(cols(M.minor(All,S)), operations::mul());

   // intersection of all these facets
   return face_closure(facets, accumulate(rows(M.minor(facets,All)), operations::mul()));
}


// Compute a base for a given face H
template <typename TSet, typename TMatrix>
Set<int> c(const GenericSet<TSet, int>& H, const GenericIncidenceMatrix<TMatrix>& M)
{
   if (H.top().empty())
      return Set<int>();

   auto H_i=entire(H.top());
   Set<int> C=scalar2set(*H_i);     // c(H) to be returned at the end
   Set<int> facets=M.col(*H_i);
   ++H_i;
   while (!H_i.at_end()) {
      int size = facets.size();
      facets *= M.col(*H_i);
      if (size > facets.size())
         C.push_back(*H_i);
      ++H_i;
   }
   return C;
}


/// iterate over the (k+1)-faces containing a given k-face H
template <typename TSet, typename TMatrix>
class faces_one_above_iterator {
public:
   typedef std::forward_iterator_tag iterator_category;
   typedef face_closure value_type;
   typedef const value_type& reference;
   typedef const value_type* pointer;
   typedef ptrdiff_t difference_type;

   faces_one_above_iterator() {}

   faces_one_above_iterator(const GenericSet<TSet, int>& H_arg, const GenericIncidenceMatrix<TMatrix>& M_arg)
      : H(&H_arg)
      , M(&M_arg)
      , candidates(sequence(0,M->cols()) - *H)
      , done(false)
   {
      find_next();
   }

   reference operator* () const { return result; }
   pointer operator-> () const { return &result; }

   faces_one_above_iterator& operator++ () { find_next(); return *this; }

   const faces_one_above_iterator operator++ (int) { faces_one_above_iterator copy=*this; operator++(); return copy; }

   bool operator== (const faces_one_above_iterator& it) const { return candidates==it.candidates; }
   bool operator!= (const faces_one_above_iterator& it) const { return !operator==(it); }
   bool at_end() const { return done; }

   void rewind()
   {
      candidates=sequence(0,M->cols()) - *H;
      minimal.clear();
      done=false;
      find_next();
   }
protected:
   void find_next()
   {
      while (!candidates.empty()) {
         int v=candidates.front();  candidates.pop_front();
         result=closure(*H+v, *M);
         if (result.first.empty()) continue;    // the closure would be the whole polytope - absolutely dull
         if ((result.second * candidates).empty() && (result.second * minimal).empty()) {
            minimal.push_back(v);
            return;
         }
      }
      done=true;
   }

   const GenericSet<TSet>* H;
   const GenericIncidenceMatrix<TMatrix>* M;
   Set<int> candidates, minimal;
   face_closure result;
   bool done;
};

template <typename TSet, typename TMatrix> inline
faces_one_above_iterator<TSet, TMatrix>
all_faces_one_above(const GenericSet<TSet, int>& H, const GenericIncidenceMatrix<TMatrix>& M)
{
   return faces_one_above_iterator<TSet, TMatrix>(H,M);
}

template <typename DiagrammFiller, bool dual> inline
void add_edge(DiagrammFiller& HD, int from, int to, bool_constant<dual>)
{
   if (dual) HD.add_edge(to,from); else HD.add_edge(from,to);
}

/// Compute the face lattice
template <typename TMatrix, typename DiagrammFiller, bool dual>
void compute(const GenericIncidenceMatrix<TMatrix>& VIF, DiagrammFiller HD, bool_constant<dual> Dual, int dim_upper_bound=-1)
{
   std::list< Set<int> > Q;    // queue of faces, which have been seen but who's faces above have not been computed yet.
   FaceMap<> Faces;

   // The bottom node: empty set (or dual: whole polytope)
   const int R=VIF.rows(), C=VIF.cols();
   if (dual)
      HD.add_node(sequence(0,R));
   else
      HD.add_node(Set<int>());

   if ( C == 0 )  // the empty polytope
      return; 

   HD.increase_dim();
   int n, end_this_dim=0, d=0;

   if (__builtin_expect(C>1, 1)) {
      // The first level: vertices.
      copy_range(entire(all_subsets_of_1(sequence(0,C))), std::back_inserter(Q));
      n= dual ? HD.add_nodes(C, cols(VIF).begin())
              : HD.add_nodes(C, all_subsets_of_1(sequence(0,C)).begin());
      end_this_dim=n+C;
      int end_next_dim=end_this_dim;
      HD.increase_dim(); ++d;
      for (int i=n; i<end_this_dim; ++i)
         add_edge(HD,0,i,Dual);

      if (__builtin_expect(C>2 && dim_upper_bound, 1)) {
         bool facets_reached=false;
         for (;;) {
            Set<int> H = Q.front(); Q.pop_front();
            for (faces_one_above_iterator<Set<int>, TMatrix> faces(H, VIF);  !faces.at_end();  ++faces) {
               if (faces->first.size() == 1) {  // we have reached the facet level
                  if (!facets_reached) {
                     if (dual)
                        HD.add_nodes(R, all_subsets_of_1(sequence(0,R)).begin());
                     else
                        HD.add_nodes(R, rows(VIF).begin());
                     facets_reached=true;
                  }
                  add_edge(HD, n, end_this_dim+faces->first.front(), Dual);
               } else {
                  int &node_ref = Faces[c(faces->second, VIF)];
                  if (node_ref==-1) {
                     node_ref=HD.add_node(dual ? faces->first : faces->second);
                     Q.push_back(faces->second);
                     ++end_next_dim;
                  }
                  add_edge(HD,n,node_ref,Dual);
               }
            }
            if (++n == end_this_dim) {
               HD.increase_dim();
               if (__builtin_expect(Q.empty() || d == dim_upper_bound, 0)) break;
               ++d;  end_this_dim=end_next_dim;
            }
         }
      } else {
         end_this_dim=n;
      }
   }

   // The top node: whole polytope (or dual: empty set)
   n= dual ? HD.add_node(Set<int>()) : HD.add_node(sequence(0,C));
   for (int i=end_this_dim; i<n; ++i)
      add_edge(HD,i,n,Dual);
}

typedef std::false_type Primal;
typedef std::true_type Dual;

/// Compute the bounded face lattice, starting always from the vertices (Primal mode)
template <typename TMatrix, typename TSet, typename DiagrammFiller>
void compute_bounded(const GenericIncidenceMatrix<TMatrix>& VIF,
                     const GenericSet<TSet, int>& far_face,
                     DiagrammFiller HD, int dim_upper_bound=-1)
{
   std::list< Set<int> > Q;    // queue of faces, which have been seen but who's faces above have not been computed yet.
   FaceMap<> Faces;

   // The bottom node: empty set
   const int C=VIF.cols();
   HD.add_node(Set<int>());
   HD.increase_dim();
   int end_this_dim=0, end_next_dim=0, d=0, max_faces_cnt=0;

   // The first level: vertices.
   const Set<int> bounded_vertices=sequence(0,C)-far_face;
   const int n_bounded_vertices=bounded_vertices.size();

   if (__builtin_expect(C>1, 1)) {
      // fill first level and add the arcs from the empty set to the bounded vertices.
      copy_range(entire(all_subsets_of_1(bounded_vertices)), std::back_inserter(Q));
      int n=HD.add_nodes(n_bounded_vertices, all_subsets_of_1(bounded_vertices).begin());
      end_next_dim=end_this_dim=n+n_bounded_vertices;
      HD.increase_dim(); ++d;
      for (int i=n; i<end_this_dim; ++i)
         HD.add_edge(0,i);

      if (__builtin_expect(C>2 && dim_upper_bound, 1)) {
         for (;;) {
            Set<int> H = Q.front(); Q.pop_front();
            bool is_max_face=true;
            for (faces_one_above_iterator<Set<int>, TMatrix> faces(H, VIF);  !faces.at_end();  ++faces) {
               int &node_ref = Faces[c(faces->second, VIF)];
               if (node_ref==-1) {
                  if ((faces->second * far_face).empty()) {
                     node_ref=HD.add_node(faces->second);
                     Q.push_back(faces->second);
                     ++end_next_dim;
                  } else {
                     node_ref=-2;
                     continue;
                  }
               } else if (node_ref==-2)
                  continue;
               HD.add_edge(n,node_ref);
               is_max_face=false;
            }
            if (is_max_face) ++max_faces_cnt;
            if (++n == end_this_dim) {
               if (__builtin_expect(Q.empty() || d == dim_upper_bound, 0)) break;
               HD.increase_dim();
               ++d;  end_this_dim=end_next_dim;
            }
         }
      }
   }

   if (max_faces_cnt + end_next_dim-end_this_dim > 1) {
      // The top node is connected to all inclusion-independent faces regardless of the dimension
      int n=HD.add_node(bounded_vertices);
      for (int i=0; i<n; ++i)
         if (HD.graph().out_degree(i)==0)
            HD.add_edge(i,n);
   }
}

} } } // end namespace face_lattice

namespace pm {

template <typename TSet, typename TMatrix>
struct check_iterator_feature<polymake::polytope::face_lattice::faces_one_above_iterator<TSet, TMatrix>, end_sensitive> : std::true_type {};
template <typename TSet, typename TMatrix>
struct check_iterator_feature<polymake::polytope::face_lattice::faces_one_above_iterator<TSet, TMatrix>, rewindable> : std::true_type {};

} // end namespace pm

#endif // POLYMAKE_POLYTOPE_FACE_LATTICE_TOOLS_H

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
