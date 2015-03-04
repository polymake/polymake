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
#include "polymake/PowerSet.h"
#include "polymake/Matrix.h"
#include "polymake/ListMatrix.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/linalg.h"
#include "polymake/polytope/is_regular.h"
#include <fstream>

namespace polymake { namespace polytope {

template <typename Scalar>
perl::Object secondary_cone(const Matrix<Scalar> &verts,const Array<Set <int> >& subdiv,perl::OptionSet options) {

   perl::Object p(perl::ObjectType::construct<Scalar>("Cone"));

  
   if (subdiv.size()==1 && subdiv[0].size() == verts.rows() && rank(verts) == verts.rows()) {
      p.take("RAYS")<<Matrix<Scalar>(0,verts.rows());
      p.take("CONE_AMBIENT_DIM")<<verts.rows();
      p.take("LINEALITY_SPACE")<<unit_matrix<Scalar>(verts.rows());
      return p;
   }
  

   std::pair<const Matrix<Scalar>, const Matrix<Scalar> > mats = secondary_cone_ineq<Scalar, Set<int> >(verts,subdiv,options);

   p.take("INEQUALITIES")<< mats.first;
   p.take("EQUATIONS")<< mats.second;
   
   if (options["test_regularity"]) {
      const Vector<Scalar> w=p.give("REL_INT_POINT");
      const Vector<Scalar> slack= mats.first*w;

      for(typename Entire<Vector<Scalar> >::const_iterator it=entire(slack); !it.at_end();++it){
         if(*it==0)
            throw std::runtime_error("Subdivision is non-regular.");   
      }
   }
   return p;
}

template <typename Scalar>
perl::Object regularity_lp(const Matrix<Scalar> &verts,const Array<Set <int> >& subdiv,perl::OptionSet options) {
   
   if (subdiv.size()<2)
      throw std::runtime_error("Subdivision is trivial.");   

   std::pair<const Matrix<Scalar>,const Matrix<Scalar> > mats = secondary_cone_ineq(verts,subdiv,options);
   const Matrix<Scalar>& inequs = mats.first;
   const Matrix<Scalar>& equats = mats.second;
   
   const int n_vertices=verts.rows();

   Scalar epsilon = options["epsilon"];
   perl::Object q(perl::ObjectType::construct<Scalar>("Polytope"));
   q.take("FEASIBLE") << true;
   q.take("EQUATIONS") << (zero_vector<Scalar>(equats.rows()) | equats);
   q.take("INEQUALITIES") << 
      (zero_vector<Scalar>(n_vertices) | unit_matrix<Scalar>(n_vertices)) /
      ((-epsilon * ones_vector<Scalar>(inequs.rows())) | inequs);

   perl::Object lp(perl::ObjectType::construct<Scalar>("LinearProgram"));
   lp.attach("INTEGER_VARIABLES") << Array<bool>(n_vertices, true);
   lp.take("LINEAR_OBJECTIVE") << (Scalar(0) | ones_vector<Scalar>(n_vertices));
   q.take("LP") << lp;
   
   return q;
}



template <typename Scalar>
std::pair<bool,Vector<Scalar> > is_regular(const Matrix<Scalar> &verts,const Array<Set <int> >& subdiv,perl::OptionSet options)
{
   perl::Object res=secondary_cone(verts,subdiv,options);
   const Matrix<Scalar> ineq=res.give("INEQUALITIES");
   const Vector<Scalar> w=res.give("REL_INT_POINT");
   const Vector<Scalar> slack= ineq*w;

   for(typename Entire<Vector<Scalar> >::const_iterator it=entire(slack); !it.at_end();++it){
      if(*it==0)
         return std::pair<bool,Vector<Scalar> >(false,Vector<Scalar>());
   }
   return std::pair<bool,Vector<Scalar> >(true,w);
}


UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# For a given subdivision //subdiv// of //points// tests"
                          "# computes the corresponding secondary cone. If the subdivision"
                          "# is not regular, the cone will be the secondary cone of"
                          "# the finest regular coarsening of //subdiv//. (See option //test_regularity//)"
                          "# Options can be used to make the Cone [[POINTED]]."
                          "# @param Matrix points in homogeneous coordinates"
                          "# @param Array<Set<Int> > subdiv"
                          "# @return Cone"
                          "# @option Matrix<Scalar> equations system of linear equation the cone is cut with."
                          "# @option Set<Int> lift_to_zero gives only lifting functions lifting the designated vertices to 0"
                          "# @option Int lift_face_to_zero gives only lifting functions lifting all vertices of the designated face to 0"
                          "# @option Bool test_regularity throws an exception if the subdivision is not regular"
                          "# @author Sven Herrmann",
                          "secondary_cone<Scalar>(Matrix<Scalar> Array<Set>; {equations => undef, lift_to_zero=>undef, lift_face_to_zero => undef, test_regularity=>0})");


UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# For a given subdivision //subdiv// of //points// tests"
                          "# if the subdivision is regular and if yes computes a weight"
                          "# vector inducing this subdivsion."
                          "# The output is a pair of Bool and the weight vector."
                          "# Options can be used to ensure properties of the resulting vector."
                          "# The default is having 0 on all vertices of the first face of //subdiv//."
                          "# @param Matrix points in homogeneous coordinates"
                          "# @param Array<Set<Int> > subdiv"
                          "# @return Pair<Bool,Vector>"
                          "# @option Matrix<Scalar> equations system of linear equation the cone is cut with."
                          "# @option Set<Int> lift_to_zero gives only lifting functions lifting the designated vertices to 0"
                          "# @option Int lift_face_to_zero gives only lifting functions lifting all vertices of the designated face to 0"
                          "# @author Sven Herrmann fixed by Benjamins with Georg",
                          "is_regular<Scalar>(Matrix<Scalar>,$;{equations => undef, lift_to_zero=>undef, lift_face_to_zero => 0})");

UserFunctionTemplate4perl("# @category Triangulations, subdivisions and volume"
                          "# For a given subdivision //subdiv// of //points// determines"
                          "# a //LinearProgram// to decide whether the subdivision is regular."
                          "# The output a Polytope with an attached LP."
                          "# Options can be used to ensure properties of the resulting LP."
                          "# The default is having 0 on all vertices of the first face of //subdiv//."
                          "# @param Matrix points in homogeneous coordinates"
                          "# @param Array<Set<Int> > subdiv"
                          "# @return Polytope<Scalar>"
                          "# @option Matrix<Scalar> equations system of linear equation the cone is cut with."
                          "# @option Set<Int> lift_to_zero gives only lifting functions lifting the designated vertices to 0"
                          "# @option Int lift_face_to_zero gives only lifting functions lifting all vertices of the designated face to 0"
                          "# @option Scalar epsilon minimum distance from all inequalities"
                          "# @author Sven Herrmann",
                          "regularity_lp<Scalar>(Matrix<Scalar>,$;{equations => undef, lift_to_zero=>undef, lift_face_to_zero => 0, epsilon => 1/100})");

} }
