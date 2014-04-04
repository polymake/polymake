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
#include "polymake/Set.h"
#include "polymake/Rational.h"
#include "polymake/Array.h"
#include "polymake/Matrix.h"

namespace polymake { namespace tropical {   
       
   template <typename Scalar>
   Matrix<Scalar> get_corners(Matrix<Scalar> V)
   {
      const int d(V.cols());
      
      // calculate corners (normalized s.t. the first coordinate is equal to zero)
      Matrix<Scalar> C(d,d);
      for (int i=0; i<d; ++i){
         Scalar norm=accumulate(-V.col(i)+V.col(0), operations::min());
         for (int j=1; j<d; ++j)
            C[i][j]=accumulate(-V.col(i)+V.col(j), operations::min())-norm;
      }
      return C;
   }
   
   template <typename Scalar>
   perl::Object cornered_hull_poly(perl::Object t_in)
   {
      Matrix<Scalar> V = t_in.give("VERTICES");
      const int d(V.cols());
      Matrix<Scalar> C=get_corners<Scalar>(V);        
      
      // tropical vertices of the cornered hull
      Matrix<Scalar> P(d,d); 
      for (int i=0; i<d; ++i)
         for (int j=0; j<d; ++j)
            P[i][j]=C[j][j]-C[j][i]+C[0][i];
      
      int k=0;
      Matrix<Scalar> I(d*(d-1),d);
      for (int i=0; i<d; ++i)
         for (int j=0; j<d; ++j)
         {
            if(i!=j)
            {
               I[k][0]=P[j][i]-P[j][j];
               if(i!=0)I[k][i]=-1;
               if(j!=0)I[k][j]=1;
               ++k;
            }
         }  
      
      perl::Object CH(perl::ObjectType::construct<Scalar>("polytope::Polytope"));
      CH.set_description()<<"Cornered Hull of "<<t_in.name()<<endl;
      
      CH.take("INEQUALITIES") << I;

      return CH;
    }

   template <typename Scalar>
   perl::Object cornered_hull(perl::Object t_in)
   {
      Matrix<Scalar> V = t_in.give("VERTICES");
      const int d(V.cols());
      
      // corners (normalized s.t. the first coordinate is equal to zero)
      Matrix<Scalar> C=get_corners<Scalar>(V);  
      
      // tropical vertices of the cornered hull
      Matrix<Scalar> P(d,d); 
      for (int i=0; i<d; ++i)
         for (int j=0; j<d; ++j)
            P[i][j]=C[j][j]-C[j][i]+C[0][i];
      
      
      perl::Object CH(perl::ObjectType::construct<Scalar>("TropicalPolytope"));
      CH.set_description()<<"Cornered Hull of "<<t_in.name()<<endl;
      
      CH.take("POINTS") << P;
      return CH;
   }
    
   
   UserFunctionTemplate4perl("# @category Others"
                             "# Compute the corners of a tropical polytope given by its vertices."
                             "# Cf."
                             "#    M. Joswig, arXiv:0809.4694v2, Lemma 17."
                             "# @param Matrix input points"
                             "# @return Matrix",
                             "get_corners(Matrix)");
   
   UserFunctionTemplate4perl("# @category Producing a new tropical polytope from another" //Other
                             "# Compute the cornered hull of a tropical polytope."
                             "# Cf."
                             "#    M. Joswig, arXiv:0809.4694v2, Lemma 17."
                             "# @param TropicalPolytope T"
                             "# @return Polytope",
                             "cornered_hull_poly<Scalar>(TropicalPolytope<Scalar>)");
   
   UserFunctionTemplate4perl("# @category Producing a new tropical polytope from another"
                             "# Compute the cornered hull of a tropical polytope."
                             "# Cf."
                             "#    M. Joswig, arXiv:0809.4694v2, Lemma 17."
                             "# @param TropicalPolytope T"
                             "# @return TropicalPolytope",
                             "cornered_hull<Scalar>(TropicalPolytope<Scalar>)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
