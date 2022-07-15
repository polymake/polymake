/* Copyright (c) 1997-2022
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
#include "polymake/Rational.h"
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Integer.h"
#include "polymake/Array.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Bitset.h"
#include "polymake/hash_map"
#include "polymake/vector"
#include "polymake/pair.h"
#include "polymake/numerical_functions.h"
#include <algorithm>

namespace polymake { namespace polytope {

namespace {

//calculates the legal edges, i.e. the edges containing no other points in their interior
template <typename Scalar>
IncidenceMatrix<> calc_illegal_edges(const Array< Vector<Scalar> >& points)
{
   const Int n = points.size();
   IncidenceMatrix<> illegal_edges(n, n);
   typedef typename pm::algebraic_traits<Scalar>::field_type field;
   for (Int i = 0; i < n; ++i)
      for (Int j = i+2; j < n; ++j)
         for (Int k = i+1; k < j; ++k) {
            field x_ik = points[k][1]-points[i][1];
            field x_ij = points[j][1]-points[i][1];
            field y_ik = points[k][2]-points[i][2];
            field y_ij = points[j][2]-points[i][2];
            if (((0==x_ik) && (0==x_ij)) || ((0!=x_ik) && (0!=x_ij) && y_ik/x_ik==y_ij/x_ij))
               illegal_edges(i,j) = 1;
         }
   return illegal_edges;
}

//container class for precalculated pieces of information about an edge
struct EdgeData {
   Bitset adv1, adv2, vis;
   bool monotony;
   bool trivial;
   EdgeData() : monotony(false), trivial(true) {}
   EdgeData(const Bitset& a1, const Bitset& a2, const Bitset& v, const bool& m) : adv1(a1), adv2(a2), vis(v), monotony(m), trivial(false) {}
};

//calculates the following information for all edges: possible advances, upper visible points to the right of the left endpoint, monotony
template <typename Scalar>
Array<Array<EdgeData>> edge_precalc(const Array<Vector<Scalar>>& points)
{
   typedef typename pm::algebraic_traits<Scalar>::field_type field;
   const Int n = points.size();
   Array<Array<EdgeData > > edges(n-2);
   for (Int i = 0; i < n-2; ++i) {
      Array<EdgeData > tmp(n-i-1);
      edges[i] = tmp;
   }
   Matrix<field> V(3, 3);
   bool candidate = true;
   BigObject::Schedule s;
   const IncidenceMatrix<>& illegal_edges = calc_illegal_edges<Scalar>(points);
   for (Int i = 0; i < n-2; ++i) {
      for (Int j = i+1; j < n; ++j) {
         if (0==illegal_edges(i,j)) {
            Bitset adv_type1(j-i-1);
            Bitset adv_type2(j-i-1);
            Bitset vis_from_above(n-i-2);
            for (Int k = i+1; k < n; ++k) {
               if (k!=j && (0==illegal_edges(i,k)) && (0==illegal_edges(j,k))) {
                  V.row(0) = points[i];
                  V.row(1) = points[k];
                  V.row(2) = points[j];
                  BigObject p("Polytope", mlist<field>(), "VERTICES", V);
                  if (!s.valid())
                     s = p.call_method("get_schedule", "FACETS", "VERTICES_IN_FACETS");
                  s.apply(p);

                  const Matrix<field> facets = p.give("FACETS");
                  const IncidenceMatrix<> VIF = p.give("VERTICES_IN_FACETS");

                  //for each point in the segment wrt. the x-coordinate check if the corresponding triangle constitutes an advance triangle
                  if (k < j) {
                     candidate = true;
                     //the triangle is an advance triangle iff there is no other point of the input in its interior
                     for (Int l = i+1; l < j; ++l) {
                        if (l != k) {
                           Vector<field> eval = facets*points[l];
                           if (eval[0]>0 && eval[1]>0 && eval[2]>0)
                              candidate = false;
                        }
                     }
                  }
                  //calculate the index of the facet which contains the points i and j
                  Int index = 0;
                  for (Int l = 0; l < 3; ++l) {
                     if (VIF.exists(l,0) && VIF.exists(l,2))
                        index = l;
                  }
                  // check if the additional point satifies the visibility condition
                  // and determine the advance type in the case of constituting an advance triangle
                  if (facets(index,2)>0) {
                     if (k<j) {
                        if (candidate)
                           adv_type1 += k-i-1;
                        vis_from_above += k-i-1;
                     }
                     else {
                        vis_from_above += k-i-2;
                     }
                  }
                  else if (candidate && (k<j)) {
                     adv_type2 += k-i-1;
                  }
               }
            }
            EdgeData ed(adv_type1,adv_type2,vis_from_above, Rational(points[j][2]-points[i][2]) < 0 ? true : false);
            edges[i][j-i-1] = ed;
         }
      }
   }
   return edges;
}

//calculates the lower and the upper boundary for the input polytope (including all such points specified in POINTS)
template <typename Scalar>
Int lower_upper_boundary(Bitset& lower, Bitset& upper, BigObject p)
{
   typedef typename pm::algebraic_traits<Scalar>::field_type field;
   const Matrix<Scalar>& P = p.give("POINTS");
   const Int n = P.rows()-1;
   Set<Int> boundary_points;
   const IncidenceMatrix<> PIF = p.give("POINTS_IN_FACETS");
   const Matrix<field> F = p.give("FACETS");

   upper += n;
   for (auto rit = entire(rows(PIF)); !rit.at_end(); ++rit) {
      const Set<Int>& ps((*rit));	
      for (auto sit = entire(ps); !sit.at_end(); ++sit) {
         boundary_points+= *sit;
         if (F(rit.index(),2)>0 || P(n,1)==P(*sit,1))
            lower+= *sit;
         else
            upper+= *sit;
      }
   }
   return boundary_points.size();
}

}// end anonymous namespace

//calculate the number of triangulations in the input points
template <typename Scalar>
Integer n_fine_triangulations(const Matrix<Scalar>& points, OptionSet options)
{
   const bool opt(options["optimization"]);
   const Int n = points.rows();

   if (points.cols() != 3)
      throw std::runtime_error("this algorithm works for planar point configurations only");
   if (n < 3)
      throw std::runtime_error("insufficient number of points");

   Array<Vector<Scalar>> ordered_points(rows(points));
   std::sort(ordered_points.begin(), ordered_points.end(), operations::lex_less());

   //precalculations for the edges
   Array<Array<EdgeData>> advance_triangles = edge_precalc<Scalar>(ordered_points);
   Matrix<Scalar> ordered_points_matrix(n,points.cols(),entire(ordered_points));

   BigObject p("Polytope", mlist<Scalar>(),
               "FEASIBLE", true,
               "BOUNDED", true,
               "POINTS", ordered_points_matrix);

   //calculate the upper boundary and the source for an DAG
   const Int size = n+log2_ceil(n);
   Bitset upper_boundary(n);
   Bitset source(size);
   source+= size-1;
   const Int n_boundary_points = lower_upper_boundary<Scalar>(source, upper_boundary, p);

   hash_map<Bitset, Integer> unmap1;
   hash_map<Bitset, Integer> unmap2;
   hash_map<Bitset, Integer> *current_lvl = &unmap1, *next_lvl = &unmap2;
   Integer counter(1);
   unmap1[source] = counter;

   //while keeping at most two hashtables for consecutive levels in memory, calculate the number of triangulations for each level one after another
   for (Int i = 0; i < 2*n-n_boundary_points-2; ++i) {	
      for (auto chain = current_lvl->begin(); chain != current_lvl->end(); chain = current_lvl->erase(chain)) {
         Bitset current_mchain = (*chain).first;
         Bitset on_or_above(n);
         if (opt) {
            // calculate all points on or above the current chain and relate them to their visible edges
            on_or_above += 0;
            auto right_end = current_mchain.begin();
            ++right_end;
            for (Int j = 0; j != n-1; j = *right_end, ++right_end) {
               on_or_above += *right_end;
               for (Int l = j+1; l < *right_end; l++)
                  if (advance_triangles[j][*right_end-j-1].vis.contains(l-j-1))
                     on_or_above += l;
            }
         }
         // calculate the marking from the corresponding bitset
         Int mark = 0;
         auto leading_one = current_mchain.begin();
         while (*leading_one < n)
            ++leading_one;
         for (Int j = size-1; j >= *leading_one; --j) {
            if (current_mchain.contains(j)) {
               mark+= pow(2,size-j-1);
            }
         }
         Int left = 0, mid = 0, right = 0;
         // calculate the start nodes according to the mark of the chain
         auto unmarked = current_mchain.begin();
         for (Int j = 0; j+2 < mark; ++j) {
            ++unmarked;
         }
         mid = *unmarked;

         if (mark > 1) {
            unmarked++;
            right = *(unmarked);
            //decrement the mark in binary of the chain
            Int j = size-1;
            while (!current_mchain.contains(j)) {
              current_mchain += j;
              j--;
            }
            current_mchain -= j;
         }

         // check each edge in the current chain for legal successors
         do {
            left = mid;
            mid = right;
            auto legal_edge = current_mchain.begin();
            while (*legal_edge < mid+1)
               ++legal_edge;
            right = *legal_edge;
            // check for advances of type 2 which exclude a point from the chain
            if (mid > 0) {
               if (!advance_triangles[left][right-left-1].trivial && advance_triangles[left][right-left-1].adv2.contains(mid-left-1)) {
                  //check if the visibility condition is met for the advanced marked chain in question
                  current_mchain -= mid;
                  if (opt) {
                     on_or_above -= mid;
                     bool visible = true;
                     auto untreated = current_mchain.begin();
                     ++untreated;
                     Int tmp = 0;
                     for (Int m = 0; m < left && visible; m = tmp) {
                        tmp = *untreated;
                        untreated++;
                        if ((upper_boundary.contains(m) && upper_boundary.contains(tmp)) || advance_triangles[m][tmp-m-1].monotony)
                           continue;
                        else
                           if (upper_boundary.contains(tmp))
                              visible = false;
                           else
                              if (tmp < left && !(advance_triangles[tmp][*untreated-tmp-1].monotony))
                                 continue;
                              else {
                                 bool retrace = false;
                                 //check for a possible retrace
                                 for (Int s = tmp+1; s < n && !retrace; ++s)
                                    if (advance_triangles[m][tmp-m-1].vis.contains(s-m-2) && on_or_above.contains(s) &&
                                       advance_triangles[m][s-m-1].adv2.contains(tmp-m-1))
                                       retrace = true;
                                 if (retrace)
                                    continue;
                                 else
                                    visible = false;
                              }
                     }
                     if (visible) {
                        (*next_lvl)[current_mchain] += (*chain).second;
                        on_or_above += mid;
                     }
                  }
                  if (!opt)
                     (*next_lvl)[current_mchain] += (*chain).second;
                  current_mchain += mid;
               }
               //increment the mark in binary of the chain
               Int j = size-1;
               while (current_mchain.contains(j)) {
                  current_mchain -= j;
                  j--;
               }
               current_mchain += j;
            }

            //check for advances of type 1 which include a new point into the chain
            if (right-mid > 1) {
               for (Int j = mid+1; j < right; ++j) {
                  if (!advance_triangles[mid][right-mid-1].trivial && advance_triangles[mid][right-mid-1].adv1.contains(j-mid-1)) {
                     //check if the visibility condition is met
                     current_mchain += j;
                     if (opt) {
                        bool visible = true;
                        auto untreated = current_mchain.begin();
                        ++untreated;
                        Int tmp = 0;
                        for (Int m = 0; m < mid && visible; m = *untreated++) {
                           tmp = *untreated;
                           untreated++;
                           if ((upper_boundary.contains(m) && upper_boundary.contains(tmp)) || advance_triangles[m][tmp-m-1].monotony)
                              continue;
                           else
                              if (upper_boundary.contains(tmp))
                                 visible = false;
                              else
                                 if (tmp < mid && !(advance_triangles[tmp][*untreated-tmp-1].monotony))
                                    continue;
                                 else {
                                    bool retrace = false;
                                    //check for a possible retrace
                                    for (Int s = tmp+1; s < n && !retrace; ++s)
                                       if (advance_triangles[m][tmp-m-1].vis.contains(s-m-2) && on_or_above.contains(s) &&
                                          advance_triangles[m][s-m-1].adv2.contains(tmp-m-1))
                                          retrace = true;
                                    if (retrace)
                                       continue;
                                    else
                                       visible = false;
                                 }
                        }
                        if (visible)
                           (*next_lvl)[current_mchain] += (*chain).second;
                     }
                     if (!opt)
                        (*next_lvl)[current_mchain] += (*chain).second;
                     current_mchain -=j;
                  }
               }
            }
         } while (right < n-1);
      }
      std::swap(current_lvl,next_lvl);
   }
   //calculate the number of triangulations from the counters of the last iteration
   Integer tr(0);
   for (auto chain = current_lvl->begin(); chain != current_lvl->end(); ++chain) {
      tr += chain->second;
   }
   return tr;
}


UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Calculates the number of fine triangulations of a planar point configuration. This can be space intensive."
                          "# "
                          "# Victor Alvarez, Raimund Seidel:"
                          "# A Simple Aggregative Algorithm for Counting Triangulations of Planar Point Sets and Related Problems."
                          "# In Proc. of the 29th Symposium on Computational Geometry (SoCG '13), pages 1-8, Rio de Janeiro, Brazil, 2013"
                          "# "
                          "# @param Matrix M in the plane (homogeneous coordinates)"
                          "# @param Bool optimization defaults to 1, where 1 includes optimization and 0 excludes it"
                          "# @return Integer number of fine triangulations"
                          "# @example To print the number of possible fine triangulations of a square, do this:"
                          "# > print n_fine_triangulations(cube(2)->VERTICES);"
                          "# | 2","n_fine_triangulations(Matrix { optimization => 1})");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
