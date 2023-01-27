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
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename Faces>
std::enable_if_t<pm::isomorphic_to_container_of<Faces, Set<Int>>::value, bool>
is_subdivision(const Matrix<Rational>& verts, const Faces& subdiv, OptionSet options)
{
   const Int n_vertices = verts.rows();

   Set<Int> all_verts; //to test the union property

   for (auto face_i=entire(subdiv); !face_i.at_end(); ++face_i) {
      all_verts += *face_i;
      auto face_l=face_i;
      while (!(++face_l).at_end()) {
         //test the intersection property
         const Matrix<Rational> intersection=null_space(verts.minor((*face_i) * (*face_l), All)); //the (affine hull of) the intersection
         const Set<Int> rest = (*face_i) ^ (*face_l);
         //test if some other vertex of one of the two faces is contained in the affine hull
         for (auto j=entire(rest); !j.at_end(); ++j) {
            bool is_contained=true;
            for (auto k=entire(rows(intersection)); is_contained && !k.at_end(); ++k)
               if ((*k)*verts.row(*j)!=0) is_contained=false;
            if (is_contained) {
               //the intersection property is violated
               if (options["verbose"]) cout << "Improper intersection of  " << *face_i << " and " << *face_l << endl;
               return false;
            }
         }
      }
   }
  
   if (options["verbose"]) cout << "Intersection property satisfied."<< endl;

   //test the union property
   if (all_verts.size()==n_vertices) {
      if (options["verbose"]) cout << "Union property satisfied."<< endl;
      return true;
   }

   const Set<Int> missing = sequence(0, n_vertices) - all_verts;

   Set<Int> non_vertices;
   if (options["interior_points"] >> non_vertices) {
      const Set<Int> fail = missing - non_vertices;
      if (fail.empty()) {
         if (options["verbose"]) cout << "Union property satisfied."<< endl;
         return true;
      }
      if (options["verbose"]) cout << "Vertices " << fail << " are not used in the subdivision." << endl;
      return false;
   }

   BigObject p("Polytope<Rational>", "POINTS", verts.minor(all_verts, All));

   const Matrix<Rational> eq=p.give("AFFINE_HULL");

   for (auto i=entire(rows(eq)); !i.at_end(); ++i)
      for (auto j=entire(missing); !j.at_end(); ++j)
         if ((*i)*(verts.row(*j))!=0) {
            if (options["verbose"]) cout << "Vertex " << *j << "is not used in the subdivision." << endl;
            return false;
         }


   const Matrix<Rational> ineqs=p.give("FACETS");
   for (auto i=entire(rows(ineqs)); !i.at_end(); ++i)
      for (auto j=entire(missing); !j.at_end(); ++j)
         if ((*i)*(verts.row(*j))<0) {
            if (options["verbose"]) cout <<  "Vertex " << *j << " is not used in the subdivision." << endl;
            return false;
         }


   if (options["verbose"]) cout << "Union property satisfied."<< endl;
   return true;
}

bool is_subdivision(const Matrix<Rational>& verts, const IncidenceMatrix<>& subdiv, OptionSet options)
{
   return is_subdivision(verts, rows(subdiv), options);
}

template <typename UnorderedFaces>
std::enable_if_t<(std::is_constructible<IncidenceMatrix<>, UnorderedFaces>::value &&
                  !pm::isomorphic_to_container_of<UnorderedFaces, Set<Int>>::value), bool>
is_subdivision(const Matrix<Rational>& verts, const UnorderedFaces& subdiv, OptionSet options)
{
   return is_subdivision(verts, IncidenceMatrix<>(subdiv), options);
}

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# @author Sven Herrmann"
                          "# Checks whether the union of the convex hulls of //faces// cover the entire set of //points// and no point lies in the interior of more than a face,"
                          "# where //points// is a set of points, and //faces// is a collection of subsets of (indices of) //points//."
                          "# It doesn't check if the intersection of faces is a proper face or if the faces covers the entire"
                          "# convex hull of the set of //points//. "
                          "# If the set of interior points of //points// is known, this set can be passed by assigning"
                          "# it to the option //interior_points//. If //points// are in convex position"
                          "# (i.e., if they are vertices of a polytope),"
                          "# the option //interior_points// should be set to [ ] (the empty set)."
                          "# @param Matrix points"
                          "# @param Array<Set<Int>> faces"
                          "# @option Set<Int> interior_points"
                          "# @example Two potential subdivisions of the square without inner points:"
                          "# > $points = cube(2)->VERTICES;"
                          "# > print is_subdivision($points,[[0,1,3],[1,2,3]],interior_points=>[ ]);"
                          "# | true"
                          "# > print is_subdivision($points,[[0,1,2],[1,2]],interior_points=>[ ]);"
                          "# | false"
                          "# @example Three points in RR^1"
                          "# > $points = new Matrix([[1,0],[1,1],[1,2]]);"
                          "# > print is_subdivision($points, [[0,2]]);"
                          "# | true"
                          "# > print is_subdivision($points, [[0,1]]);"
                          "# | false",
                          "is_subdivision(Matrix,*; {verbose => undef, interior_points => undef})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
