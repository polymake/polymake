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
#include "polymake/SparseMatrix.h"
#include "polymake/SparseVector.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/linalg.h"
#include "polymake/Plucker.h"

namespace polymake { namespace polytope {

typedef QuadraticExtension<Rational> QE;

namespace {

void make_pentagon(Matrix<QE>& V) 
{
   const QE tau(Rational(1,2), Rational(1,2), 5); // golden ratio

   SparseVector<QE> r0(4), r1(4), r2(4), r3(4), r4(4); // the roots
   // first, generate a subset of the H3 root system
   r0[1] = 2;
   r1[1] = -tau; r1[2] = tau-1; r1[3] = QE(-1,0,5);
   // then, complete the root subsystem by reflecting these roots. They will look like this:
   // (remember that the root vectors are normal to the reflecting hyperplane)
   /*
              r2
 
        r1

                 o        r0

        r4

              r3    
    */
   r2 = reflect(r0,r1); 
   r3 = reflect(r1,r2);
   r4 = reflect(r2,r3);
   if (POLYMAKE_DEBUG && reflect(r3,r4) != r0)
      throw std::runtime_error("Something's wrong.");

   // Now make a regular pentagon embedded in a plane.
   // The order of reflections is dictated by the root vectors above;
   // the vertices will be in the order given by their convex hull.
   V.row(0) = r1+r2; V(0,0) = QE(1,0,5);  // a point to start reflecting in
   V.row(1) = reflect(V.row(0), r0);
   V.row(2) = reflect(V.row(1), r2);
   V.row(3) = reflect(V.row(2), r4);
   V.row(4) = reflect(V.row(3), r1);
}

void complete_configuration(Matrix<QE>& V)
{
   // We will use Plucker coordinates to calculate joins and
   // intersections of points and lines.

   // First, the normal vector to the plane containing the configuration.
   const Vector<QE> nv(null_space(V).row(0));
   const Plucker<QE> k(nv);

   // now, the supporting lines of the edges of the pentagon.
   // Since the pentagon is embedded in a plane with kernel k, 
   // we artificially add the vector k so that we can use
   // the nice expression for the join offered by Plücker coordinates
   const Plucker<QE> 
      l01 = 
      Plucker<QE>(Vector<QE>(V.row(0))) +      // FIXME: Get rid of the explicit Vector<QE>() 
      Plucker<QE>(Vector<QE>(V.row(1))) + k,

      l12 = 
      Plucker<QE>(Vector<QE>(V.row(1))) + 
      Plucker<QE>(Vector<QE>(V.row(2))) + k,

      l23 = 
      Plucker<QE>(Vector<QE>(V.row(2))) + 
      Plucker<QE>(Vector<QE>(V.row(3))) + k,

      l34 = 
      Plucker<QE>(Vector<QE>(V.row(3))) + 
      Plucker<QE>(Vector<QE>(V.row(4))) + k,

      l04 = 
      Plucker<QE>(Vector<QE>(V.row(0))) + 
      Plucker<QE>(Vector<QE>(V.row(4))) + k;

   // to obtain four more points of the Gale transform, we intersect certain pairs of lines
   // and project out the vector k.
   V.row(5) = (l01 * l34).project_out(k);
   V.row(6) = (l04 * l12).project_out(k);
   V.row(7) = (l01 * l23).project_out(k);
   V.row(8) = (l04 * l23).project_out(k);


   // We replace point 0 by the barycenter of the pentagon
   for (int i=1; i<5; ++i)
      V.row(0) += V.row(i);
   V.row(0).dehomogenize();

   // we translate along the kernel vector to get something of rank 3
   for (int i=0; i<9; ++i)
      V.row(i) += nv;

   // ... and negate the first point. 
   // We don't bother to negate the first component, because we'll take a minor soon anyway.
   V.row(0).negate(); 

   // We add some of the vectors, negated
   V.row( 9) = - V.row(1); 
   V.row(10) = - V.row(3); 
   V.row(11) = - V.row(4); 

   V = V.minor(All, range_from(1));
}

void apply_transform(Matrix<QE>& V)
{
   // We apply a projective transform in the primal by rescaling the
   // vectors in the dual (i.e., in the Gale diagram) so that the
   // barycenter lies in the origin.

   // Since by construction {1,9}, {3,10} and {4,11} already sum to
   // zero, it suffices to balance the rest.
   Set<int> rowset; 
   rowset += 0;
   rowset += 2;
   rowset += 5;
   rowset += 6;
   rowset += 7;
   rowset += 8;  // c++11 will spare us this

   // a basis for the coefficients that make the scaled rows sum to zero:
   const Matrix<QE> M = null_space(T(V.minor(rowset, All))); 

   // we'll consider ourselves lucky and choose a special vector in this kernel:
   const Vector<QE> coeffs = ones_vector<QE>(3) * M; 
   if (accumulate(coeffs, operations::min()) <= 0)
      throw std::runtime_error("Couldn't find an all-positive kernel vector");

   // rescale the relevant vectors so that the configuration is balanced
   auto vit = entire(coeffs);
   for (auto sit = entire(rowset); !sit.at_end(); ++sit, ++vit)
      V.row(*sit) *= *vit;
   
   // ... and check.
   if (!is_zero(ones_vector<QE>(V.rows()) * V))
      throw std::runtime_error("Didn't succeed in balancing the configuration");
}

} // end anonymous namespace

perl::Object perles_irrational_8_polytope()
{
   Matrix<QE> V(12,4);  // The points of the Gale transform

   // First,  make a regular pentagon embedded in a plane.
   make_pentagon(V);

   // Complete the configuration to Perles' construction.
   // We will later take the Gale dual of this configuration.
   complete_configuration(V);

   // Apply a projective transform to the primal (i.e., rescale the Gale vectors in the dual)
   // so that the resulting polytope is bounded (i.e., so that the Gale transform is balanced). 
   apply_transform(V);

   // This completes the Gale transform. 
   Matrix<QE> W = T(null_space(T(V)));
   
   // Since we know that the all-ones vector is in the kernel of V by balancedness, 
   // we can force the first column to be this vector.
   W.col(0) = ones_vector<QE>(W.rows());

   // done.
   perl::Object p("Polytope<QuadraticExtension<Rational>>");
   p.set_description() << "An 8-dimensional polytope with 12 vertices due to Perles that has no rational realization" << endl;
   p.take("POINTS") << W;
   p.take("BOUNDED") << true;
   p.take("POINTED") << true;
   p.take("GALE_TRANSFORM") << V;
   return p;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Create an 8-dimensional polytope without rational realizations due to Perles"
                  "# @return Polytope",
                  &perles_irrational_8_polytope, "perles_irrational_8_polytope()");

} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
