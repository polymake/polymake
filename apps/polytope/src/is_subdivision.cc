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
#include "polymake/Set.h"
#include "polymake/Matrix.h"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/linalg.h"

namespace polymake { namespace polytope {

template <typename Faces>
typename pm::enable_if<bool, pm::isomorphic_to_container_of<Faces, Set<int> >::value>::type
is_subdivision(const Matrix<Rational>& verts, const Faces& subdiv, perl::OptionSet options)
{
   const int n_vertices=verts.rows();

   Set<int> all_verts; //to test the union property

   for (typename Entire<Faces>::const_iterator face_i=entire(subdiv); !face_i.at_end(); ++face_i) {
      all_verts += *face_i;
      typename Entire<Faces>::const_iterator face_l=face_i;
      while (!(++face_l).at_end()) {
         //test the intersection property
         const Matrix<Rational> intersection=null_space(verts.minor((*face_i) * (*face_l), All)); //the (affine hull of) the intersection
         const Set<int> rest=(*face_i) ^ (*face_l);
         //test if some other vertex of one of the two faces is contained in the affine hull
         for (Entire< Set<int> >::const_iterator j=entire(rest); !j.at_end(); ++j) {
            bool is_contained=true;
            for (Entire< Rows < Matrix<Rational> > >::const_iterator k=entire(rows(intersection)); is_contained && !k.at_end(); ++k)
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

   const Set<int> missing=sequence(0,n_vertices)-all_verts;

   Set<int> non_vertices;
   if (options["interior_points"]>>non_vertices) {
      const Set<int> fail=missing-non_vertices;
      if (fail.empty()) {
         if (options["verbose"]) cout << "Union property satisfied."<< endl;
         return true;
      }
      if (options["verbose"]) cout << "Vertices " << fail << " are not used in the subdivision." << endl;
      return false;
   }

   perl::Object p("Polytope<Rational>");
   p.take("POINTS") << verts.minor(all_verts,All);

   const Matrix<Rational> eq=p.give("AFFINE_HULL");

   for (Entire< Rows < Matrix<Rational> > >::const_iterator i=entire(rows(eq)); !i.at_end(); ++i)
      for (Entire<Set <int> >::const_iterator j=entire(missing); !j.at_end(); ++j)
         if ((*i)*(verts.row(*j))!=0) {
            if (options["verbose"]) cout << "Vertex " << *j << "is not used in the subdivision." << endl;
            return false;
         }


   const Matrix<Rational> ineqs=p.give("FACETS");
   for (Entire< Rows < Matrix<Rational> > >::const_iterator i=entire(rows(ineqs)); !i.at_end(); ++i)
      for (Entire<Set <int> >::const_iterator j=entire(missing); !j.at_end(); ++j)
         if ((*i)*(verts.row(*j))<0) {
            if (options["verbose"]) cout <<  "Vertex " << *j << " is not used in the subdivision." << endl;
            return false;
         }


   if (options["verbose"]) cout << "Union property satisfied."<< endl;
   return true;
}

bool is_subdivision(const Matrix<Rational>& verts, const IncidenceMatrix<>& subdiv, perl::OptionSet options)
{
   return is_subdivision(verts, rows(subdiv), options);
}

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# Checks whether //faces// forms a valid subdivision of //points//, where //points//"
                          "# is a set of points, and //faces// is a collection of subsets of (indices of) //points//."
                          "# If the set of interior points of //points// is known, this set can be passed by assigning"
                          "# it to the option //interior_points//. If //points// are in convex position"
                          "# (i.e., if they are vertices of a polytope),"
                          "# the option //interior_points// should be set to [ ] (the empty set)."
                          "# @param Matrix points"
                          "# @param Array<Set<Int>> faces"
                          "# @option Set<Int> interior_points"
                          "# @author Sven Herrmann",
                          "is_subdivision(Matrix,*; {verbose => undef, interior_points => undef})");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
