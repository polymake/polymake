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
#include "polymake/SparseMatrix.h"
#include "polymake/QuadraticExtension.h"
#include "polymake/polytope/simple_roots.h"

namespace polymake { namespace polytope {

SparseMatrix<Rational> simple_roots_type_A (const int n)
{
   /*
     Read rowwise, these simple root vectors are
     0 1 -1  0 0 ... 0  0
     0 0  1 -1 0 ... 0  0
     ...
     0 0  0  0 0 ... 1 -1
     In particular, they lie at infinity, and in the plane (sum of coordinates = 0)
    */
   SparseMatrix<Rational> R(n, n+2);
   Rows<SparseMatrix<Rational> >::iterator rit = rows(R).begin();
   for (int i=0; i<n; ++i, ++rit) {
      SparseVector<Rational> v(n+2);
      v[i+1] = 1; 
      v[i+2] = -1;
      *rit = v;
   }
   return R;
}

SparseMatrix<Rational> simple_roots_type_B (const int n)
{
   /*
     Read rowwise, these simple root vectors are
     0 1 -1  0 0 ... 0  0
     0 0  1 -1 0 ... 0  0
     ...
     0 0  0  0 0 ... 1 -1
     0 0  0  0 0 ... 0  1

     In particular, they lie at infinity.

     The Dynkin diagram is:

     0 ---- 1 ---- ... ---- n-2 --(4)--> n-1,
    */
   SparseVector<Rational> v(n+1);
   v[n] = 1;
   return simple_roots_type_A(n-1) / v;
}

SparseMatrix<Rational> simple_roots_type_C (const int n)
{
   /*
     Read rowwise, these simple root vectors are
     0 1 -1  0 0 ... 0  0
     0 0  1 -1 0 ... 0  0
     ...
     0 0  0  0 0 ... 1 -1
     0 0  0  0 0 ... 0  2

     In particular, they lie at infinity.

     The Dynkin diagram is:

     0 ---- 1 ---- ... ---- n-2 <--(4)-- n-1,
    */
   SparseVector<Rational> v(n+1);
   v[n] = 2;
   return simple_roots_type_A(n-1) / v;
}

SparseMatrix<Rational> simple_roots_type_D (const int n)
{
   /*
     Read rowwise, these simple root vectors are
     0 1 -1  0 0 ... 0 0
     0 0  1 -1 0 ... 0 0
     ...
     0 0  0  0 0 ... 1 -1
     0 0  0  0 0 ... 1  1
     In particular, they lie at infinity. The indexing of the Dynkin diagram is

                           n-2
                           /
     0 - 1 - 2 - ... - n-3
                           \
                           n-1

   */
   SparseVector<Rational> v(n+1);
   v[n-1] = v[n] = 1;
   return simple_roots_type_A(n-1) / v;
}

SparseMatrix<Rational> simple_roots_type_E8()
{
   /*
     Read rowwise, these simple root vectors are
     0 1 -1  0 0 0 0  0 0
     0 0  1 -1 0 0 0  0 0
     ...
     0 0  0  0 0 0 1 -1 0
     0 0  0  0 0 0 1  1 0
 1/2(0 1  1  1 1 1 1  1 1)  
 
     These are the coordinates in the even coordinate system.
     In particular, they lie at infinity. The indexing of the Dynkin diagram is


                   7
                   |
                   |
     0 ---- 1 ---- 2 ---- 3 ---- 4 ---- 5 ---- 6 
     
   */
   SparseVector<Rational> v(ones_vector<Rational>(9));
   v[0] = 0;
   v *= Rational(1,2);
   return (simple_roots_type_D(7) | zero_vector<Rational>(7)) / v;
}

SparseMatrix<Rational> simple_roots_type_F4()
{
   /*
     Read rowwise, these simple root vectors are
     0  1    -1     0    0
     0  0     1    -1    0
     0  0     0     1    0
     0 -1/2  -1/2  -1/2 -1/2

     The Dynkin diagram is:

     0 ---- 1 --(4)--> 2 ---- 3
   */
   SparseMatrix<Rational> R(4,5);
   R(0,1) = R(1,2) = R(2,3) = 1;
   R(0,2) = R(1,3) = -1;
   R(3,1) = R(3,2) = R(3,3) = R(3,4) = Rational(-1,2);
   return R;
}

SparseMatrix<Rational> simple_roots_type_G2()
{
   /*
     Read rowwise, these simple root vectors are
     0  1 -1  0
     0 -1  2 -1

     Notice that each row sums to zero.

     The Dynkin diagram is:

     0 <--(6)-- 1
   */
   SparseMatrix<Rational> R(2,4);
   R(0,1) = 1;
   R(0,2) = R(1,1) = R(1,3) = -1;
   R(1,2) = 2;
   return R;
}

typedef QuadraticExtension<Rational> QE;

SparseMatrix<QE> simple_roots_type_H3()
{
   const QE tau(Rational(1,2), Rational(1,2), 5); // golden ratio
   
   /*
     For H_3, the Dynkin diagram is

     0 --(5)-- 1 ---- 2,

     and the simple root vectors are, 

      0 2 0 0
      0 a b -1
      0 0 0 2

      with a=-tau and b=1/tau. Notice they all have length 2.
                              
   */

   SparseMatrix<QE> R(3,4);
   R(0,1) = R(2,3) = QE(2, 0, 5); 
   R(1,1) = -tau; R(1,2) = tau - 1; R(1,3) = QE(-1, 0, 5); 
   return R;
}

SparseMatrix<QE> simple_roots_type_H4()
{
   const QE tau(Rational(1,2), Rational(1,2), 5); // golden ratio

   /*
     For H_4, the Dynkin diagram is

     0 --(5)-- 1 ---- 2 ---- 3,

     and the simple root vectors are, according to 
     [John H. Stembridge, A construction of H_4 without miracles, 
      Discrete Comput. Geom. 22, No.3, 425-427 (1999)],

      0 a b b b
      0 -1 1 0 0
      0 0 -1 1 0
      0 0 0 -1 1

      with a=(1+tau)/2 and b=(1-tau)/2, so that the length of each root is sqrt{2}.
                              
   */
   SparseMatrix<QE> R(4, 5);
   Rows<SparseMatrix<QE> >::iterator rit = rows(R).begin();
 
   SparseVector<QE> v(5);
   v[1] = (1+tau)/2;
   v[2] = v[3] = v[4] = (1-tau)/2;
   *rit = v; ++rit;
   for (int i=0; i<3; ++i, ++rit) {
      SparseVector<QE> v(5);
      v[i+1] = QE(-1, 0, 5);
      v[i+2] = QE(1, 0, 5);
      *rit = v;
   }
   return R;
}

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the simple roots of the Coxeter arrangement of type A"
                  "# Indices are taken w.r.t. the Dynkin diagram  0 ---- 1 ---- ... ---- n-1"
                  "# Note that the roots lie at infinity to facilitate reflecting in them, and are normalized to length sqrt{2}." 
                  "# @param Int index of the arrangement (3, 4, etc)"
                  "# @return SparseMatrix",
                  &simple_roots_type_A, "simple_roots_type_A($)");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the simple roots of the Coxeter arrangement of type B"
                  "# Indices are taken w.r.t. the Dynkin diagram  0 ---- 1 ---- ... ---- n-2 --(4)--> n-1"
                  "# Note that the roots lie at infinity to facilitate reflecting in them, and are normalized to length sqrt{2}." 
                  "# @param Int index of the arrangement (3, 4, etc)"
                  "# @return SparseMatrix",
                  &simple_roots_type_B, "simple_roots_type_B($)");
 
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the simple roots of the Coxeter arrangement of type C"
                  "# Indices are taken w.r.t. the Dynkin diagram  0 ---- 1 ---- ... ---- n-2 <--(4)-- n-1"
                  "# Note that the roots lie at infinity to facilitate reflecting in them, and are normalized to length sqrt{2}." 
                  "# @param Int index of the arrangement (3, 4, etc)"
                  "# @return SparseMatrix",
                  &simple_roots_type_C, "simple_roots_type_C($)");
 
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the simple roots of the Coxeter arrangement of type D"
                  "# Indices are taken w.r.t. the Dynkin diagram"
                  "#                      n-2"
                  "#                      /"
                  "#     0 - 1 - ... - n-3"
                  "#                      \\"
                  "#                      n-1"
                  "# Note that the roots lie at infinity to facilitate reflecting in them, and are normalized to length sqrt{2}." 
                  "# @param Int index of the arrangement (3, 4, etc)"
                  "# @return SparseMatrix",
                  &simple_roots_type_D, "simple_roots_type_D($)");
 
UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the simple roots of the Coxeter arrangement of type E8"
                  "# Indices are taken w.r.t. the Dynkin diagram "
                  "#                                 7"
                  "#                                 |"
                  "#                                 |"
                  "#     0 ---- 1 ---- 2 ---- 3 ---- 4 ---- 5 ---- 6 "
                  "# Note that the roots lie at infinity to facilitate reflecting in them." 
                  "# @return SparseMatrix",
                  &simple_roots_type_E8, "simple_roots_type_E8()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the simple roots of the Coxeter arrangement of type F4"
                  "# Indices are taken w.r.t. the Dynkin diagram "
                  "#     0 ---- 1 --(4)--> 2 ---- 3"
                  "# @return SparseMatrix",
                  &simple_roots_type_F4, "simple_roots_type_F4()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the simple roots of the Coxeter arrangement of type G2"
                  "# Indices are taken w.r.t. the Dynkin diagram  0 <--(6)-- 1"
                  "# @return SparseMatrix",
                  &simple_roots_type_G2, "simple_roots_type_G2()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the simple roots of the Coxeter arrangement of type H3"
                  "# Indices are taken w.r.t. the Dynkin diagram  0 --(5)-- 1 ---- 2"
                  "# Note that the roots lie at infinity to facilitate reflecting in them, and are normalized to length 2" 
                  "# @return SparseMatrix",
                  &simple_roots_type_H3, "simple_roots_type_H3()");

UserFunction4perl("# @category Producing a polytope from scratch"
                  "# Produce the simple roots of the Coxeter arrangement of type H4"
                  "# Indices are taken w.r.t. the Dynkin diagram  0 --(5)-- 1 ---- 2 ---- 3"
                  "# Note that the roots lie at infinity to facilitate reflecting in them, and are normalized to length sqrt{2}" 
                  "# @return SparseMatrix",
                  &simple_roots_type_H4, "simple_roots_type_H4()");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
