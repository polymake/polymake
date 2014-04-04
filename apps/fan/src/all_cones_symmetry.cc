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
#include "polymake/Matrix.h"
#include "polymake/Set.h"
#include "polymake/Array.h"
#include "polymake/graph/HasseDiagram.h"
#include "polymake/hash_set"
#include "polymake/hash_map"

namespace polymake { namespace fan {

void all_cones_symmetry(perl::Object f,int dim)
{
   const Array<Set<int> > cones=f.give("MAXIMAL_CONES_REPS");
   const Matrix<Rational> rays=f.give("RAYS");
   const Array<Array<int> > symmetry=f.give("RAYS_IMAGES");
   const Matrix<Rational> lin_space=f.give("LINEALITY_SPACE");
   const int lin_dim=lin_space.rows();
   const int amb_dim=rays.rows();
  

   //if the dimension of the fan is unknown it will be computet in res_dim and amb_dim
   //is an upper boun d
   bool dim_given=true;
   int res_dim=dim;
   Array<int> max_cones_dims;
   if (dim<0) {
      dim_given=false;
      dim=amb_dim;
      max_cones_dims=Array<int>(cones.size());
   }

   Array<int> dims(dim,0);
   Array<int> orbit_dims(dim,0);

   typedef hash_set< Set<int> > Hashset;
   Array<Hashset> all_cones(dim);
   const int n_syms=symmetry[0].size();
   Array<std::list<Set<int> > > cone_orbits(dim);//all cones up to symmetry
   Array<std::list<int> > cone_orbit_sizes(dim);//the sizes of all arbits of cones
   
   int j=0;
   for (Entire<Array<Set<int> > >::const_iterator i=entire(cones);!i.at_end();++i) {
      const Set<int> cone=*i;
      //cerr<<"Cone "<<cone<<endl;
      const int n_rays=cone.size();
      const Array<int> cone_A(n_rays,entire(cone));
      perl::Object c("Cone");
      c.take("RAYS")<<rays.minor(cone,All);
      c.take("LINEALITY_SPACE")<<lin_space;
      const graph::HasseDiagram hd=c.give("HASSE_DIAGRAM");
      const int c_dim=hd.dim()+1;
      if (!dim_given) {
         res_dim=res_dim<c_dim?c_dim:res_dim;
         max_cones_dims[j++]=c_dim+lin_dim;
      }
      for (int l=0;l<c_dim;++l) {
         //cerr<<" in dimension "<<l<<endl;
         for(Entire<graph::HasseDiagram::nodes_of_dim_set>::const_iterator f=entire(hd.nodes_of_dim(l));!f.at_end();++f) {
            const Set<int> face_old=hd.face(*f);
            //cerr<<face_old;
            
            Set<int> face;
            for (Entire<Set<int> >::const_iterator k=entire(face_old);!k.at_end();++k) {
               face.insert(cone_A[*k]);
            }
            //cerr<<face<<endl<<"apply symmetry:";
            
            if (all_cones[l].find(face)==all_cones[l].end()) {
               //if the face does not exist so far, we create its whole orbit
                int orbit_size=0;
                for (int j=0;j<n_syms;++j) {
                   Set<int> new_cone;
                   for (Entire<Set<int> >::const_iterator k=entire(face);!k.at_end();++k) {
                      new_cone.insert(symmetry[*k][j]);
                   }
                   if (all_cones[l].insert(new_cone).second) {
                      ++orbit_size;
                      ++dims[l];
                      //cerr<<new_cone<<endl;
                   }
                }
                cone_orbits[l].push_back(face);
                cone_orbit_sizes[l].push_back(orbit_size);
                //cerr<<"new orbit: "<<cone_orbits[l]<<endl;
                ++orbit_dims[l];
             }
          }   
      }
      //cerr<<"got "<<dims<<" till now"<<endl;
   }
   
   if(!dim_given) {
      f.take("COMBINATORIAL_DIM")<<res_dim;
      f.take("MAXIMAL_CONES_REPS_DIMS")<<max_cones_dims;
      f.take("F_VECTOR")<<Array<int>(res_dim,entire(dims));
      f.take("ORBITS_F_VECTOR")<<Array<int>(res_dim,entire(orbit_dims));
      //cerr<<"write cones:"<<endl;
      f.take("CONES_REPS")<<Array<Array<Set<int> > >(res_dim,entire(cone_orbits));
      //cerr<<"write sizes:"<<cone_orbit_sizes<<endl;
      Array<Array<int> > cos(res_dim);
      /*for (int i=0; i<res_dim; ++i)
        cos[i]=Array<int>(max_cones_dims[i],entire(cone_orbit_sizes[i]));
      //cerr<<"..";*/
      f.take("CONES_ORBIT_SIZES")<<Array<Array<int> >(res_dim,entire(cone_orbit_sizes));
      //cerr<<"done!"<<endl;
     }
   else {
      f.take("F_VECTOR")<<dims;
      f.take("ORBITS_F_VECTOR")<<orbit_dims;
      //cerr<<"write cones:"<<endl;
      f.take("CONES_REPS")<<cone_orbits;
      //cerr<<"write sizes:"<<cone_orbit_sizes<<endl;
      f.take("CONES_ORBIT_SIZES")<<cone_orbit_sizes;
   }    
}

Function4perl(&all_cones_symmetry, "all_cones_symmetry(SymmetricFan;$=-1)");

} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
