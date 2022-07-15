/* Copyright (c) 1997-2022
   Ewgenij Gawrilow, Michael Joswig, and the polymake team
   Technische Universität Berlin, Germany
   https://polymake.org

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

#pragma once

#include "polymake/Matrix.h"
#include "polymake/PowerSet.h"

namespace polymake { namespace polytope {

template <typename Scalar> 
void facets_of_simplex(const Set<Int>& simplex, const Matrix<Scalar>& V, Matrix<Scalar>& facets, Matrix<Scalar>& hull)
{
   if (rank(V.minor(simplex, All)) != simplex.size()){
      throw std::runtime_error("Affinely dependent point configuration wants to be a simplex");
   }
   hull = null_space(V.minor(simplex, All));
   facets = Matrix<Scalar>(simplex.size(), V.cols());
   Int i = 0;
   for (auto fbi = entire(all_subsets_less_1(simplex)); !fbi.at_end(); ++fbi, ++i) {
      const Set<Int> facet(*fbi);
      Vector<Scalar> nv = null_space(V.minor(facet, All) / hull).row(0);
      const Set<Int> rest = simplex - facet;
      const Int v = rest.front();
      if (nv * V.row(v) < 0) 
            nv = -nv;
      facets.row(i) = nv;
   }
}

template <typename Scalar> 
bool is_empty(const Set<Int>& simplex, const Matrix<Scalar>& V)
{
   if (simplex.size()==1) return true;
   Int i = 0;
   bool has_interior(false);
   Matrix<Scalar> facets, hull;
   facets_of_simplex(simplex, V, facets, hull);
   for (auto rit = entire(rows(V)); !rit.at_end() && !has_interior; ++rit, ++i) {
      if (!simplex.contains(i)){
         if (hull.cols() == 0 || accumulate(attach_operation(hull*(*rit), operations::abs_value()), operations::max()) == 0) {
            if (facets.cols() != 0)
               has_interior = (accumulate(facets*(*rit), operations::min()) >= 0);
            else
               has_interior = true;
      }
      }
   }
   return !has_interior;
}

template<typename SetType, typename IncidenceType>
bool is_in_boundary(const SetType& face, const IncidenceType& VIF)
{
   for (Int f = 0; f < VIF.rows(); ++f) 
      if (incl(face, VIF.row(f)) <= 0) 
         return true;
   return false;   
}

template<typename SetType>
bool is_interior(const SetType& ridge, const IncidenceMatrix<>& VIF)
{
    for (auto fit = entire(rows(VIF)); !fit.at_end(); ++fit) 
        if ((ridge * (*fit)).size() == ridge.size())
            return false;
    return true;
}


} }


// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

