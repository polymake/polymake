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
#include "polymake/hash_map"
#include "polymake/Matrix.h"
#include "polymake/linalg.h"
#include "polymake/SparseMatrix.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"

namespace polymake { namespace fan {
  
template <typename Coord>
void facetsToRays(perl::Object f) 
{
   perl::ObjectType t=perl::ObjectType::construct<Coord>("Cone");
 
   const int ambientDim = f.give("FAN_AMBIENT_DIM");
   const Matrix<Coord> facets=f.give("FACET_NORMALS");
   if (!facets.rows()) {
      f.take("RAYS")<<Matrix<Coord>(0,ambientDim);
      f.take("MAXIMAL_CONES")<<IncidenceMatrix<>(0,0);
      f.take("LINEALITY_SPACE")<<Matrix<Coord>(0,ambientDim);
      return;
   }
   
   const SparseMatrix<int> facetIndices=f.give("MAXIMAL_CONES_FACETS");
   const int n_cones=facetIndices.rows();
   Matrix<Coord> linearSpan;
   IncidenceMatrix<> linearSpanIndices;
   bool ls_exists=false;
   if(f.lookup("LINEAR_SPAN_NORMALS") >> linearSpan && linearSpan.rows()) {
      ls_exists=true;
      f.lookup("MAXIMAL_CONES_LINEAR_SPAN_NORMALS") >>linearSpanIndices;
   }

   std::string n;
   const Matrix<Coord> lin = f.lookup_with_property_name("LINEALITY_SPACE|INPUT_LINEALITY",n);
   Matrix<Coord> lineality_space;  
   bool lin_given=false;
   if (n=="LINEALITY_SPACE") {
     lineality_space=lin;
     lin_given=true;
   }
   
   Array<Set<int> > max_cones(n_cones);
   hash_map<Vector<Coord>,int > rays;    
   int n_rays=0;
   // iterate cones
   for(int i=0; i<n_cones;++i) {
      perl::Object cone(t);
      Set<int> pos_eq,neg_eq;
      //cerr<<i<<": "<<facetIndices.row(i)<<":"<<endl;
      for (Entire< SparseMatrix<int>::row_type>::const_iterator e=entire(facetIndices.row(i)); !e.at_end(); ++e) {
         if ((*e)>0) pos_eq.push_back(e.index());
         else if ((*e)<0)  neg_eq.push_back(e.index());
      }
      //cerr<<pos_eq<<" vs. "<<neg_eq<<endl;
      const Matrix<Coord> c_facets=facets.minor(pos_eq,All)/-facets.minor(neg_eq,All);
      cone.take("INEQUALITIES")<<c_facets;
      if (ls_exists) cone.take("EQUATIONS")<<linearSpan.minor(linearSpanIndices.row(i),All);
      // wrinting INPUT_LINEALITY without INPUT_RAYS is not allowed after #519
      // cone.take("INPUT_LINEALITY")<<lin;
      Matrix<Coord> c_rays=cone.give("RAYS");
      if (!lin_given) {
        const Matrix<Coord> lin_space=cone.give("LINEALITY_SPACE");
        if (i==0) lineality_space=lin_space;
        else if (lineality_space!=lin_space) {
          if (lineality_space.rows()!=lin_space.rows()) throw std::runtime_error("facetsToRays: Cones have different lineality");
          if (rank(lineality_space/lin_space)!=lineality_space.rows()) throw std::runtime_error("facetsToRays: Cones have different lineality");
        }
      }
      // we need to normalize the rays wrt the lineality
      // otherwise we might get the combinatorics wrong (via different representatives for the same face)
      if (lineality_space.rows() > 0)
         project_to_orthogonal_complement(c_rays,lineality_space);
      //cerr<<"rays:"<<c_rays<<endl;
      Set<int> ray_indices;
      for(typename Entire<Rows<Matrix<Coord> > >::const_iterator r=entire(rows(c_rays)); !r.at_end(); ++r) {
         typename hash_map<Vector<Coord>,int >::iterator r_iti=rays.find(*r);
         if (r_iti==rays.end()) {
            rays[*r]=n_rays;
            ray_indices.insert(n_rays++);
         }
         else 
            ray_indices.insert(r_iti->second);
      }
      max_cones[i]=ray_indices;
   }
    
   // create ray matrix
   Matrix<Coord> R(n_rays,ambientDim);
   for(typename Entire<hash_map<Vector<Coord>,int> >::const_iterator r=entire(rays); !r.at_end(); ++r)
      R.row(r->second)=r->first;

   f.take("RAYS")<<R;
   f.take("LINEALITY_SPACE")<<lineality_space;
   f.take("MAXIMAL_CONES")<<max_cones;
   
}
  
FunctionTemplate4perl("facetsToRays<Coord> (PolyhedralFan<Coord>) : void");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
