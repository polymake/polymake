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

#ifndef POLYMAKE_COMMON_PRINT_CONSTRAINTS_H
#define POLYMAKE_COMMON_PRINT_CONSTRAINTS_H

#include "polymake/client.h"
#include "polymake/Matrix.h"
#include "polymake/Array.h"
#include "polymake/Rational.h"

namespace polymake { namespace common {

template <typename Scalar>
void print_constraints_sub(const Matrix<Scalar>& M, Array<std::string> coord_labels, Array<std::string> row_labels, const bool are_eqs, const bool homogeneous)
{
   if (M.cols() == 0)
      throw std::runtime_error("print_constraints - Invalid dimension 0!");

   const int start = homogeneous ? 0 : 1;

   if (coord_labels.size() > 0) {
      if (!homogeneous && coord_labels.size() == M.cols()-1) {
         Array<std::string> new_coord(1, "inhomog_var");
         new_coord.append(coord_labels.size(), coord_labels.begin());
         coord_labels = new_coord;
      }
      if (coord_labels.size() != M.cols())
         throw std::runtime_error("print_constraints - Wrong number of variables!");
   } else {
      const std::string var("x");
      coord_labels.resize(M.cols());
      for (int i=start; i<M.cols(); ++i)
         coord_labels[i]=var+std::to_string(i);

      // the coordinate label we assign below will not be used
      if (!homogeneous)
         coord_labels[0] = "inhomog_var";
   }

   for (int i=0; i < M.rows(); ++i) {
      if(i < row_labels.size())
         cout << row_labels[i];
      else
         cout << i;
      cout << ": ";
      if (is_zero(M.row(i).slice(start))) {
         cout << "0";
      } else {
         bool first=true;
         for (int j=start; j < M.cols(); ++j) {
            const Scalar cur_coeff = M.row(i)[j];
            if ( cur_coeff != 0 ) {
               if ( ! first )
                  cout << " ";
               if ( cur_coeff > 0 ) {
                  if ( ! first )
                     cout << "+ ";
                  if ( cur_coeff != 1 )
                     cout << std::setprecision(16) << cur_coeff << " ";
               }
               if ( cur_coeff < 0 ) {
                  if ( ! first )
                     cout << "- ";
                  else
                     cout << "-";
                  if ( cur_coeff != -1 )
                     cout << std::setprecision(16) << -cur_coeff << " ";
               }
               first=false;
               cout << coord_labels[j];
            }
         }
      }
      if ( are_eqs )
         cout << " = ";
      else
         cout << " >= ";
      Scalar neg_rhs = homogeneous ? zero_value<Scalar>() : M.row(i)[0];
      cout << std::setprecision(16) << (neg_rhs!=0 ? -neg_rhs : neg_rhs) << '\n';
   }
   cout << endl;
}

} }

#endif
