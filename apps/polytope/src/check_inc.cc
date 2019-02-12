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
#include <sstream>

namespace polymake { namespace polytope {

template <typename Scalar>
bool check_inc(const Matrix<Scalar>& P, const Matrix<Scalar>& H, std::string sign_arg, bool verbose)
{
   bool ok=true,
     minus=false, equal=false, plus=false;  // what is allowed
   int n_minus=0, n_equal=0, n_plus=0;         // number of point/hyperplane pairs

   // analyze sign argument
   for (char c : sign_arg) {
      switch (c) {
      case '-' : minus = true; break;
      case '0' : equal = true; break;
      case '+' : plus  = true; break;
      default :
         throw std::runtime_error("check_inc: unrecognized sign argument " + sign_arg);
      }
   }

   // read the points and hyperplanes
   // matrices of points and hyperplanes, rsp.

   if (!P.rows() || !H.rows())   // nothing to do
      return true;
   if (P.cols() != H.cols())
      throw std::runtime_error("check_inc: P/H matrix dimension mismatch");

   // check all pairs
   Scalar scp;
   for (int i=0; i<P.rows(); ++i) {
      for (int j=0; j<H.rows(); ++j) {
         scp = P[i] * H[j];
         int s=sign(scp);
         if (s<0) {
            n_minus++;
            if (!minus) {
               if (verbose)
                  cout << "<" << i << "," << j << ">   ( "
                       << P[i] << " ) * [ " << H[j] << " ] == " << scp << endl;
               ok=false;
            }
         } else if (s==0) {
            n_equal++;
            if (!equal) {
               if (verbose)
                  cout << "<" << i << "," << j << ">   ( "
                       << P[i] << " ) * [ " << H[j] << " ] == " << scp << endl;
               ok=false;
            }
         } else {
            n_plus++;
            if (!plus) {
               if (verbose)
                  cout << "<" << i << "," << j << ">   ( "
                       << P[i] << " ) * [ " << H[j] << " ] == " << scp << endl;
               ok=false;
            }
         }
      }
   }

   // print statistics if data not plausible
   if (!ok && verbose)
      cout << "#points==" << P.rows() << ", #hyperplanes==" << H.rows()
           << ", -:" << n_minus << ", 0:" << n_equal
           << ", +:" << n_plus << ", total:" << P.rows()*H.rows() << endl;
   return ok;
}

UserFunctionTemplate4perl("# @category Consistency check"
                  "# Check coordinate data. For each pair of vectors from two given matrices"
                  "# their inner product must satisfy the given relation."
                  "# @param Matrix points"
                  "# @param Matrix hyperplanes"
                  "# @param String sign composed of one or two characters from [-+0], representing the"
                  "#  allowed domain of the vector inner products."
                  "# @param Bool verbose print all products violating the required relation"
                  "# @return Bool 'true' if all relations are satisfied, 'false' otherwise"
                  "# @example Let's check which vertices of the square lie in its zeroth facet:"
                  "# > $H = cube(2)->FACETS->minor([0],All);"
                  "# > print check_inc(cube(2)->VERTICES,$H,'0',1);"
                  "# | <1,0>   ( 1 1 -1 ) * [ 1 1 0 ] == 2"
                  "# | <3,0>   ( 1 1 1 ) * [ 1 1 0 ] == 2"
                  "# | \\#points==4, \\#hyperplanes==1, -:0, 0:2, +:2, total:4"
                  "# | false"
                  "# Thus, the first and third vertex don't lie on the hyperplane defined by the facet"
                  "# but on the positive side of it, and the remaining two lie on the hyperplane.",
                  "check_inc<Scalar>(Matrix<type_upgrade<Scalar>> Matrix<type_upgrade<Scalar>> $; $=0)");
} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
