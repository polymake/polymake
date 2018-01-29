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
#include "polymake/hash_map"
#include "polymake/Rational.h"
#include "polymake/IncidenceMatrix.h"
#include "polymake/Set.h"
#include "polymake/FacetList.h"

namespace polymake { namespace fan {
namespace {

bool is_face (const Set<int>& F, const IncidenceMatrix<>& I)
{
   Set<int> face;

   int i = 0;
   while ( i < I.rows() && face.size() == 0 && incl(F,I[i]) > 0 ) ++i;

   if ( i == I.rows() ) {
      return false;
   }

   face = I[i];
   while ( i < I.rows() && face.size() > F.size() ) {
      if ( incl( F, I[i] ) <= 0 ) {
         face *= I[i];
      }
      ++i;
   }

   return ( face.size() == F.size() );
}

}


template <typename Coord>
perl::Object check_fan_objects(const Array<perl::Object>& all_cones, perl::OptionSet options)
{
   const int n_i_cones=all_cones.size();
   bool verbose=options["verbose"];
   FacetList max_cones;

   const perl::ObjectType t=all_cones.element_type();
   const int lineality_dim=all_cones[0].give("LINEALITY_DIM");
   const int ambientDim = all_cones[0].give("CONE_AMBIENT_DIM");

   for (int i=0; i<n_i_cones; ++i) {
      perl::Object cone=all_cones[i];
      const Matrix<Coord> c_rays=cone.give("RAYS");
      const Matrix<Coord> facets=cone.give("FACETS");
      const Matrix<Coord> eqs=cone.give("LINEAR_SPAN");
      const IncidenceMatrix<> inc=cone.give("RAYS_IN_FACETS");
      const int cone_lin_dim=cone.give("LINEALITY_DIM");
      if (cone_lin_dim != lineality_dim) {
         if (verbose) cout << "Cones do not have the same lineality space." << endl;
         throw std::runtime_error("not a fan");
      }
      // test intersection property
      for (int j=i+1; j<n_i_cones;++j){
         perl::Object inters(t);
         const Matrix<Coord> facets2=all_cones[j].give("FACETS");
         const Matrix<Coord> eqs2=all_cones[j].give("LINEAR_SPAN");
         const Matrix<Coord> c_rays2=all_cones[j].give("RAYS");
         //cerr<<facets2<<endl<<eqs2<<endl<<c_rays2<<endl;
         inters.take("INEQUALITIES")<<(facets/facets2);
         inters.take("EQUATIONS")<<(eqs/eqs2);
         const Matrix<Coord> int_rays=inters.give("RAYS");
         //cerr<<"intersection of:"<<endl<<c_rays<<" and "<<endl<<c_rays2<<":"<<endl<<int_rays<<endl;
         const int n_int_rays=int_rays.rows();
         if (n_int_rays>0) {
            //the rays of the intersection must be rays of the two cones
            Set<int> rays_in_cone,rays_in_cone2;
            for (int i=0;i<n_int_rays;++i) {
               bool found=false;
               for (int j=0;!found&&j<c_rays.rows();++j) if (c_rays[j]==int_rays[i]) {
                  rays_in_cone.insert(j);
                  found=true;
               }
               if (!found) {
                  if (verbose) cout<<"Ray "<<int_rays[i]<<" is not a ray of the cone with rays"<<endl<< c_rays<<"."<<endl;
                  throw std::runtime_error("not a fan");
               }
               found=false;
               for (int j=0;!found&&j<c_rays2.rows();++j) if (c_rays2[j]==int_rays[i]) {
                  rays_in_cone2.insert(j);
                  found=true;
               }
               if (!found) {
                  if (verbose) cout<<"Ray "<<int_rays[i]<<" is not a ray of the cone with rays"<<endl<< c_rays2<<"."<<endl;
                  throw std::runtime_error("not a fan");
               }
            }
            //if the intersection has the same number of rays than one of the cones, it is one of the cones
            //otherwise we explicitely check if it is a face of
            if (!(n_int_rays==c_rays.rows()||n_int_rays==c_rays2.rows())){
               if (!is_face(rays_in_cone,inc)) {
                  if (verbose) cout << "Intersection " << rays_in_cone << " from " << i << " with " << j << " not a face of the first one, RIF: " << inc << endl;
                  throw std::runtime_error("not a fan");
               }
               const IncidenceMatrix<> inc2=all_cones[j].give("RAYS_IN_FACETS");
               if (!is_face(rays_in_cone2,inc2)) {
                  if (verbose) cout << "Intersection " << rays_in_cone2 << " from " << i << " with " << j << " not a face of the second one, RIF: " << inc2<< endl;
                  throw std::runtime_error("not a fan");
               }
            }
         }
      }
   }

   hash_map<Vector<Coord>, int> rays;

   int n_rays=0;

   for(int i=0; i<n_i_cones;++i) {
      perl::Object cone=all_cones[i];
      const Matrix<Coord> c_rays=cone.give("RAYS");
      Set<int> ray_indices;
      for(typename Entire<Rows<Matrix<Coord> > >::const_iterator r=entire(rows(c_rays)); !r.at_end(); ++r) {
         typename hash_map<Vector<Coord>,int >::const_iterator r_iti=rays.find(*r);
         if (r_iti==rays.end()) {
            rays[*r]=n_rays;
            ray_indices.insert(n_rays++);
         }
         else {
            ray_indices.insert(r_iti->second);
         }
      }
      max_cones.insertMax(ray_indices);
      //FIXME: one could also extract and save other properties here.
   }
   //cerr<<"created rays"<<endl;
   // create ray matrix
   Matrix<Coord> R(n_rays,ambientDim);
   for(typename Entire<hash_map<Vector<Coord>,int> >::const_iterator r=entire(rays); !r.at_end(); ++r)
      R.row(r->second)=r->first;
   perl::ObjectType t_fan=perl::ObjectType::construct<Coord>("PolyhedralFan");
   perl::Object f(t_fan);
   //cerr<<".."<<endl;
   const Matrix<Coord> lineality=all_cones[0].give("LINEALITY_SPACE");
   //cerr<<"got linspace"<<endl;
   f.take("LINEALITY_SPACE")<<lineality;
   f.take("RAYS")<<R;
   f.take("MAXIMAL_CONES")<<max_cones;
   return f;
}


//template<typename Coord>
typedef Rational Coord;
perl::Object check_fan(const Matrix<Coord>& i_rays, const IncidenceMatrix<>& i_cones, perl::OptionSet options)
{
   const int n_i_cones=i_cones.rows();
   Matrix<Coord> linealitySpace;
   const int ambientDim=i_rays.cols();
   if (!(options["lineality_space"] >> linealitySpace))
      linealitySpace=Matrix<Coord>(0, ambientDim);
   perl::ObjectType t=perl::ObjectType::construct<Coord>("Cone");
   Array<perl::Object> all_cones(t, n_i_cones);
   for (int i=0; i<n_i_cones; ++i) {
      all_cones[i].take("INPUT_RAYS") << i_rays.minor(i_cones[i], All);
      all_cones[i].take("INPUT_LINEALITY") << linealitySpace;
   }
   perl::Object f=check_fan_objects<Coord>(all_cones, options);
   f.take("INPUT_RAYS") << i_rays;
   f.take("INPUT_CONES") << i_cones;
   return f;
}


UserFunction4perl("# @category Consistency check"
                  "# Checks whether a given set of //rays// together with a list //cones//"
                  "# defines a polyhedral fan."
                  "# If this is the case, the ouput is the [[PolyhedralFan]] defined by //rays//"
                  "# as [[INPUT_RAYS]], //cones// as [[INPUT_CONES]], //lineality_space// as"
                  "# [[LINEALITY_SPACE]] if this option is given."
                  "# @param Matrix rays"
                  "# @param IncidenceMatrix cones"
                  "# @option Matrix lineality_space Common lineality space for the cones."
                  "# @option Bool verbose prints information about the check."
                  "# @return PolyhedralFan",
                  &check_fan,"check_fan($ $ {lineality_space=> undef, verbose=>0})");

UserFunctionTemplate4perl("# @category Consistency check"
                          "# Checks whether the [[polytope::Cone]] objects form a polyhedral fan."
                          "# If this is the case, returns that [[PolyhedralFan]]."
                          "# @param Array<Cone> cones"
                          "# @option Bool verbose prints information about the check."
                          "# @tparam Coord"
                          "# @return PolyhedralFan",
                          "check_fan_objects<Coord>(Cone<Coord> +;{verbose=>0})");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
