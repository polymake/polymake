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
#include "polymake/IncidenceMatrix.h"
#include "polymake/graph/Lattice.h"
#include "polymake/graph/Decoration.h"
#include "polymake/Set.h"
#include "polymake/hash_set"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/linalg.h"
#include "polymake/topaz/sum_triangulation_tools.h"

namespace polymake { namespace topaz {

namespace {

   typedef Set<int> face_type;
   typedef Set<face_type> complex_type;
   typedef hash_set<complex_type> ballhash_type;

// Decide whether a simplex is star-shaped wrt the origin
// we suppose that the ridge ``simplex minus new_vertex'' is already present
template<typename Scalar>
bool still_star_shaped_after_extension(const Matrix<Scalar>& vertices,
                                       const face_type& simplex,
                                       int new_vertex)
{
   for (const auto& s : simplex) {
      if (s == new_vertex) continue;
      try {
         const Vector<Scalar> normal_vector = null_space(vertices.minor(simplex-scalar2set(s), All))[0]; 
         if (is_zero(normal_vector))
            throw std::runtime_error("Found zero normal vector");
         if (sign(normal_vector * vertices[s]) != sign(normal_vector[0]))
            // the affine span of (simplex - s) separates s from 0
            return false;
      } catch (degenerate_matrix) {
         // this occurs if the affine span of a ridge contains 0
         return false;
      }
   }
   return true;
}

// Along which boundary ridges can we extend a star-shaped simplicial ball and have it remain star-shaped?
// We store them as std::vectors so that we can later iterate over all their subsets via their indices, which is cheaper
template<typename Scalar>
void extensible_ridges_and_facets(const Matrix<Scalar>& vertices,
                                  const complex_type& candidate_ridges,
                                  const complex_type& current_ball,
                                  const Set<int>& current_support,
                                  const Map<face_type, std::vector<int>>& link_of_ridge,
                                  std::vector<face_type>& extensible_ridges,
                                  std::vector<face_type>& extensible_facets)
{
   for (const auto& cr : candidate_ridges) {
      const std::vector<int>& link(link_of_ridge[cr]);
      if (link.size() < 2) continue; // the ridge is not on the global boundary
      
      if (current_support.contains(link[0]) &&
          current_support.contains(link[1])) { // all vertices of the neighboring simplex are already in the ball, so just add it
         for (const auto& l : link) {
            const face_type new_simplex(cr + scalar2set(l));
            if (!current_ball.contains(new_simplex)) {
               extensible_ridges.push_back(cr);
               extensible_facets.push_back(new_simplex);
               break; // this can only happen once, as one element of the star of cr is already in the ball
            }
         }
      } else { // one vertex in the link is outside the ball, so we have to test for star-shapedness
         const int v = current_support.contains(link[0]) 
            // Of the two vertices in the link, which one is outside the current support?
            ? link[1]
            : link[0];
         if (still_star_shaped_after_extension(vertices, cr + scalar2set(v), v)) {
            extensible_ridges.push_back(cr);
            extensible_facets.push_back(cr + scalar2set(v));
         }
      }
   }
}

// update the boundary and the candidate ridges with the ridges of new_facet
void update_boundary_ridges(const face_type& new_facet,
                            const Map<face_type, std::vector<int>>& link_of_ridge,
                            complex_type& new_boundary,
                            complex_type& new_candidate_ridges)
{
   for (Entire<Subsets_less_1<const face_type&>>::const_iterator rit = entire(all_subsets_less_1(new_facet)); !rit.at_end(); ++rit) {
      const face_type ridge(*rit);
      if (new_boundary.contains(ridge)) {
         new_boundary -= ridge;
         new_candidate_ridges -= ridge;
      } else {
         new_boundary += ridge;
         if (link_of_ridge[ridge].size() == 2)
            new_candidate_ridges += ridge;
      }
   }
}

template<typename Scalar>
void enumerate_star_shaped_balls(const Matrix<Scalar>& vertices,
                                 const complex_type& current_ball,        // simplices in the current ball B
                                 const complex_type& current_boundary,    // all ridges in the boundary of B
                                 const complex_type& candidate_ridges,    // for extension across them
                                 const face_type& current_support,
                                 const Map<face_type, std::vector<int>>& link_of_ridge,
                                 ballhash_type &balls_processed)
{
   std::vector<face_type> extensible_ridges, extensible_facets;
   extensible_ridges_and_facets(vertices, candidate_ridges, current_ball, current_support, link_of_ridge, 
                                extensible_ridges, extensible_facets);

   // iterate over all ridges that can be extended, add the simplex on the other side and recurse
   for (unsigned int i=0; i<extensible_ridges.size(); ++i) {
      const complex_type new_ball(current_ball + extensible_facets[i]);
      const face_type new_support(current_support + extensible_facets[i]);
      complex_type new_boundary(current_boundary);
      complex_type new_candidate_ridges(candidate_ridges);
      update_boundary_ridges(extensible_facets[i], link_of_ridge, new_boundary, new_candidate_ridges);
      if (!balls_processed.exists(new_ball)) {
         balls_processed += new_ball;
         // now recurse
         enumerate_star_shaped_balls(vertices, new_ball, new_boundary, new_candidate_ridges, new_support, link_of_ridge, balls_processed);
      }
   }
}
   
} // end anonymous namespace

template<typename Scalar>  
Array<complex_type> star_shaped_balls(perl::Object triangulation)
{
   const Array<face_type> facets = triangulation.give("FACETS");
   const Matrix<Scalar> _vertices = triangulation.give("COORDINATES");
   Array<int> vertex_indices;
   Matrix<Scalar> vertices;
   const bool must_rename = (triangulation.lookup("VERTEX_INDICES") >> vertex_indices);
   if (must_rename)
      vertices = ones_vector<Scalar>(vertex_indices.size()) | _vertices.minor(vertex_indices, All);
   else
      vertices = ones_vector<Scalar>(_vertices.rows()) | _vertices; // we work with homogeneous coordinates

   perl::Object HD_obj = triangulation.give("HASSE_DIAGRAM");
   const graph::Lattice<graph::lattice::BasicDecoration> HD(HD_obj);
   const Map<face_type, std::vector<int>> link_of_ridge = links_of_ridges(HD);

   const complex_type st0(star_of_zero_impl(vertices, facets));
   complex_type current_ball(st0);

   face_type current_support;
   for (const auto& b : current_ball)
      current_support += b;

   const complex_type boundary_of_star = boundary_of(st0);
   ballhash_type balls_processed;
   balls_processed += current_ball;
   balls_processed += complex_type();

   enumerate_star_shaped_balls(vertices, current_ball, boundary_of_star, boundary_of_star, current_support, link_of_ridge, balls_processed);

   if (!must_rename) return Array<complex_type>(balls_processed.size(), entire(balls_processed));

   Array<complex_type> ssb(balls_processed.size());
   Entire<Array<complex_type>>::iterator oit = entire(ssb);
   for (Entire<ballhash_type>::const_iterator iit = entire(balls_processed); !iit.at_end(); ++iit, ++oit) {
      complex_type ball;
      for (const auto& s : *iit)
         ball += permuted_inv(s, vertex_indices);
      *oit = ball;
   }
   return ssb;
}

template<typename Scalar>  
complex_type star_of_zero(perl::Object triangulation)
{
   const Array<face_type> facets = triangulation.give("FACETS");
   const Matrix<Scalar> _vertices = triangulation.give("COORDINATES");
   Array<int> vertex_indices;
   Matrix<Scalar> vertices;
   const bool must_rename = (triangulation.lookup("VERTEX_INDICES") >> vertex_indices);
   if (must_rename)
      vertices = ones_vector<Scalar>(vertex_indices.size()) | _vertices.minor(vertex_indices, All);
   else
      vertices = ones_vector<Scalar>(_vertices.rows()) | _vertices; // we work with homogeneous coordinates

   const complex_type ssz(star_of_zero_impl(vertices, facets));
   if (!must_rename) return ssz;

   complex_type output;
   for (Entire<complex_type>::const_iterator sit = entire(ssz); !sit.at_end(); ++sit)
      output += permuted_inv(*sit, vertex_indices);
   return output;
}


template<typename Container>
Graph<Directed> poset_by_inclusion(const Array<Container>& collection)
{
   const int m(collection.size());
   Graph<Directed> poset(m);
   for (int i=0; i<m-1; ++i) {
      for (int j=i+1; j<m; ++j) {
         const int result_of_compare=incl(collection[i], collection[j]);
         if (result_of_compare==-1) poset.edge(i,j);
         else if (result_of_compare==1) poset.edge(j,i);
      }
   }
   return poset;
}


UserFunctionTemplate4perl("# @category Other\n"
                          "# Enumerate all balls formed by the simplices of a geometric simplicial complex"
                          "# that are strictly star-shaped with respect to the origin."
                          "# The origin may be a vertex or not."
                          "# For details see Assarf, Joswig & Pfeifle:"
                          "# Webs of stars or how to triangulate sums of polytopes, to appear"
                          "# @param GeometricSimplicialComplex P"
                          "# @return Array<Set<Set>>",
                          "star_shaped_balls<Scalar>(GeometricSimplicialComplex<type_upgrade<Scalar>>)"); 

UserFunctionTemplate4perl("# @category Other\n"
                          "# Find the facets of the star of the origin in the simplicial complex."
                          "# The origin may be a vertex or not."
                          "# For details see Assarf, Joswig & Pfeifle:"
                          "# Webs of stars or how to triangulate sums of polytopes, to appear"
                          "# @param GeometricSimplicialComplex C"
                          "# @return Set<Set<Int>> ",
                          "star_of_zero<Scalar>(GeometricSimplicialComplex<type_upgrade<Scalar>>)");

UserFunctionTemplate4perl("# @category Other\n"
                          "# Construct the inclusion poset from a given container."
                          "# The elements of the container are interpreted as sets.  They define a poset"
                          "# by inclusion.  The function returns this poset encoded as a directed graph."
                          "# The direction is towards to larger sets.  All relations are encoded, not"
                          "# only the covering relations."
                          "# For details see Assarf, Joswig & Pfeifle:"
                          "# Webs of stars or how to triangulate sums of polytopes, to appear"
                          "# @param Array<T> P"
                          "# @return Graph<Directed>",
                          "poset_by_inclusion<T>(Array<T>)"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
