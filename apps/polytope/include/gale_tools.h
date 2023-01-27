/* Copyright (c) 1997-2023
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

namespace polymake { namespace polytope {

template <typename TMatrix, typename E>
Matrix<E> balance(const GenericMatrix<TMatrix, E>& G_arg)
{
   // Given a matrix of vectors in R^d (actually, R^{n-d}), we want to
   // scale d of them by positive coefficients so that the entire set
   // of vectors has the origin as barycenter. To do this, we
   // exhaustively check all d-tuples of vectors for feasibility.
   Matrix<E> G (G_arg);
   const Int n = G.rows(), d = G.cols();
   Vector<E> coeffs;
   bool success (false);
   for (auto r = entire(all_subsets_of_k(sequence(0,n), d)); !r.at_end() && !success; ++r) {
      Set<Int> comp (sequence(0,n)), rset(*r);
      comp -= rset;
      const Vector<E> mbarycenter = - ones_vector<E>(comp.size()) * G.minor(comp,All);
      try {
         coeffs = lin_solve(T(G.minor(rset,All)), mbarycenter);
      } catch (const linalg_error& e) {
         continue;
      }
      success = accumulate(coeffs, operations::min()) > 0;
      if (success) {
         auto vit = entire(coeffs);
         for (auto sit = entire(rset); !sit.at_end() && !vit.at_end(); ++sit, ++vit)
            G.row(*sit) *= *vit;
      }
   }
   if (!success)
      throw std::runtime_error("Could not balance the Gale diagram. This shouldn't have happened.");
   return G;
}

} } // namespaces



// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:
