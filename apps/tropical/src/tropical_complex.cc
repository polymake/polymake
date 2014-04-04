/* Copyright (c) 1997-2014
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
#include "polymake/Rational.h"
#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/hash_map"

namespace polymake { namespace tropical {


template <typename Coord>
perl::Object tropical_intersection(const Array<perl::Object>& pc_in)
{
   const int n_pc=pc_in.size();
   
   perl::ObjectType t=perl::ObjectType::construct<Coord>("fan::PolyhedralComplex");
   perl::Object pc_out(t);
   pc_out=pc_in[0];

   const int d=pc_in[0].give("FAN_DIM");

   for(int i=1;i<n_pc;++i)
   {
      const Array<Set<int> > max_polytopes1=pc_out.give("MAXIMAL_POLYTOPES");
      const Matrix<Coord> points1=pc_out.give("VERTICES");
      const Array<Set<int> > max_polytopes2=pc_in[i].give("MAXIMAL_POLYTOPES");
      const Matrix<Coord> points2=pc_in[i].give("VERTICES");
      
      ListMatrix<Vector<Coord> > points = points1/points2;
      hash_map<Vector<Coord>, int> point_map;
      int index=0;
      for (typename Entire<Rows<ListMatrix<Vector<Coord> > > >::const_iterator i=entire(rows(points)); !i.at_end(); ++i)
         point_map[*i]=index++;
      std::list<Set<int> > new_max_polytopes;
      perl::ObjectType polytope_type=perl::ObjectType::construct<Coord>("polytope::Polytope");
      

      Array<perl::Object> all_polytopes1(max_polytopes1.size());
      for (int i=0; i<max_polytopes1.size(); ++i) {
         all_polytopes1[i].create_new(polytope_type);
         const Matrix<Coord> p1_polytope_vert=points1.minor(max_polytopes1[i],All);
         all_polytopes1[i].take("POINTS")<<p1_polytope_vert;
      }
      Array<perl::Object> all_polytopes2(max_polytopes2.size());
      for (int i=0; i<max_polytopes2.size(); ++i) {
         all_polytopes2[i].create_new(polytope_type);
         const Matrix<Coord> p1_polytope_vert=points2.minor(max_polytopes2[i],All);
         all_polytopes2[i].take("POINTS")<<p1_polytope_vert;
      }

      for (Entire<Array<perl::Object> >::iterator i1=entire(all_polytopes1); !i1.at_end(); ++i1) {
         for (Entire<Array<perl::Object> >::iterator i2=entire(all_polytopes2); !i2.at_end(); ++i2) {
            perl::Object inters=CallPolymakeFunction("polytope::intersection", *i1, *i2);
            const int inters_dim=inters.give("CONE_DIM");
            if (inters_dim==d) {
               const Matrix<Coord> inters_points=inters.give("VERTICES");
               Array<int> point_indices(inters_points.rows());
               int index=0;
               for (typename Entire<Rows<Matrix<Coord> > >::const_iterator i=entire(rows(inters_points)); !i.at_end(); ++i,++index) {
                  
                  const Vector<Coord> point=*i;
                  const typename hash_map<Vector<Coord>,int>::iterator rep=point_map.find(point);
                  if (rep!=point_map.end()) {
                     point_indices[index]=rep->second;
                  } else {
                     point_indices[index]=points.rows();
                     points/=point;
                  }
               }
               Set<int> new_polytope;
               for (Entire<sequence>::const_iterator j=entire(sequence(0,index)); !j.at_end(); ++j)
                  if (point_indices[*j]>=0) new_polytope.insert(point_indices[*j]);
               new_max_polytopes.push_back(new_polytope);
            }
         }
      }
      
      pc_out.create_new(t);
      pc_out.take("FAN_DIM")<<d;
      pc_out.take("POINTS")<<points;
      pc_out.take("INPUT_POLYTOPES")<<new_max_polytopes;
   }
   
   return pc_out;
}

template <typename Coord>
perl::Object tropical_complex(const Matrix<Coord> &tropical_input_points)
{
   const int d=tropical_input_points.cols();
   const int n_points=tropical_input_points.rows();

      const Matrix<Coord> points = ones_vector<Coord>(n_points) | tropical_input_points.minor(All,range(1,d-1));
   
   Matrix<Coord> tropical_directions(d,d);
   for(int i=1;i<d;++i)
   {
      tropical_directions(0,i)=1;
      tropical_directions(i,i)=-1;
   }

   perl::ObjectType t=perl::ObjectType::construct<Coord>("fan::PolyhedralComplex");
   Array<perl::Object> all(n_points);

   //construct for every input point a polyhedral complex //pc// defined by the max-tropical-hyperplane through it
   for(int i=0; i<n_points;++i) {
      all[i].create_new(t);
      Matrix<Coord> pc_points=points[i]/tropical_directions;
      std::list<Set<int> > pc_polytopes;
      
      for (Entire< Subsets_of_k<const sequence&> >::const_iterator k=entire(all_subsets_of_k(sequence(1,d),d-1)); !k.at_end(); ++k)
      {
         Set<int> set=(*k);
         set.push_front(0);
         pc_polytopes.push_back(set);
      }
      all[i].take("POINTS")<<pc_points;
      all[i].take("INPUT_POLYTOPES")<<pc_polytopes;
   }
   
   return tropical_intersection<Coord>(all);
   //return CallPolymakeFunction("fan::pc_intersection", all);
}

   UserFunctionTemplate4perl("# @category Constructing a polyhedral complex"
                             "# Computes the intersection of two polyhedral complexes."
                             "# @param fan::PolyhedralComplex pc1"
                             "# @param fan::PolyhedralComplex pc2"
                             "# @return fan::PolyhedralComplex",
                             "tropical_intersection<Coord>(fan::PolyhedralComplex<Coord> +)");

   UserFunctionTemplate4perl("# @category Constructing a polyhedral complex"
                             "# Computes the tropical complex of //points//."
                             "# @param Matrix points"
                             "# @return PolyhedralComplex"
                             "# @author Katja Kulas",
                             "tropical_complex<Coord>(Matrix<Coord>)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
