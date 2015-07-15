/* Copyright (c) 1997-2015
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
#include "polymake/graph/HasseDiagram.h"
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/linalg.h"
#include "polymake/topaz/sum_triangulation_tools.h"

namespace polymake { namespace topaz {

namespace {

// Decide whether a simplex is star-shaped wrt the origin
// we suppose that the ridge ``simplex minus new_vertex'' is already present
template<typename Scalar>
bool still_star_shaped_after_extension(const Matrix<Scalar>& vertices,
                                       const Set<int>& simplex,
                                       int new_vertex)
{
   for (Entire<Set<int> >::const_iterator sit = entire(simplex); !sit.at_end(); ++sit) {
      if (*sit == new_vertex) continue;
      try {
         const Vector<Scalar> normal_vector = null_space(vertices.minor(simplex-scalar2set(*sit), All))[0]; 
         if (normal_vector == zero_vector<Scalar>(vertices.cols()+1))
            throw std::runtime_error("Found zero normal vector");
         if (sign(normal_vector * vertices[*sit]) != sign(normal_vector[0]))
            // the affine span of (simplex - *sit) separates *sit from 0
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
                                  const Set<Set<int> >& candidate_ridges,
                                  const Set<Set<int> >& current_ball,
                                  const Set<int>& current_support,
                                  const Map<Set<int>, std::vector<int> >& link_of_ridge,
                                  std::vector<Set<int> >& extensible_ridges,
                                  std::vector<Set<int> >& extensible_facets)
{
   for (Entire<Set<Set<int> > >::const_iterator cit = entire(candidate_ridges); !cit.at_end(); ++cit) {
      const std::vector<int>& link(link_of_ridge[*cit]);
      if (link.size() < 2) continue; // the ridge is not on the global boundary
      
      if (current_support.contains(link[0]) &&
          current_support.contains(link[1])) { // all vertices of the neighboring simplex are already in the ball, so just add it
         for (Entire<std::vector<int> >::const_iterator lit = entire(link); !lit.at_end(); ++lit) {
            const Set<int> new_simplex(*cit + scalar2set(*lit));
            if (!current_ball.contains(new_simplex)) {
               extensible_ridges.push_back(*cit);
               extensible_facets.push_back(new_simplex);
               break; // this can only happen once, as one element of the star of *cit is already in the ball
            }
         }
      } else { // one vertex in the link is outside the ball, so we have to test for star-shapedness
         const int v = current_support.contains(link[0]) 
            // Of the two vertices in the link, which one is outside the current support?
            ? link[1]
            : link[0];
         if (still_star_shaped_after_extension(vertices, *cit + scalar2set(v), v)) {
            extensible_ridges.push_back(*cit);
            extensible_facets.push_back(*cit + scalar2set(v));
         }
      }
   }
}

// update the boundary and the candidate ridges with the ridges of new_facet
void update_boundary_ridges(const Set<int>& new_facet,
                            const Map<Set<int>, std::vector<int> >& link_of_ridge,
                            Set<Set<int> >& new_boundary,
                            Set<Set<int> >& new_candidate_ridges)
{
   for (Entire<Subsets_less_1<const Set<int>&> >::const_iterator rit = entire(all_subsets_less_1(new_facet)); !rit.at_end(); ++rit) {
      const Set<int> ridge(*rit);
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
                                 const Set<Set<int> >& current_ball,        // simplices in the current ball B
                                 const Set<Set<int> >& current_boundary,    // all ridges in the boundary of B
                                 const Set<Set<int> >& candidate_ridges,    // for extension across them
                                 const Set<int>& current_support,
                                 const Map<Set<int>, std::vector<int> >& link_of_ridge,
                                 Set<Set<Set<int> > > &star_shaped_balls)
{
   std::vector<Set<int> > extensible_ridges, extensible_facets;
   extensible_ridges_and_facets(vertices, candidate_ridges, current_ball, current_support, link_of_ridge, 
                                extensible_ridges, extensible_facets);

   // iterate over all ridges that can be extended, add the simplex on the other side and recurse
   for (unsigned int i=0; i<extensible_ridges.size(); ++i) {
      const Set<Set<int> > new_ball(current_ball + extensible_facets[i]);
      const Set<int> new_support(current_support + extensible_facets[i]);
      Set<Set<int> > new_boundary(current_boundary);
      Set<Set<int> > new_candidate_ridges(candidate_ridges);
      update_boundary_ridges(extensible_facets[i], link_of_ridge, new_boundary, new_candidate_ridges);
      star_shaped_balls += new_ball;

      // now recurse
      enumerate_star_shaped_balls(vertices, new_ball, new_boundary, new_candidate_ridges, new_support, link_of_ridge, star_shaped_balls);
   }
}
   
} // end anonymous namespace

template<typename Scalar>  
Array<Set<Set<int> > > star_shaped_balls(perl::Object triangulation)
{
   const Array<Set<int> > facets = triangulation.give("FACETS");
   const Matrix<Scalar> _vertices = triangulation.give("COORDINATES");
   Array<int> vertex_indices;
   Matrix<Scalar> vertices;
   const bool must_rename = (triangulation.lookup("VERTEX_INDICES") >> vertex_indices);
   if (must_rename)
      vertices = ones_vector<Scalar>(vertex_indices.size()) | _vertices.minor(vertex_indices, All);
   else
      vertices = ones_vector<Scalar>(_vertices.rows()) | _vertices; // we work with homogeneous coordinates

   const graph::HasseDiagram HD = triangulation.give("HASSE_DIAGRAM");
   const Map<Set<int>, std::vector<int> > link_of_ridge = links_of_ridges(HD);

   const Set<Set<int> > st0(star_of_zero_impl(vertices, facets));
   Set<Set<int> > current_ball(st0);

   Set<int> current_support;
   for (Entire<Set<Set<int> > >::const_iterator bit = entire(current_ball); !bit.at_end(); ++bit)
      current_support += *bit;

   const Set<Set<int> > boundary_of_star = boundary_of(st0);
   Set<Set<Set<int> > > star_shaped_balls;
   star_shaped_balls += current_ball;
   star_shaped_balls += Set<Set<int> >();

   enumerate_star_shaped_balls(vertices, current_ball, boundary_of_star, boundary_of_star, current_support, link_of_ridge, star_shaped_balls);

   if (!must_rename) return Array<Set<Set<int> > >(star_shaped_balls.size(), entire(star_shaped_balls));

   Array<Set<Set<int> > > ssb(star_shaped_balls.size());
   Entire<Array<Set<Set<int> > > >::iterator oit = entire(ssb);
   for (Entire<Set<Set<Set<int> > > >::const_iterator iit = entire(star_shaped_balls); !iit.at_end(); ++iit, ++oit) {
      Set<Set<int> > ball;
      for (Entire<Set<Set<int> > >::const_iterator sit = entire(*iit); !sit.at_end(); ++sit)
         ball += permuted_inv(*sit, vertex_indices);
      *oit = ball;
   }
   return ssb;
}

template<typename Scalar>  
Set<Set<int> > star_of_zero(perl::Object triangulation)
{
   const Array<Set<int> > facets = triangulation.give("FACETS");
   const Matrix<Scalar> _vertices = triangulation.give("COORDINATES");
   Array<int> vertex_indices;
   Matrix<Scalar> vertices;
   const bool must_rename = (triangulation.lookup("VERTEX_INDICES") >> vertex_indices);
   if (must_rename)
      vertices = ones_vector<Scalar>(vertex_indices.size()) | _vertices.minor(vertex_indices, All);
   else
      vertices = ones_vector<Scalar>(_vertices.rows()) | _vertices; // we work with homogeneous coordinates

   const Set<Set<int> > ssz(star_of_zero_impl(vertices, facets));
   if (!must_rename) return ssz;

   Set<Set<int> > output;
   for (Entire<Set<Set<int> > >::const_iterator sit = entire(ssz); !sit.at_end(); ++sit)
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
         if (incl(collection[i], collection[j])<0) poset.edge(i,j);
         if (incl(collection[j], collection[i])<0) poset.edge(j,i);
      }
   }
   return poset;
}


UserFunctionTemplate4perl("# @category Producing a new simplicial complex from others\n"
                          "# Enumerate all balls formed by the simplices of a geometric simplicial complex that are"
                          "# star-shaped with respect to the origin"
                          "# @param GeometricSimplicialComplex P"
                          "# @return Array<Set<Set>>",
                          "star_shaped_balls<Scalar>(GeometricSimplicialComplex<type_upgrade<Scalar>>)"); 

UserFunctionTemplate4perl("# @category Producing a new simplicial complex from others\n"
                          "# Find the facets of the star of the vertex 0 in the simplicial complex"
                          "# @param GeometricSimplicialComplex C"
                          "# @return Set<Set<Int>> ",
                          "star_of_zero<Scalar>(GeometricSimplicialComplex<type_upgrade<Scalar>>)");

UserFunctionTemplate4perl("# @category Producing a new simplicial complex from others\n"
                          "# Construct the poset w.r.t. inclusion from the given container"
                          "# @param Array<T> P"
                          "# @return Graph<Directed>",
                          "poset_by_inclusion<T>(Array<T>)"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
