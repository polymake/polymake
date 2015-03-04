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

#ifndef POLYMAKE_POLYTOPE_IS_REGULAR_H
#define POLYMAKE_POLYTOPE_IS_REGULAR_H

#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include <fstream>

namespace polymake { namespace polytope {


template <typename Scalar, typename SetInt>
std::pair< const Matrix<Scalar>,const Matrix<Scalar> > secondary_cone_ineq(const Matrix<Scalar> &verts,const Array<SetInt>& subdiv,perl::OptionSet options)
{

   const int n_vertices=verts.rows();
   const int ambient_dim=verts.cols()-1;
   const int n_facets=subdiv.size();

   //compute the set of all points that is not used in any face
   SetInt not_used(sequence(0,n_vertices));
   for (int i=0; i<n_facets; ++i)
      not_used-=subdiv[i];

   //compute a full-dimensional orthogonal projection if verts is not full_dimensional
   const Matrix<Scalar> affine_hull=null_space(verts);
   const int codim=affine_hull.rows();
   bool found=false;
   SetInt coords;
   for (Entire< Subsets_of_k <const sequence &> >::const_iterator i=entire(all_subsets_of_k(sequence(0,ambient_dim),codim));!found&&!i.at_end(); ++i)
      if (det(affine_hull.minor(All,*i)).non_zero()) {
         coords=*i;
         found=true;
      }
   const Matrix<Scalar> vertices=verts.minor(All,~coords);  

   const int dim=vertices.cols()-1;

   // the equations and inequalities for the possible weight vectors
   // (without right hand side which will be 0)
   ListMatrix<Vector<Scalar> > equats;
   ListMatrix<Vector<Scalar> > inequs;

   Matrix<Scalar> eqs;
   if (options["equations"]>> eqs) equats/=eqs;
   Set<int> tozero = options["lift_to_zero"];
   int face;
   if (!equats.rows() &&tozero.empty() && options["lift_face_to_zero"]>>face) {
      tozero+=subdiv[face];
   }
   for(typename Entire<Set<int> >::const_iterator j=entire(tozero);!j.at_end();++j)
      equats/=unit_vector<Scalar>(n_vertices,*j);



   //now we start generating the equation and inequalities
   for (int i=0; i<n_facets; ++i) {
      const SetInt b(basis_rows(vertices.minor(subdiv[i],All)));

      //we have to map the numbers the right way:
      SetInt basis;
      int k=0;
      typename Entire<SetInt>::const_iterator l=entire(subdiv[i]);
      for(typename Entire<SetInt>::const_iterator j=entire(b);!j.at_end();++j,++k,++l) {
         while(k<*j) {
            ++k;
            ++l;
         }
         basis.push_back(*l);
      }
      const Scalar basis_det=det(vertices.minor(basis,All));
      const int basis_sign=basis_det>0?1:-1;
      const SetInt non_basis=subdiv[i]-basis;

      //for each maximal face F, all points have to be lifted to the same facet
      for(typename Entire<SetInt>::const_iterator j=entire(non_basis);!j.at_end();++j) {
         Vector<Scalar> eq(n_vertices);
         int s=basis_sign;
         eq[*j]=s*basis_det;
         for(typename Entire<SetInt>::const_iterator k=entire(basis);!k.at_end();++k) {
            const SetInt rest=basis-SetInt(scalar2set(*k));
            s=-s;
            eq[*k]=s*det(vertices[*j]/vertices.minor(rest,All));
         }
         equats/=eq;
      }


      // for all adjacent maximal faces, all vertices not contained in F have to be lifted
      // in the same direction
      for(int l=i+1; l<n_facets; ++l)
         if (rank(vertices.minor(subdiv[i]*subdiv[l],All))==dim) {
            const int j=*((subdiv[l]-subdiv[i]).begin());
            Vector<Scalar> ieq(n_vertices);
            int s=basis_sign;      
            ieq[j]=s*basis_det;
            for(typename Entire<SetInt>::const_iterator k=entire(basis);!k.at_end();++k) {
               const SetInt rest=basis-SetInt(scalar2set(*k));
               s=-s;
               ieq[*k]=s*det(vertices[j]/vertices.minor(rest,All));
            }
            inequs/=ieq;
         }

      //additional equations for the non-used points
      for(typename Entire<SetInt>::const_iterator l=entire(not_used);!l.at_end();++l) {
         Vector<Scalar> ieq(n_vertices);
         int s=basis_sign;      
         ieq[*l]=s*basis_det;
         for(typename Entire<SetInt>::const_iterator k=entire(basis);!k.at_end();++k) {
            const SetInt rest=basis-SetInt(scalar2set(*k));
            s=-s;
            ieq[*k]=s*det(vertices[*l]/vertices.minor(rest,All));
         }
         inequs/=ieq;
      }

   }
   return std::pair<const Matrix<Scalar>,const Matrix<Scalar> >(inequs,equats);
}

} }

#endif
