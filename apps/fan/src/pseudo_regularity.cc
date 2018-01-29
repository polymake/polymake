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
#include "polymake/IncidenceMatrix.h"
#include "polymake/SparseMatrix.h"
#include "polymake/Integer.h"
#include "polymake/Rational.h"
#include "polymake/linalg.h"
#include "polymake/common/lattice_tools.h"

namespace polymake { namespace fan {

   template <typename Scalar>
      std::pair< bool , Matrix<Scalar> > pseudo_regular(perl::Object fan)
      {



         perl::ObjectType linear_program_type=perl::ObjectType::construct<Scalar>("LinearProgram");
         perl::ObjectType polytope_type=perl::ObjectType::construct<Scalar>("Polytope");

         const Matrix<Scalar> raysout= fan.give("RAYS");
         const Matrix<Scalar> rays = -raysout;
         const IncidenceMatrix<> max_cones = fan.give("MAXIMAL_CONES");
         const int num_rays = rays.rows();
         const int num_max_cones = max_cones.rows();
         const int dim = rays.cols();//fan.give("FAN_DIM");
         const int lp_dim = num_max_cones * dim + num_rays + 1; 

         // for the d-dim. fan, we construct a polytope in dimension [(#max.cones)*d + #rays + 1] 
         // the first summand describes the vertices of the polytope whose normal fan shall be the given fan, the second the distances of the facet hyperplanes to the origin, the last one a slack variable epsilon >= 0

         // the first max. cone gives us the first part of the inequality matrix I and the equality matrix L

         int num_rays_cone0 = max_cones.row(0).size();
         int num_rays_not_cone0 =  num_rays - num_rays_cone0;

         SparseMatrix<Rational> L( num_rays_cone0 , lp_dim );
         int r = 0;
         for( const auto it : max_cones.row(0) )
         {
            for( int i = 0 ; i < dim ; i++ )
            {
               L[r][ 0*dim+i ] = rays[it][i];
            }
            L[r][ num_max_cones*dim+it ] = -1;
            r++;
         }

         Set<int> all_rays{};
         for( int i = 0 ; i < num_rays ; i++ )
         {
            all_rays += i;
         }
         Set<int> rays_not_cone0 = all_rays - max_cones.row(0);

         SparseMatrix<Rational> I( num_rays_not_cone0+num_rays+2 , lp_dim );
         // epsilon >= 0 & epsilon <= 1
         I[0][lp_dim-1] = -1;
         I[1][lp_dim-1] = 1;
         int rr= 2;
         // the a_i's > 0
         for( int i = 0 ; i < num_rays ; i++ )
         {
            I[rr][ num_max_cones*dim+i ] = -1;
            I[rr][ lp_dim-1 ] = 1;
            rr++;
         }
         // <n_i,p_j> < a_i for all normals n_i that are not rays of the cone C_j corresponding to the vertex p_j
         for( const auto it : rays_not_cone0 )
         {
            for( int i = 0 ; i < dim ; i++ )
            {
               I[rr][ 0*dim+i ] = rays[it][i];
            }

            I[rr][ num_max_cones*dim+it ] = -1;
            I[rr][ lp_dim-1 ] = 1;
            rr++;
         }

         for( int i = 1 ; i < num_max_cones ; ++i )
         {
            int num_rays_cone_i = max_cones.row(i).size();
            int num_rays_not_cone_i =  num_rays - num_rays_cone_i;

            SparseMatrix<Rational> L_i( num_rays_cone_i , lp_dim );
            int a = 0;
            for( const auto it : max_cones.row(i) )
            {
               for( int j = 0 ; j < dim ; j++ )
               {
                  L_i[a][ i*dim+j ] = rays[it][j];
               }
               L_i[a][ num_max_cones*dim+it ] = -1;
               a++;
            }
            L /= L_i;

            Set<int> rays_not_cone_i = all_rays - max_cones.row(i);

            SparseMatrix<Rational> I_i( num_rays_not_cone_i , lp_dim );
            int aa = 0;
            for( const auto it : rays_not_cone_i )
            {
               for( int j = 0 ; j < dim ; j++ )
               {
                  I_i[aa][ i*dim+j ] = rays[it][j];
               }
               I_i[aa][ num_max_cones*dim+it ] = -1;
               I_i[aa][ lp_dim-1 ] = 1;
               aa++;
            }
            I /= I_i;
         }


         // each rays should be the normal of a FACET, not just representing a suppoting hyperplane through the corr. vertices
         // we make sure of this by demanding each facet-centroid to lie only on its facet defining hyperplane, that is NOT on the other hyperplanes
         for( int i = 0 ; i < num_rays ; ++i )
         {
            SparseMatrix<Rational> I_i( num_rays-1 , lp_dim );
            Set<int> cones_with_ray_i{};
            for( int j = 0 ; j < num_max_cones ; ++j )
            {
               if( max_cones[j].contains(i) ) cones_with_ray_i += j;
            }
            int num_cones_with_ray_i = cones_with_ray_i.size();
            int curr = 0;
            for( int k = 0 ; k < num_rays ; ++k )
            {
               if( k != i )
               {
                  for( auto it : cones_with_ray_i )
                  {  
                     for( int l = 0 ; l < dim ; ++l )
                     {
                        I_i[curr][ it*dim+l ] = rays[k][l];
                     }
                  }
                  I_i[curr][ num_max_cones*dim+k ] = -num_cones_with_ray_i;
                  curr++;
               }
            }
            I /= I_i;
         }



         I = zero_vector<Scalar>() | I;
         I[1][0] = -1;
         L = zero_vector<Scalar>() | L;


         perl::Object p(polytope_type);
         p.take("INEQUALITIES") << -I;
               //cout << "I" << endl << I << endl;
         p.take("EQUATIONS") << L;
              // cout << "L" << endl << L << endl;

         bool feasible = p.give("FEASIBLE");
         bool alt_regular = false;
         Matrix<Scalar> Facets_empty{};
         if (feasible)
         {
            perl::Object lp(linear_program_type);
            lp.take("LINEAR_OBJECTIVE") << unit_vector<Scalar>(I.cols(), I.cols()-1);
            p.add("LP", lp);

            const Scalar max = lp.give("MAXIMAL_VALUE");
            if (max > 0)
            {
               alt_regular = true;

               const Vector<Scalar> solution = lp.give("MAXIMAL_VERTEX");
               //cout << "S" << solution << endl;
               Matrix<Scalar> Facets{ num_rays , dim+1 };
               for( int i = 0 ; i < num_rays ; ++i )
               {
                  Vector<Scalar> facet_i = solution[ num_max_cones*dim+i+1 ] | -rays[i];
                  Facets[i] = facet_i;
               }

               std::pair< bool , Matrix<Scalar> > pair( alt_regular , Facets );
               return pair;
            }
         }

         std::pair< bool , Matrix<Scalar> > pair_false( alt_regular , Facets_empty );

         return pair_false;

      }

   FunctionTemplate4perl("pseudo_regular<Scalar>(PolyhedralFan<Scalar>)");

}
}

