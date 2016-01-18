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
#include "polymake/Set.h"
#include "polymake/PowerSet.h"
#include "polymake/Array.h"
#include "polymake/Map.h"
#include "polymake/linalg.h"
#include "polymake/topaz/sum_triangulation_tools.h"
#include "polymake/polytope/beneath_beyond.h"
#include "polymake/polytope/to_interface.h"

namespace polymake { namespace topaz {

namespace {


template<typename Scalar>
Matrix<Scalar> facets_of_cone_over(Set<int> facet,
                                   const Matrix<Scalar>& vertices,
                                   int ioz)
{
   if (ioz >= 0) facet -= ioz;
   const Matrix<Scalar> cone(vertices.minor(facet, ~scalar2set(0)));
   polytope::beneath_beyond_algo<Scalar> algo(cone, false);
   algo.compute(entire(sequence(0, facet.size())));
   return zero_vector<Scalar>() | algo.getFacets();
}

template<typename Scalar>
Matrix<Scalar> ineqs2vertices(const Matrix<Scalar>& inequalities)
{
   polytope::beneath_beyond_algo<Scalar> algo(inequalities, false);
   algo.compute(entire(sequence(0, inequalities.rows())));
   return algo.getFacets();
}




template<typename Scalar>
Matrix<Scalar> inner_facet_normals(const Set<int>& facet,
                                   const Matrix<Scalar>& vertices)
{
   Matrix<Scalar> inner_facet_normals(facet.size(), vertices.cols());
   typename Entire<Rows<Matrix<Scalar> > >::iterator rit = entire(rows(inner_facet_normals));
   for (Entire<Set<int> >::const_iterator fit = entire(facet); !fit.at_end(); ++fit, ++rit) {
      const Vector<Scalar> normal = null_space(vertices.minor(facet - scalar2set(*fit), All))[0];
      if (sign(normal * vertices[*fit]) > 0)
         *rit = normal;
      else 
         *rit = -normal;
   }
   return inner_facet_normals;
}

template<typename Scalar>
std::vector<int> indices_of_normals_towards_0(const Set<int>& facet,
                                              const Matrix<Scalar>& inner_facet_normals)
{
   std::vector<int> indices_of_normals_towards_0;
   int i(0);
   for (typename Entire<Rows<Matrix<Scalar> > >::const_iterator nit = entire(rows(inner_facet_normals)); !nit.at_end(); ++nit, ++i) {
      if ((*nit)[0] > 0)
         indices_of_normals_towards_0.push_back(i);
   }
   return indices_of_normals_towards_0;
}


// return the list of indices of simplices that are greater than the one indexed by sigma
template<typename Scalar>
Set<int> ideal_of(int sigma,
                  const Matrix<Scalar>& vertices,
                  const Array<Set<int> >& simplices,
                  const Array<Matrix<Scalar> >& inner_facet_normals_of,
                  const Array<std::vector<int> >& indices_of_normals_towards_0_of,
                  int ioz)
{
   Set<int> ideal_of_sigma;
   const Matrix<Scalar> cone_facets = facets_of_cone_over(simplices[sigma], vertices, ioz);

   // check if zero is in sigma;
   bool zero_in_sigma = true;
   for (typename Entire<Rows<Matrix<Scalar> > >::const_iterator fit = entire(rows(inner_facet_normals_of[sigma])); !fit.at_end(); ++fit) {
      if((*fit)[0] < 0){
         zero_in_sigma = false;
         break;
      }
   }
   

   for (int tau=0; tau<simplices.size(); ++tau) {
      if (tau==sigma) continue;
      /*
        First Check if one of the two contains 0
        Then everything is easy as simplices containing 0 are the smallest ones
        They can be compared with every other simplex not containing 0
       */

      // check if zero is in tau
      bool zero_in_tau = true;
      for (typename Entire<Rows<Matrix<Scalar> > >::const_iterator fit = entire(rows(inner_facet_normals_of[tau])); !fit.at_end(); ++fit) {
         if((*fit)[0] < 0){
            zero_in_tau = false;
            break;
         }
      }

      if(zero_in_tau) continue;
      if(zero_in_sigma){
         ideal_of_sigma += tau;
         continue;
      }


      // Check if the affine hull of the intersection of sigma and tau contain
      // the origin. If this is the case sigma is not preceeding tau
      perl::Object p_sigma(perl::ObjectType::construct<Scalar>("polytope::Polytope"));
      p_sigma.take("FACETS") << inner_facet_normals_of[sigma];

      perl::Object p_tau(perl::ObjectType::construct<Scalar>("polytope::Polytope"));
      p_tau.take("FACETS") << inner_facet_normals_of[tau];

      perl::Object intersection = CallPolymakeFunction("polytope::intersection", p_sigma, p_tau);
      Matrix<Scalar> affine_hull = intersection.give("AFFINE_HULL");
      bool sigma_tau_intersect = intersection.give("FEASIBLE");
      bool zero_in_affine_hull = true;

      for (typename Entire<Rows<Matrix<Scalar> > >::const_iterator fit = entire(rows(affine_hull)); !fit.at_end(); ++fit) {
         if((*fit)[0] != 0){
            zero_in_affine_hull = false;
            break;
         }
      }
      
      if(zero_in_affine_hull && sigma_tau_intersect) continue;

      /*
        Take the intersection of tau and the cone over sigma.
        (if this is empty no stabbing ray exists)
        Take a relative interior point x in that intersection.
        If the line segment between that point and 0:
          - has no intersection with sigma, then sigma does not preceed tau
          - has a 1 dimensional intersection with sigma, then sigma preceeds tau
          - intersects sigma in only one point then 
            sigma preceeds tau iff that point is not equal to x
       */
      Matrix<Scalar> intersection_ineqs = cone_facets / inner_facet_normals_of[tau];
      intersection = perl::Object(perl::ObjectType::construct<Scalar>("polytope::Polytope"));
      intersection.take("INEQUALITIES") << intersection_ineqs;
      bool feasible = intersection.give("FEASIBLE");
      if(!feasible) continue;
      
      Vector<Scalar> rel_int_p = intersection.give("REL_INT_POINT");

      Matrix<Scalar> segment_verts = unit_vector<Scalar>(rel_int_p.dim(),0) / rel_int_p;
      perl::Object segment(perl::ObjectType::construct<Scalar>("polytope::Polytope"));
      segment.take("VERTICES") << segment_verts;

      intersection = CallPolymakeFunction("polytope::intersection", p_sigma, segment);

      // check dimension of intersection. It is either -1, 0 or 1
      // hence CONE_DIM is either 0, 1 or 2
      int dim_inter = intersection.give("CONE_DIM");

      if(dim_inter == 0) continue;

      if(dim_inter == 2){
         ideal_of_sigma += tau;
         continue;
      }

      // intersection is just one point since 0 dimensional
      // hence the vertices is precisely one point
      Matrix<Scalar> inter_p = intersection.give("VERTICES");
      if(inter_p.row(0) == rel_int_p) continue;

      ideal_of_sigma +=tau;
   }

   return ideal_of_sigma;
}

} // end anonymous namespace


template<typename Scalar>  
Graph<Directed> stabbing_order(perl::Object triangulation)
{
   const Array<Set<int> > simplices = triangulation.give("FACETS");
   const Matrix<Scalar> _vertices = triangulation.give("COORDINATES");
   Array<int> vertex_indices;
   Matrix<Scalar> vertices;
   const bool must_rename = (triangulation.lookup("VERTEX_INDICES") >> vertex_indices);
   if (must_rename)
      vertices = ones_vector<Scalar>(vertex_indices.size()) | _vertices.minor(vertex_indices, All);
   else
      vertices = ones_vector<Scalar>(_vertices.rows()) | _vertices; // we work with homogeneous coordinates

   const int ioz = topaz::index_of_zero(vertices);

   Array<std::vector<int> > indices_of_normals_towards_0_of(simplices.size());
   Array<Matrix<Scalar> > inner_facet_normals_of(simplices.size());

   for (int i=0; i<simplices.size(); ++i) {
      inner_facet_normals_of[i] = inner_facet_normals(simplices[i], vertices);
      indices_of_normals_towards_0_of[i] = indices_of_normals_towards_0(simplices[i], inner_facet_normals_of[i]);
   }

   Graph<Directed> stabbing_order(simplices.size());
   for (int sigma=0; sigma<simplices.size(); ++sigma) {
      const Set<int> ideal_of_sigma = ideal_of(sigma, vertices, simplices, inner_facet_normals_of, indices_of_normals_towards_0_of, ioz);
      for (Entire<Set<int> >::const_iterator iit = entire(ideal_of_sigma); !iit.at_end(); ++iit)
         stabbing_order.edge(sigma, *iit);
   }
   // if 0 is not a point of the configuration, we're done
   if (ioz < 0) return stabbing_order;

   // We remove all edges between simplices in the star of 0
   Map<Set<int>, int> index_of;
   std::vector<int> st_0_indices;
   int index(-1);
   for (Entire<Array<Set<int> > >::const_iterator ait = entire(simplices); !ait.at_end(); ++ait) {
      index_of[*ait] = ++index;
      if ((*ait).contains(ioz))
         st_0_indices.push_back(index);
   }

   // if 0 is not a vertex of the triangulation, we're done   
   if (!st_0_indices.size()) return stabbing_order;

   for (Entire< Subsets_of_k<const sequence&> >::const_iterator i=entire(all_subsets_of_k(sequence(0, st_0_indices.size()),2)); !i.at_end(); ++i) {
      const Set<int> pair(*i);
      stabbing_order.delete_edge(st_0_indices[pair.front()],
                                 st_0_indices[pair.back() ]);
   }
   return stabbing_order;
}

InsertEmbeddedRule("REQUIRE_APPLICATION polytope\n\n");

UserFunctionTemplate4perl("# @category Other"
                          "# Determine the stabbing partial order of a simplicial ball with respect to the origin."
                          "# The origin may be a vertex or not."
                          "# For details see Assarf, Joswig & Pfeifle:"
                          "# Webs of stars or how to triangulate sums of polytopes, to appear"
                          "# @param GeometricSimplicialComplex P"
                          "# @return graph::Graph<Directed>",
                          "stabbing_order<Scalar>(GeometricSimplicialComplex<type_upgrade<Scalar>>)"); 

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
