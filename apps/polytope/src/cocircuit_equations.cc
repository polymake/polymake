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

#include "polymake/client.h"
#include "polymake/polytope/cocircuit_equations.h"

namespace polymake { namespace polytope {

template<typename Scalar, typename SetType>
ListMatrix<SparseVector<Int> > 
cocircuit_equations(Int d, 
                    const Matrix<Scalar>& points, 
                    const IncidenceMatrix<>& VIF, 
                    const Array<SetType>& interior_ridge_simplices, 
                    const Array<SetType>& interior_simplices, 
                    OptionSet options)
{
   return cocircuit_equations_impl(d, points, VIF, interior_ridge_simplices, interior_simplices, options);
}

FunctionTemplate4perl("cocircuit_equations<Scalar, SetType>($ Matrix<Scalar> IncidenceMatrix Array<SetType> Array<SetType> { filename=>'', reduce_rows=>1, log_frequency=>0 })");


template<typename Scalar, typename SetType>
SparseMatrix<Int> 
cocircuit_equations(BigObject P,
                    const Array<SetType>& interior_ridge_simplices, 
                    const Array<SetType>& interior_simplices, 
                    OptionSet options)
{
   const Int d = P.give("COMBINATORIAL_DIM");
   const Matrix<Scalar> points = P.give("RAYS");
   const IncidenceMatrix<> VIF = P.give("RAYS_IN_FACETS");
   return cocircuit_equations_impl(d, points, VIF, interior_ridge_simplices, interior_simplices, options);
}

UserFunctionTemplate4perl("# @category Combinatorics"
                          "# A matrix whose rows contain the cocircuit equations of a cone C"
                          "# with respect to a list of interior ridge simplices"
                          "# symmetries of the cone are NOT taken into account"
                          "# @param Cone C"
                          "# @param Array<Set> interior_ridge_simplices interior codimension 1 simplices"
                          "# @param Array<Set> interior_simplices interior simplices of maximal dimension"
                          "# @option [complete file] String filename where to write the output (default empty)"
                          "# @option Bool reduce_rows whether to perform row reduction (default 1)"
                          "# @option Int log_frequency how often to print log messages"
                          "# @return SparseMatrix<Int>",
                          "cocircuit_equations<Scalar,SetType>(Polytope<Scalar> Array<SetType> Array<SetType> { filename=>'', reduce_rows=>1, log_frequency=>0 })");

template<typename Scalar, typename SetType>
auto
cocircuit_equation_of_ridge(BigObject P,
                            const SetType& interior_ridge)
{
   const Matrix<Scalar> points = P.give("RAYS");
   return cocircuit_equation_of_ridge_impl(points, interior_ridge);
}

UserFunctionTemplate4perl("# @category Combinatorics"
                          "# The cocircuit equations of a cone C corresponding to some interior ridge rho"
                          "# with respect to a list of interior simplices"
                          "# symmetries of the cone are NOT taken into account"
                          "# @param Cone C"
                          "# @param Set rho the interior ridge"
                          "# @return HashMap<Set,Rational>",
                          "cocircuit_equation_of_ridge<Scalar, SetType>(Polytope<Scalar> SetType)");

template<typename Scalar, typename SetType>
ListMatrix<SparseVector<Int>>
foldable_cocircuit_equations(Int d,
                             const Matrix<Scalar>& points,
                             const IncidenceMatrix<>& VIF,
                             const Array<SetType>& interior_ridges, // FIXME: Map
                             const Array<SetType>& max_interior_simplices,
                             OptionSet options)
{
   return foldable_cocircuit_equations_impl(d, points, VIF, interior_ridges, max_interior_simplices, options, false);
}

FunctionTemplate4perl("foldable_cocircuit_equations<Scalar, SetType>($ Matrix<Scalar> IncidenceMatrix Array<SetType> Array<SetType> { filename=>'', reduce_rows=>1, log_frequency=>0 })");




} }

// Local Variables:
// mode:C++
// c-basic-offset:3
// indent-tabs-mode:nil
// End:

 
