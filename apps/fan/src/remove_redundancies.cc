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
#include "polymake/hash_map"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/FacetList.h"
#include "polymake/linalg.h"

namespace polymake { namespace fan {
  
template <typename Coord>
void remove_redundancies(perl::Object f)
{
   const int ambientDim = f.give("FAN_AMBIENT_DIM");
   const Matrix<Coord> i_rays = f.give("INPUT_RAYS");
   const IncidenceMatrix<> i_cones = f.give("INPUT_CONES");
   const int n_i_cones=i_cones.rows();
   perl::ObjectType t=perl::ObjectType::construct<Coord>("Cone");

   Matrix<Coord> lineality_space;
     const Matrix<Coord> lin = f.lookup("INPUT_LINEALITY");
     perl::Object cone(t);
     cone.take("INPUT_RAYS")<<i_rays.minor(i_cones.row(0),All);
     cone.take("INPUT_LINEALITY")<<lin;
     cone.give("LINEALITY_SPACE")>>lineality_space;


   hash_map<Vector<Coord>,int > rays;    
   FacetList max_cones;
   int n_rays=0;
   for(int i=0; i<n_i_cones;++i) 
   {
      perl::Object cone(t);
      cone.take("INPUT_RAYS")<<i_rays.minor(i_cones.row(i),All);
      cone.take("INPUT_LINEALITY")<<lineality_space;
      const Matrix<Coord> c_rays=cone.give("RAYS");
      Set<int> ray_indices;
      for(typename Entire<Rows<Matrix<Coord> > >::const_iterator r=entire(rows(c_rays)); !r.at_end(); ++r) {
         const Vector<Rational> the_ray(*r);
         typename hash_map<Vector<Coord>,int >::iterator r_iti=rays.find(the_ray);
         if (r_iti==rays.end()) {
            bool found=false;
            for(typename Entire<hash_map<Vector<Coord>,int> >::const_iterator rr=entire(rays); !found && !rr.at_end(); ++rr) {
              try {
                const Vector<Coord> v=lin_solve(T((rr->first)/lineality_space),the_ray);
                if (v[0]>0) {
                  ray_indices.insert(rr->second);
                  found=true;
                }
              }catch(infeasible){}
            }
            if (!found) {
              rays[the_ray]=n_rays;
              ray_indices.insert(n_rays++);
            }
         }
         else ray_indices.insert(r_iti->second);
      }
      max_cones.insertMax(ray_indices);
   }

   // create ray matrix
   Matrix<Coord> R(n_rays,ambientDim);
   for(typename Entire<hash_map<Vector<Coord>,int> >::const_iterator r=entire(rays); !r.at_end(); ++r)
      R.row(r->second)=r->first;
   f.take("RAYS")<<R;
   f.take("LINEALITY_SPACE")<<lineality_space;

   // Take care of the empty fan and the fan containing only the origin
   if(max_cones.empty() && n_i_cones > 0)
      f.take("MAXIMAL_CONES")<< IncidenceMatrix<>(1,0);
   else
      f.take("MAXIMAL_CONES")<<max_cones;
}
  
FunctionTemplate4perl("remove_redundancies<Coord>(PolyhedralFan<Coord>) : void");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
